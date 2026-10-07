# Feraxia native overlay redesign

User chose the full Feraxia-branded redesign and requested a fork of Detanup01/gbe_fork. Base: release-2026_09_27, commit 7103add7ca6ef3ef8353279219880068f2494fc8. Fork: https://github.com/froster01/feraxia-gbe.

Keep upstream renderer hooks, Steam API exports, networking, invitations, achievements, screenshots, input and callbacks. Change the native Dear ImGui presentation. No new backend/API integration or fake account/room information.

Use Feraxia charcoal surfaces (#0a0a0b, #131315, #1e1e21), titanium text (#c9cacc), crimson focus/selected accent (#e01b24), readable type and rounded panels. The main Shift+Tab interface has a Feraxia header and shortcut hint, friends/invite area, and clear navigation for achievements, screenshots and settings. Restyle existing chat, notifications and auxiliary windows consistently; keep their working callbacks and controls. Preserve upstream configuration visibility and hotkey behavior.

Adapt layout to viewport and font scaling without negative child sizes or controls outside the screen. No full-screen opaque obstruction when the overlay is closed. Do not introduce blocking I/O into rendering.

Build x86/x64 experimental Steam API DLLs using VS2022 and pinned upstream submodule/dependency inputs. Keep upstream notices/licenses and source history. Initially verify the fork separately; do not replace Feraxia packaged binaries until both architecture builds and tests pass and new pins/source attribution are in place. Actual in-game Shift+Tab rendering and invites remain manual validation.
