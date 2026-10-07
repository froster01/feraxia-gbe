# Feraxia Native Overlay Implementation Plan

**Goal:** Redesign the experimental native overlay in the user's fork while preserving Steam/game behavior.

**Architecture:** Dedicated native theme/layout helpers plus surgical changes to overlay presentation, retaining upstream callback and resource ownership. Parent owns fork/build setup and application source documentation; native implementer owns overlay presentation and layout tests.

**Tech Stack:** C++17/20, Dear ImGui, Premake, Visual Studio 2022, existing overlay renderer.

- [x] Create GitHub fork, local sibling checkout, upstream remote and feature branch from current app release.
- [x] Initialize pinned Windows submodules and dependency build inputs; generate VS2022 experimental project.
- [x] Add meaningful layout/theme safety regressions and verify failure, then implement Feraxia main overlay, navigation, notification and auxiliary styling. Keep callbacks and visibility flags intact.
- [x] Build Release api_experimental x64 and Win32; run layout tests and native verification. Review spec compliance and code quality independently; fix important findings.
- [x] Update Feraxia AGENTS.md with fork/local source; record status and build evidence. Do not swap app runtime binaries unless both native architectures verified and matching pins/source packaging updated.

## Verification on 2026-10-07

- Pinned Windows dependencies built successfully for x86 and x64 with VS2022.
- Full Release `api_experimental` builds passed for x64 and Win32. An initial Win32 DOS-stub post-build failure exposed upstream's x86/x32 helper filename mismatch; the narrow Premake filter fix was reviewed, then both builds passed.
- 120 viewport/font geometry cases and real ImGui frames passed. Auxiliary tests cover viewport/font transitions, actual decoration and clip bounds, valid dragging, edge clamping and collapsed windows. Regressions were reproduced before their fixes.
- Independent specification and code-quality reviews approved the final UI and DOS-stub fix.
- Real hidden Direct3D11 fixture captures inspected at desktop and compact sizes. They do not exercise game hooks or connected Steam peers.
- Both DLL PE architectures and complete export sets match their official upstream baseline (1,282 exports each).
- Local pre-commit build IDs are marked `+local`. Verification data is in `build/feraxia-artifact-verification.json`; app binaries were not replaced.
- Native source diff and documentation formatting checks passed. Actual in-game Shift+Tab, invites and app resource integration remain separate validation steps.
