# Feraxia GBE fork

Code index: use the `codebase-mcp` server (codebase-memory MCP, project
`froster01_feraxia-gbe_1408674745`). It indexes the pushed repository on the server, not this
checkout; check `index_status` and `check_index_coverage` before relying on it, and read the source
when coverage is missing or stale. Keep decisions and handoffs in the repository or the conversation.

Fork: https://github.com/froster01/feraxia-gbe
Upstream: https://github.com/Detanup01/gbe_fork
Feature base: release-2026_09_27 (7103add7ca6ef3ef8353279219880068f2494fc8).
Sibling app: ../feraxia; read its AGENTS.md for packaging and pinned resource update rules.

Preserve upstream Steam API/network/invite behavior. UI lives in overlay_experimental/steam_overlay.cpp and overlay_experimental/overlay/steam_overlay.h. Renderer and ImGui dependencies come from pinned third-party submodules. Local Windows build targets VS2022; upstream default scripts currently target VS2026 and ARM too, so use explicit VS2022 generation and x86/x64 api_experimental targets for Feraxia.

Do not replace app DLLs or release the fork until both architectures build, resource hashes and corresponding source/attribution are coordinated, and review is complete. A native build does not prove actual game rendering/invite compatibility. Preserve notices and licenses.
