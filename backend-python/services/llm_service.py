import logging
import json
import httpx
from typing import List, Dict, Any, AsyncGenerator
from config import settings

logger = logging.getLogger("domos.llm")

# System prompt defining DomOS identity and capabilities
SYSTEM_PROMPT = """You are DomOS, an intelligent AI voice terminal assistant running on embedded hardware.
You are helpful, concise, and direct in your responses because your voice output is read aloud to the user.
Keep answers under 3 sentences unless explicitly asked for details.

You can control Smart Home devices via MQTT function calls when requested.
Available Smart Home actions:
- set_light(room: string, device: string, state: "on"|"off", brightness: int=100)
- set_climate(room: string, target_temp: int)
- run_scene(scene_name: string)
"""

class LLMService:
    def __init__(self):
        self.default_model = settings.DEFAULT_LLM_MODEL

    async def chat_completion(
        self,
        messages: List[Dict[str, str]],
        provider: str = "openai",
        model: str = "gpt-4o",
    ) -> Dict[str, Any]:
        """Generate response from selected LLM provider."""
        formatted_messages = [{"role": "system", "content": SYSTEM_PROMPT}] + messages

        if provider == "openai" or (provider == "default" and settings.OPENAI_API_KEY):
            return await self._call_openai(formatted_messages, model or "gpt-4o")
        elif provider == "claude" and settings.ANTHROPIC_API_KEY:
            return await self._call_claude(formatted_messages, model or "claude-3-5-sonnet-20241022")
        elif provider == "gemini" and settings.GEMINI_API_KEY:
            return await self._call_gemini(formatted_messages)
        else:
            # Fallback to local Ollama or simulated response
            return await self._call_ollama(formatted_messages)

    async def _call_openai(self, messages: List[Dict[str, str]], model: str) -> Dict[str, Any]:
        from openai import AsyncOpenAI
        client = AsyncOpenAI(api_key=settings.OPENAI_API_KEY)
        try:
            response = await client.chat.completions.create(
                model=model,
                messages=messages, # type: ignore
                max_tokens=300,
                temperature=0.7,
            )
            text = response.choices[0].message.content or ""
            return {"text": text, "provider": "openai", "model": model}
        except Exception as e:
            logger.error(f"OpenAI error: {e}")
            return await self._call_ollama(messages)

    async def _call_claude(self, messages: List[Dict[str, str]], model: str) -> Dict[str, Any]:
        from anthropic import AsyncAnthropic
        client = AsyncAnthropic(api_key=settings.ANTHROPIC_API_KEY)
        system = messages[0]["content"] if messages[0]["role"] == "system" else ""
        user_messages = [m for m in messages if m["role"] != "system"]
        try:
            response = await client.messages.create(
                model=model,
                max_tokens=300,
                system=system,
                messages=user_messages, # type: ignore
            )
            text = response.content[0].text
            return {"text": text, "provider": "claude", "model": model}
        except Exception as e:
            logger.error(f"Claude error: {e}")
            return await self._call_ollama(messages)

    async def _call_gemini(self, messages: List[Dict[str, str]]) -> Dict[str, Any]:
        # Fallback implementation for Gemini
        return await self._call_ollama(messages)

    async def _call_ollama(self, messages: List[Dict[str, str]]) -> Dict[str, Any]:
        """Local Ollama endpoint or offline response."""
        try:
            async with httpx.AsyncClient(timeout=10.0) as client:
                resp = await client.post(
                    f"{settings.OLLAMA_BASE_URL}/api/chat",
                    json={"model": "llama3", "messages": messages, "stream": False},
                )
                if resp.status_code == 200:
                    data = resp.json()
                    return {"text": data["message"]["content"], "provider": "ollama", "model": "llama3"}
        except Exception:
            pass

        # Offline local echo for testing
        last_user_msg = messages[-1]["content"] if messages else "Hello"
        return {
            "text": f"I am DomOS (Offline Mode). You said: '{last_user_msg}'. All device systems operational.",
            "provider": "offline",
            "model": "rule-based",
        }

llm_service = LLMService()
