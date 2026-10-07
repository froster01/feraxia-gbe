# Feraxia native overlay fork

Source fork: [froster01/feraxia-gbe](https://github.com/froster01/feraxia-gbe).
Upstream: [Detanup01/gbe_fork](https://github.com/Detanup01/gbe_fork).
The initial redesign is based on `release-2026_09_27` to match the sibling
Feraxia application's current emulator version.

The experimental overlay uses a centered charcoal shell, titanium text,
crimson accents, responsive navigation, and a friends/invite area. Existing
achievements, screenshots, chat, notification history, settings and Steam
callbacks remain native upstream features. This does not add a Feraxia account
or room API inside a game.

## Build and verify

Use Windows with Visual Studio 2022 C++ tools and a Windows SDK installed.
From this repository, run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-feraxia-overlay.ps1 -PrepareDependencies
```

The first build initializes pinned Windows dependencies and compiles them for
both architectures. Subsequent builds can omit `-PrepareDependencies`. The
helper builds only Release `api_experimental` for x64 and Win32, using VS2022;
it does not use upstream's optional fake certificate signing or build ARM.
Outputs are under `build/win/vs2022/release/experimental/x64/` and `x86/`.

Run focused layout and real ImGui checks with
`tests/feraxia-overlay/run-layout-tests.cmd` and
`tests/feraxia-overlay/run-ui-tests.cmd`. See
[the test guide](tests/feraxia-overlay/README.md) for a hidden Direct3D preview.
Preview content is an explicit fixture; it is not a connected game.

## Application integration

The sibling application is `../feraxia`. Its `AGENTS.md` maps all related source
repositories and resource packaging entry points. Building this fork does not
replace the app's pinned, official DLLs or modify installed games.

Before integrating a new fork build, verify both architectures, record exact
binary and corresponding-source hashes, update the app's build/runtime pins and
attribution together, preserve licenses and recovery behavior, and validate
actual Shift+Tab rendering and invitations in a supported game. Generated
settings must preserve `disable_lan_only=1` for Feraxia VPN peers and apply the
desired appearance values; explicit existing upstream INI colors can override
the new native default palette.

Keep the upstream LGPL license and dependency notices. Native presentation
changes live in `overlay_experimental/`; Steam API/networking implementation
changes are outside this redesign's scope.
