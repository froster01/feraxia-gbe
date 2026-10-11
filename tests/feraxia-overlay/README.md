# Feraxia native overlay checks

Run these from any working directory on Windows with VS2022 C++ tools installed:

- `run-layout-tests.cmd`: standalone font-relative geometry regressions over 120 viewport/font combinations. The first test run against a full-screen/no-rail stub failed the compact, readable-rail and game-margin requirements before implementation.
- `run-texture-tests.cmd [x64|x86]`: real DX11 WARP regression for backend texture invalidation after a frame is prepared. Covers repeated invalidation, recreation before drawing, four production fonts, atlas growth, a hidden/reopened panel and intentional texture retirement. Run both architectures. The original pinned ImGui fails at frame 8 with an invalid live texture ID and no recreation request; the backport must recover without removing assertions. This fault injection reproduces the invalid texture condition, not a particular game's hardware/hook trigger.
- `run-ui-tests.cmd`: real pinned ImGui context/draw-frame checks at 12 viewport/font combinations, plus a persistent auxiliary window that survives 1920x1080 -> 640x480, font scaling, valid/out-of-bounds dragging and collapsing. The fixture checks clip/inner rectangles and actual decoration/content vertices on the same frame. The earlier post-Begin clamp fails those visual assertions; the pre-Begin clamp passes.
- `run-preview.cmd [output.bmp] [width] [height] [font-scale]`: real D3D11 capture of the shared shell/theme helpers in a hidden native window. Example: `run-preview.cmd build\feraxia-preview-640.bmp 640 480 1.5`. Uses the pinned ImGui Win32/DX11 example renderer and shader blobs. No visible window or game process is needed. Output paths are relative to the repository.

The UI checks and preview require the pinned `ingame_overlay` Windows dependency checkout at `build/deps/win/vs2022/ingame_overlay`. Test executables/objects go into `%TEMP%`. VS2022 is discovered with `vswhere`.

The preview is clearly labeled as a fixture with no connected peers. It renders the same production shell, navigation, content panels and palette; it does not instantiate Steam networking, game input hooks, actual friend actions, achievements or screenshot resources. Build and game-level Shift+Tab/invitation tests remain separate requirements. Its default ImGui font is a fixture font; production retains configured upstream fonts.

No parent application resources or DLL pins are modified by these checks.

The dependency build backports only `ImTextureData::SetStatus()` live-texture
recreation from Dear ImGui v1.92.4, associated with upstream issue #8811:
https://github.com/ocornut/imgui/blob/v1.92.4/imgui.h . The bundled dependency
otherwise remains the pinned Nemirtingas ImGui version; this is not a full
ImGui upgrade. `premake5-deps.lua` applies the exact change to extracted source
and rejects an unexpected implementation. `build-feraxia-overlay.ps1` rebuilds
and installs the renderer on cached builds too, so the static library and headers
agree. Game-level Shift+Tab, invites, graphics resets and OBS compatibility still
require validation on the affected PC.

The preview adapts Dear ImGui's Win32/Direct3D11 example. Its MIT notice is preserved in `LICENSE.imgui.txt`.
