# Feraxia GBE fork

<!-- workspace-ai:start -->

Shared AI Workspace rules:

- At the beginning of every session, determine the current Git project and branch, then call the shared-ai-workspace MCP tool workspace_bootstrap before editing.
- Treat the returned handoff, active work, memories, and decisions as the continuation context for this session.
- Record durable project knowledge with workspace_write_memory and architectural choices with workspace_record_decision.
- Keep active work updated when the task direction changes.
- Before ending a task or switching tools, call workspace_write_handoff with completed work, files changed, tests run, blockers, constraints, and the exact next step.
- Never claim shared context was loaded unless the MCP call succeeded.

<!-- workspace-ai:end -->

If workspace_write_handoff is unavailable, write equivalent shared memory.

Fork: https://github.com/froster01/feraxia-gbe
Upstream: https://github.com/Detanup01/gbe_fork
Feature base: release-2026_09_27 (7103add7ca6ef3ef8353279219880068f2494fc8).
Sibling app: ../feraxia; read its AGENTS.md for packaging and pinned resource update rules.

Preserve upstream Steam API/network/invite behavior. UI lives in overlay_experimental/steam_overlay.cpp and overlay_experimental/overlay/steam_overlay.h. Renderer and ImGui dependencies come from pinned third-party submodules. Local Windows build targets VS2022; upstream default scripts currently target VS2026 and ARM too, so use explicit VS2022 generation and x86/x64 api_experimental targets for Feraxia.

Do not replace app DLLs or release the fork until both architectures build, resource hashes and corresponding source/attribution are coordinated, and review is complete. A native build does not prove actual game rendering/invite compatibility. Preserve notices and licenses.
