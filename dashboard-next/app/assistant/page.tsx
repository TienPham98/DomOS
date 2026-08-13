"use client";

import { useState } from "react";
import { Mic, Send, Bot, User, Sparkles, Brain, Settings2 } from "lucide-react";
import { PageHeader, Section } from "@/components/dashboard-primitives";
import { Button } from "@/components/ui/button";

const llmProviders = [
  { id: "openai", name: "OpenAI GPT-4o", icon: "🧠", status: "connected" },
  { id: "claude", name: "Claude Sonnet", icon: "🤖", status: "connected" },
  { id: "gemini", name: "Gemini Pro", icon: "✨", status: "connected" },
  { id: "ollama", name: "Ollama (local)", icon: "🏠", status: "offline" },
];

const demoConversation = [
  { role: "user" as const, text: "What's the weather like today?" },
  {
    role: "assistant" as const,
    text: "Currently in Ho Chi Minh City it's 32°C with partly cloudy skies. Humidity is at 72%. Would you like me to set up a weather widget on your DomOS dashboard?",
  },
  { role: "user" as const, text: "Yes, and turn on the living room lights" },
  {
    role: "assistant" as const,
    text: "Done! I've added a weather widget to your dashboard and sent an MQTT command to turn on the living room lights via Home Assistant. The lights should be on now. 💡",
  },
];

export default function AssistantPage() {
  const [messages, setMessages] = useState(demoConversation);
  const [input, setInput] = useState("");
  const [activeProvider, setActiveProvider] = useState("openai");
  const [wakeWordEnabled, setWakeWordEnabled] = useState(true);

  const handleSend = () => {
    if (!input.trim()) return;
    setMessages((prev) => [
      ...prev,
      { role: "user", text: input },
      {
        role: "assistant",
        text: "I'll process that request through the AI Gateway. This feature will be fully functional when the Python backend is connected.",
      },
    ]);
    setInput("");
  };

  return (
    <>
      <PageHeader
        title="Assistant"
        subtitle="Intelligence"
        badge="Hello Dom"
        action={
          <div className="flex items-center gap-3">
            <div className="flex items-center gap-2 px-3 py-1.5 rounded-lg bg-emerald-500/10 border border-emerald-500/20">
              <div className="dom-dot dom-dot-online" />
              <span className="text-xs text-emerald-400 font-medium">
                Wake Word {wakeWordEnabled ? "Active" : "Off"}
              </span>
            </div>
          </div>
        }
      />

      <div className="grid gap-6 xl:grid-cols-3">
        {/* Chat Panel */}
        <div className="xl:col-span-2 dom-card-static flex flex-col" style={{ height: "calc(100vh - 200px)" }}>
          {/* Messages */}
          <div className="flex-1 overflow-y-auto p-6 space-y-4">
            {messages.map((msg, i) => (
              <div
                key={i}
                className={`flex gap-3 animate-fade-in ${
                  msg.role === "user" ? "flex-row-reverse" : ""
                }`}
                style={{ animationDelay: `${i * 60}ms` }}
              >
                <div
                  className={`flex items-center justify-center w-8 h-8 rounded-xl flex-shrink-0 ${
                    msg.role === "user"
                      ? "bg-cyan-500/10"
                      : "bg-purple-500/10"
                  }`}
                >
                  {msg.role === "user" ? (
                    <User className="w-4 h-4 text-cyan-400" />
                  ) : (
                    <Bot className="w-4 h-4 text-purple-400" />
                  )}
                </div>
                <div
                  className={`max-w-[75%] px-4 py-3 rounded-2xl text-sm leading-relaxed ${
                    msg.role === "user"
                      ? "bg-cyan-500/10 text-white rounded-tr-sm"
                      : "bg-slate-800/50 text-slate-300 rounded-tl-sm"
                  }`}
                >
                  {msg.text}
                </div>
              </div>
            ))}
          </div>

          {/* Input bar */}
          <div className="p-4 border-t border-slate-800/50">
            <div className="flex gap-3">
              <button className="flex items-center justify-center w-10 h-10 rounded-xl bg-slate-800 hover:bg-slate-700 transition flex-shrink-0">
                <Mic className="w-5 h-5 text-slate-400" />
              </button>
              <input
                type="text"
                value={input}
                onChange={(e) => setInput(e.target.value)}
                onKeyDown={(e) => e.key === "Enter" && handleSend()}
                placeholder="Ask Dom anything…"
                className="flex-1 px-4 py-2 rounded-xl bg-slate-900 border border-slate-700 text-sm text-white placeholder-slate-600 focus:border-cyan-500 focus:outline-none"
              />
              <Button onClick={handleSend} className="px-4">
                <Send className="w-4 h-4" />
              </Button>
            </div>
          </div>
        </div>

        {/* Settings Panel */}
        <div className="space-y-6">
          <Section title="LLM Providers">
            <div className="space-y-2">
              {llmProviders.map((provider) => (
                <button
                  key={provider.id}
                  className={`w-full flex items-center gap-3 p-3 rounded-xl border transition text-left ${
                    activeProvider === provider.id
                      ? "border-cyan-500/50 bg-cyan-500/5"
                      : "border-slate-800/50 hover:border-slate-700"
                  }`}
                  onClick={() => setActiveProvider(provider.id)}
                >
                  <span className="text-xl">{provider.icon}</span>
                  <div className="flex-1 min-w-0">
                    <p className="text-sm font-medium text-white">{provider.name}</p>
                    <p className={`text-xs ${provider.status === "connected" ? "text-emerald-400" : "text-slate-500"}`}>
                      {provider.status}
                    </p>
                  </div>
                  {activeProvider === provider.id && (
                    <span className="dom-badge dom-badge-cyan text-[10px]">Active</span>
                  )}
                </button>
              ))}
            </div>
          </Section>

          <Section title="Voice Pipeline">
            <div className="space-y-3">
              <div className="flex items-center justify-between p-3 rounded-xl border border-slate-800/50">
                <div className="flex items-center gap-2">
                  <Mic className="w-4 h-4 text-slate-500" />
                  <span className="text-sm text-white">Wake Word</span>
                </div>
                <div
                  className={`w-10 h-6 rounded-full transition-colors flex items-center px-1 cursor-pointer ${
                    wakeWordEnabled ? "bg-cyan-500" : "bg-slate-700"
                  }`}
                  onClick={() => setWakeWordEnabled(!wakeWordEnabled)}
                >
                  <div
                    className={`w-4 h-4 rounded-full bg-white transition-transform ${
                      wakeWordEnabled ? "translate-x-4" : "translate-x-0"
                    }`}
                  />
                </div>
              </div>
              <div className="p-3 rounded-xl border border-slate-800/50">
                <div className="flex items-center gap-2 mb-2">
                  <Sparkles className="w-4 h-4 text-purple-400" />
                  <span className="text-sm text-white">STT Engine</span>
                </div>
                <p className="text-xs text-slate-500">Whisper (via Python Gateway)</p>
              </div>
              <div className="p-3 rounded-xl border border-slate-800/50">
                <div className="flex items-center gap-2 mb-2">
                  <Brain className="w-4 h-4 text-cyan-400" />
                  <span className="text-sm text-white">TTS Engine</span>
                </div>
                <p className="text-xs text-slate-500">Edge-TTS / ElevenLabs</p>
              </div>
            </div>
          </Section>

          <Section title="Capabilities">
            <div className="flex flex-wrap gap-2">
              {["Weather", "Smart Home", "Calendar", "News", "Timer", "Music", "Notes", "Reminders"].map(
                (cap) => (
                  <span key={cap} className="dom-badge dom-badge-cyan">
                    {cap}
                  </span>
                )
              )}
            </div>
          </Section>
        </div>
      </div>
    </>
  );
}
