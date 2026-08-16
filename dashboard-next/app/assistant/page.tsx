import { Bot, CheckCircle2, Clock3, Cloud, Database, History, TerminalSquare, Wrench } from "lucide-react";

export const dynamic = "force-dynamic";

type ToolTrace = { id: string; name: string; arguments: Record<string, unknown>; result: unknown; duration_ms: number; status: "success" | "error"; created_at: string };
type ConversationTurn = { id: string; device_id: string; user_text: string; assistant_text: string; provider: string; model: string; created_at: string; completed_at: string | null; tool_calls: ToolTrace[] };
type ConversationResponse = { items: ConversationTurn[]; count: number };

const gatewayUrl = process.env.NEXT_PUBLIC_AI_GATEWAY_URL ?? "http://localhost:8000";

function formatTime(value: string | null) {
  if (!value) return "Đang xử lý";
  return new Intl.DateTimeFormat("vi-VN", { hour: "2-digit", minute: "2-digit", second: "2-digit", timeZone: "Asia/Bangkok" }).format(new Date(value));
}

function formatJson(value: unknown) {
  return JSON.stringify(value, null, 2);
}

async function getConversations(): Promise<{ data: ConversationResponse; error: string | null }> {
  try {
    const response = await fetch(`${gatewayUrl}/api/v1/conversations?limit=50`, { cache: "no-store" });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return { data: await response.json(), error: null };
  } catch (error) {
    return { data: { items: [], count: 0 }, error: error instanceof Error ? error.message : "Không thể kết nối gateway" };
  }
}

export default async function AssistantPage() {
  const { data, error } = await getConversations();
  return (
    <>
      <header className="mb-8 flex flex-col gap-4 sm:flex-row sm:items-end sm:justify-between">
        <div>
          <p className="mb-1 text-sm font-medium tracking-wide text-cyan-400">OpenRouter Free</p>
          <div className="flex items-center gap-3">
            <h2 className="text-3xl font-bold tracking-tight text-white">Dom AI</h2>
            <span className="dom-badge dom-badge-cyan">Cloud AI</span>
          </div>
        </div>
        <div className="flex items-center gap-2 rounded-lg border border-emerald-500/20 bg-emerald-500/10 px-3 py-1.5">
          <span className="dom-dot dom-dot-online" />
          <span className="text-xs font-medium text-emerald-400">Lưu lịch sử tự động</span>
        </div>
      </header>

      <div className="grid gap-6 xl:grid-cols-[minmax(0,2fr)_minmax(280px,1fr)]">
        <section className="dom-card-static overflow-hidden">
          <div className="flex items-center justify-between border-b border-slate-800/70 px-6 py-5">
            <div className="flex items-center gap-3"><History className="h-5 w-5 text-cyan-400" /><h3 className="font-semibold text-white">Lịch sử trò chuyện</h3></div>
            <span className="text-xs text-slate-500">{data.count} lượt gần nhất</span>
          </div>
          {error ? (
            <div className="m-6 rounded-xl border border-red-500/20 bg-red-500/5 p-4 text-sm text-red-300">Gateway chưa sẵn sàng: {error}</div>
          ) : data.items.length === 0 ? (
            <div className="flex min-h-80 flex-col items-center justify-center px-6 text-center">
              <Bot className="mb-4 h-10 w-10 text-slate-600" />
              <p className="font-medium text-slate-300">Chưa có cuộc trò chuyện</p>
              <p className="mt-2 max-w-sm text-sm text-slate-500">Nói với Dom trên thiết bị. Transcript, phản hồi và lệnh điều khiển sẽ xuất hiện tại đây.</p>
            </div>
          ) : (
            <div className="divide-y divide-slate-800/70">
              {data.items.map((turn) => (
                <article key={turn.id} className="space-y-4 p-6">
                  <div className="flex items-start justify-between gap-4">
                    <p className="text-base leading-relaxed text-white">{turn.user_text}</p>
                    <time className="shrink-0 font-mono text-xs text-slate-600">{formatTime(turn.created_at)}</time>
                  </div>
                  {turn.tool_calls.map((tool) => (
                    <div key={tool.id} className="rounded-xl border border-violet-500/20 bg-violet-500/5 p-4">
                      <div className="mb-3 flex flex-wrap items-center justify-between gap-2">
                        <div className="flex items-center gap-2 text-sm font-medium text-violet-300"><Wrench className="h-4 w-4" />Lệnh gọi công cụ</div>
                        <div className="flex items-center gap-3 text-xs text-slate-500"><span>{tool.duration_ms}ms</span><span className={tool.status === "success" ? "text-emerald-400" : "text-red-400"}>{tool.status === "success" ? "Thành công" : "Lỗi"}</span></div>
                      </div>
                      <code className="block overflow-x-auto whitespace-pre-wrap rounded-lg bg-slate-950/60 px-3 py-2 text-xs text-cyan-300">{tool.name}({formatJson(tool.arguments)})</code>
                      <details className="mt-2 text-xs text-slate-500"><summary className="cursor-pointer select-none">Kết quả thiết bị</summary><pre className="mt-2 overflow-x-auto whitespace-pre-wrap rounded-lg bg-slate-950/60 p-3 text-slate-400">{formatJson(tool.result)}</pre></details>
                    </div>
                  ))}
                  {turn.assistant_text && (
                    <div className="rounded-2xl border border-cyan-500/15 bg-cyan-500/5 p-4">
                      <div className="flex items-start justify-between gap-4"><p className="leading-relaxed text-slate-200">{turn.assistant_text}</p><time className="shrink-0 font-mono text-xs text-slate-600">{formatTime(turn.completed_at)}</time></div>
                      <p className="mt-3 flex items-center gap-1.5 text-[11px] text-slate-600"><CheckCircle2 className="h-3 w-3" />Nội dung trên được tạo bởi AI</p>
                    </div>
                  )}
                </article>
              ))}
            </div>
          )}
        </section>

        <aside className="space-y-6">
          <section className="dom-card-static p-6">
            <h3 className="mb-5 text-lg font-semibold text-white">Trạng thái AI</h3>
            <div className="space-y-4 text-sm">
              <StatusRow icon={Cloud} label="Provider" value="OpenRouter" />
              <StatusRow icon={Cloud} label="Nhận dạng giọng nói" value="Google Cloud" />
              <StatusRow icon={TerminalSquare} label="Model" value="openrouter/free" />
              <StatusRow icon={Database} label="Bộ nhớ" value="SQLite bền vững" />
              <StatusRow icon={Clock3} label="Ngữ cảnh" value="12 lượt gần nhất" />
              <StatusRow icon={Bot} label="Kích hoạt" value="Hey Dom hoặc chạm" />
            </div>
          </section>
          <section className="dom-card-static p-6">
            <h3 className="mb-3 text-lg font-semibold text-white">Cách hoạt động</h3>
            <p className="text-sm leading-relaxed text-slate-500">Mỗi câu nói, phản hồi và kết quả điều khiển thiết bị được lưu trên server. Dom dùng lại lịch sử này ở những lần trò chuyện sau, kể cả sau khi gateway khởi động lại.</p>
          </section>
        </aside>
      </div>
    </>
  );
}

function StatusRow({ icon: Icon, label, value }: { icon: typeof Cloud; label: string; value: string }) {
  return <div className="flex items-center justify-between gap-4"><span className="flex items-center gap-2 text-slate-500"><Icon className="h-4 w-4" />{label}</span><span className="text-right text-slate-200">{value}</span></div>;
}
