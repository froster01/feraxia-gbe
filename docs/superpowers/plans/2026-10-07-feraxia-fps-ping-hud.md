# Feraxia FPS and ping HUD Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Show real FPS and peer ping in the Feraxia overlay (always-on HUD plus a ping column in the friends list), hide the Copy ID and Screenshots buttons by default, and make the rare-achievement toast border visible.

**Architecture:** Ping is measured by timing the existing `PING` to `PONG` UDP announce exchange per connection, with no change to `dll/net.proto`. A pure header (`ping_tracker.h`) holds the timing and tier logic and is unit-tested standalone. `Networking::Run` publishes a small snapshot under its own mutex, and the overlay render thread reads only that snapshot, never the live `connections` vector. An optional 1 second probe, enabled only while the overlay needs ping, replaces the 5 second broadcast cadence.

**Tech Stack:** C++17, ImGui (pinned, via InGameOverlay), MSVC VS2022, standalone `cl` test executables under `tests/feraxia-overlay`.

## Global Constraints

- Branch `feat/feraxia-overlay`, base `7103add7ca6ef3ef8353279219880068f2494fc8` (release-2026_09_27). Do not touch `dev`.
- Preserve upstream Steam API, network and invite behavior. No change to `dll/net.proto`. Probes are off by default and use the existing `PING` message only.
- Use `std::chrono::steady_clock` for all ping timing. Do not pass it to `check_timedout`, which takes `high_resolution_clock` time points.
- `Networking::Run` runs on the game thread and takes no locks. The overlay renders on another thread. Never read `Networking::connections` from the overlay.
- Windows build uses explicit VS2022 and x86 and x64 `api_experimental` targets only. Use `scripts/build-feraxia-overlay.ps1`.
- Every new visible string goes through `steam_overlay_translations.h` (31 languages) and `create_fonts`' `font_builder.AddText` list. Follow the existing `FPS: ` precedent: the same text in all 31 languages.
- Ping tiers: under 60 ms good, 60 to 150 ms fair, over 150 ms poor. Colors: good uses the normal text color, fair `0xd9a441`, poor `0xe01b24`, unknown `0x929399`.
- Do not replace app DLLs, release, push tags, or open a PR. Preserve notices and licenses. A native build does not prove in-game behavior.
- Commit messages end with `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.

## File Structure

| File | Responsibility |
| --- | --- |
| `dll/dll/ping_tracker.h` (new) | `PingTracker` (per-connection RTT smoothing), `PingTier`, `ping_tier()`. Pure, no sockets, no ImGui. |
| `tests/feraxia-overlay/ping_test.cpp` (new) | Standalone tests for `ping_tracker.h`. |
| `tests/feraxia-overlay/run-ping-tests.cmd` (new) | Builds and runs `ping_test.cpp` with VS2022. |
| `dll/dll/network.h`, `dll/network.cpp` | Per-connection tracker, probes, snapshot publish and getter. |
| `dll/dll/settings.h`, `dll/settings_parser.cpp` | `overlay_always_show_ping`, `overlay_show_checkbox_ping`, new Copy ID and Screenshots defaults. |
| `post_build/steam_settings.EXAMPLE/configs.overlay.EXAMPLE.ini` | Documented INI keys. |
| `overlay_experimental/overlay/steam_overlay_translations.h` | `translationPingDisplay`, `translationPingCheckbox`. |
| `overlay_experimental/overlay/feraxia_theme.h` | `ping_color()`. |
| `overlay_experimental/overlay/steam_overlay_stats.h`, `overlay_experimental/steam_overlay_stats.cpp` | HUD shows ping, colored by tier. |
| `overlay_experimental/overlay/steam_overlay.h`, `overlay_experimental/steam_overlay.cpp` | Snapshot cache, probe enable, worst ping, checkbox, friend row ping, rare border color. |

---

### Task 1: Hide Copy ID and Screenshots by default, fix the rare toast border

**Files:**
- Modify: `dll/dll/settings.h:381-382`
- Modify: `post_build/steam_settings.EXAMPLE/configs.overlay.EXAMPLE.ini:76-77`
- Modify: `overlay_experimental/steam_overlay.cpp:1266-1268`

**Interfaces:**
- Consumes: nothing.
- Produces: `overlay_show_button_copy_id` and `overlay_show_button_screenshots` default `false`. The friend right-click "Copy ID" entry and the screenshot hotkey are unaffected.

- [ ] **Step 1: Change the two defaults in `dll/dll/settings.h`**

```cpp
    bool overlay_show_button_copy_id = false;
    bool overlay_show_button_screenshots = false;
```

- [ ] **Step 2: Change the example INI**

In `configs.overlay.EXAMPLE.ini` set `show_button_copy_id=0` and `show_button_screenshots=0`. Check the comment lines above them say `default=0` for these two keys, and edit them if they say `default=1`.

- [ ] **Step 3: Make the rare toast border visible**

In `steam_overlay.cpp` replace the near-black rare color (`32/255, 24/255, 8/255`) with a bronze that reads on the `#131315` toast background:

```cpp
        ImGui::PushStyleColor(ImGuiCol_Border, is_rare_achievement
            ? feraxia::color(0xd9a441, settings_noti_alpha)
            : feraxia::color(0xe01b24, settings_noti_alpha));
```

- [ ] **Step 4: Verify the edits**

Run: `git diff --stat` and `grep -n "overlay_show_button_copy_id\|overlay_show_button_screenshots" dll/dll/settings.h`
Expected: the two defaults show `false`, and the rare border line no longer contains `32.0f / 255.0f`.

- [ ] **Step 5: Commit**

```bash
git add dll/dll/settings.h post_build/steam_settings.EXAMPLE/configs.overlay.EXAMPLE.ini overlay_experimental/steam_overlay.cpp
git commit -m "feat(overlay): hide copy id and screenshots by default, visible rare border"
```

---

### Task 2: PingTracker and tiers (pure, test first)

**Files:**
- Create: `tests/feraxia-overlay/ping_test.cpp`
- Create: `tests/feraxia-overlay/run-ping-tests.cmd`
- Create: `dll/dll/ping_tracker.h`

**Interfaces:**
- Consumes: nothing.
- Produces (used by Tasks 3, 6, 7):
  - `struct PingTracker { void sent(clock::time_point); void pong(clock::time_point); std::optional<int> ms(clock::time_point) const; }` with `using clock = std::chrono::steady_clock`.
  - `enum class PingTier { none, good, fair, poor }` and `inline PingTier ping_tier(std::optional<int> ms)`.

- [ ] **Step 1: Write the failing test**

`tests/feraxia-overlay/ping_test.cpp`:

```cpp
#include <cstdio>
#include "../../dll/dll/ping_tracker.h"
int main() {
    int failures = 0;
    auto check = [&](bool condition, const char *message) {
        if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); ++failures; }
    };
    const PingTracker::clock::time_point t0{};
    auto at = [&](int ms) { return t0 + std::chrono::milliseconds(ms); };

    { PingTracker p; check(!p.ms(at(0)).has_value(), "no sample before any pong"); }
    { PingTracker p; p.pong(at(40)); check(!p.ms(at(40)).has_value(), "pong without a sent ping is ignored"); }
    { PingTracker p; p.sent(at(0)); p.pong(at(40)); check(p.ms(at(40)) == 40, "first sample is the raw round trip"); }
    { PingTracker p; p.sent(at(0)); p.pong(at(40)); p.pong(at(60)); check(p.ms(at(60)) == 40, "duplicate pong from a second interface is ignored"); }
    { PingTracker p; p.sent(at(0)); p.pong(at(2500)); check(!p.ms(at(2500)).has_value(), "reply slower than 2 s is not a sample"); }
    { PingTracker p; p.sent(at(0)); p.sent(at(1000)); p.pong(at(1030)); check(p.ms(at(1030)) == 30, "latest ping time is used"); }
    { PingTracker p; p.sent(at(0)); p.pong(at(40)); p.sent(at(1000)); p.pong(at(1080));
      check(p.ms(at(1080)) == 52, "second sample is smoothed (40 + 0.3 * (80 - 40))"); }
    { PingTracker p; p.sent(at(0)); p.pong(at(40));
      check(p.ms(at(40 + 15000)).has_value(), "sample still valid at 15 s");
      check(!p.ms(at(40 + 15001)).has_value(), "sample goes stale after 15 s"); }

    check(ping_tier(std::nullopt) == PingTier::none, "unknown ping has no tier");
    check(ping_tier(59) == PingTier::good, "59 ms is good");
    check(ping_tier(60) == PingTier::fair, "60 ms is fair");
    check(ping_tier(150) == PingTier::fair, "150 ms is fair");
    check(ping_tier(151) == PingTier::poor, "151 ms is poor");
    std::printf("%s: ping tracker\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
```

`tests/feraxia-overlay/run-ping-tests.cmd`:

```bat
@echo off
setlocal
pushd "%~dp0\..\.."
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 1
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -version [17.0^,18.0^) -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "FERAXIA_VS=%%i"
if not defined FERAXIA_VS exit /b 1
call "%FERAXIA_VS%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 tests\feraxia-overlay\ping_test.cpp /Fe:"%TEMP%\feraxia-ping-test.exe" /Fo:"%TEMP%\feraxia-ping-test.obj"
if errorlevel 1 exit /b 1
"%TEMP%\feraxia-ping-test.exe"
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `tests\feraxia-overlay\run-ping-tests.cmd`
Expected: compile error, `ping_tracker.h` not found.

- [ ] **Step 3: Write the implementation**

`dll/dll/ping_tracker.h`:

```cpp
#pragma once
#include <chrono>
#include <cmath>
#include <optional>

// Times the existing PING -> PONG UDP exchange for one connection.
// Pure logic: no sockets, no locks. Owned and used by the networking thread only.
struct PingTracker {
    using clock = std::chrono::steady_clock;
    static constexpr std::chrono::milliseconds max_sample{2000};
    static constexpr std::chrono::seconds stale_after{15};
    static constexpr double smoothing = 0.3;

    // Call right before a PING is sent to this connection. Later calls overwrite earlier ones.
    void sent(clock::time_point now) {
        sent_at = now;
        awaiting = true;
    }

    // Call when a PONG arrives from this connection.
    void pong(clock::time_point now) {
        if (!awaiting) return;
        awaiting = false;
        const auto elapsed = now - sent_at;
        if (elapsed < clock::duration::zero() || elapsed > max_sample) return;
        const double sample = std::chrono::duration<double, std::milli>(elapsed).count();
        smoothed_ms = has_sample ? smoothed_ms + smoothing * (sample - smoothed_ms) : sample;
        has_sample = true;
        last_sample_at = now;
    }

    std::optional<int> ms(clock::time_point now) const {
        if (!has_sample || now - last_sample_at > stale_after) return std::nullopt;
        return static_cast<int>(std::lround(smoothed_ms));
    }

private:
    clock::time_point sent_at{};
    clock::time_point last_sample_at{};
    double smoothed_ms = 0.0;
    bool awaiting = false;
    bool has_sample = false;
};

enum class PingTier { none, good, fair, poor };

inline PingTier ping_tier(std::optional<int> ms) {
    if (!ms.has_value()) return PingTier::none;
    if (*ms < 60) return PingTier::good;
    if (*ms <= 150) return PingTier::fair;
    return PingTier::poor;
}
```

- [ ] **Step 4: Run the test to verify it passes**

Run: `tests\feraxia-overlay\run-ping-tests.cmd`
Expected: `PASS: ping tracker`, exit code 0.

- [ ] **Step 5: Commit**

```bash
git add dll/dll/ping_tracker.h tests/feraxia-overlay/ping_test.cpp tests/feraxia-overlay/run-ping-tests.cmd
git commit -m "feat(network): add ping tracker with unit tests"
```

---

### Task 3: Measure ping in Networking and publish a snapshot

**Files:**
- Modify: `dll/dll/network.h` (Connection struct near line 88, private members near line 107, public section near line 160)
- Modify: `dll/network.cpp` (`handle_announce` ~line 694-716, `send_announce_broadcasts` ~line 890, `Run` ~line 908-1140)

**Interfaces:**
- Consumes: `PingTracker` from Task 2.
- Produces (used by Task 7):
  - `void Networking::set_ping_probes(bool enable)`
  - `std::unordered_map<uint64_t, int> Networking::get_peer_pings()`: steam id to smoothed ms. Only ids with a fresh sample appear. Safe to call from any thread.

- [ ] **Step 1: Add the includes, the tracker field and the new members to `network.h`**

Add after `#include "base.h"`:

```cpp
#include "ping_tracker.h"
#include <atomic>
#include <mutex>
#include <unordered_map>
```

In `struct Connection`, add as the last field:

```cpp
    PingTracker ping{};
```

In `class Networking`, add under the private members next to `last_broadcast`:

```cpp
    // ping measurement: written by Run() on the game thread, read through ping_mutex only
    std::atomic<bool> ping_probes{false};
    std::chrono::steady_clock::time_point last_probe{};
    std::chrono::steady_clock::time_point last_ping_snapshot{};
    std::mutex ping_mutex;
    std::unordered_map<uint64_t, int> ping_snapshot{};
    void send_ping_probes();
    void publish_ping_snapshot(std::chrono::steady_clock::time_point now);
```

In the public section, after `uint32 getOwnIP();`:

```cpp
    // while true, PING is sent to every answered peer once a second instead of every 5 s broadcast
    void set_ping_probes(bool enable);
    // steam id -> smoothed round trip in ms, only peers with a fresh sample. Thread-safe.
    std::unordered_map<uint64_t, int> get_peer_pings();
```

- [ ] **Step 2: Mark pings as sent and record PONGs in `network.cpp`**

In `send_announce_broadcasts()`, immediately before `Common_Message msg = create_announce(true);`:

```cpp
    {
        const auto ping_now = std::chrono::steady_clock::now();
        for (auto &c : connections) c.ping.sent(ping_now);
    }
```

In `handle_announce`, inside `if (!conn->udp_pinged) {` before it serializes the message, add `conn->ping.sent(std::chrono::steady_clock::now());`. In the `else if (msg->announce().type() == Announce::PONG) {` branch, add as the first statement:

```cpp
        conn->ping.pong(std::chrono::steady_clock::now());
```

- [ ] **Step 3: Add the probe, snapshot and getter functions**

Add after `send_announce_broadcasts()`:

```cpp
void Networking::send_ping_probes()
{
    const auto now = std::chrono::steady_clock::now();
    last_probe = now;

    Common_Message msg = create_announce(true);
    size_t size = msg.ByteSizeLong();
    std::vector<char> buffer(size);
    msg.SerializeToArray(&buffer[0], static_cast<int>(size));
    for (auto &conn : connections) {
        if (!conn.udp_pinged) continue; // only peers that already answered over UDP
        conn.ping.sent(now);
        send_packet_to(udp_socket, conn.udp_ip_port, &buffer[0], static_cast<unsigned long>(size));
    }
}

void Networking::publish_ping_snapshot(std::chrono::steady_clock::time_point now)
{
    if (now - last_ping_snapshot < std::chrono::milliseconds(500)) return;
    last_ping_snapshot = now;

    std::unordered_map<uint64_t, int> fresh;
    for (const auto &conn : connections) {
        const auto ms = conn.ping.ms(now);
        if (!ms.has_value()) continue;
        for (const auto &id : conn.ids) fresh[id.ConvertToUint64()] = *ms;
    }

    std::lock_guard<std::mutex> lock(ping_mutex);
    ping_snapshot = std::move(fresh);
}

void Networking::set_ping_probes(bool enable)
{
    ping_probes.store(enable, std::memory_order_relaxed);
}

std::unordered_map<uint64_t, int> Networking::get_peer_pings()
{
    std::lock_guard<std::mutex> lock(ping_mutex);
    return ping_snapshot;
}
```

- [ ] **Step 4: Call them from `Run()`**

After the `if (check_timedout(last_broadcast, BROADCAST_INTERVAL)) { send_announce_broadcasts(); }` block:

```cpp
    if (ping_probes.load(std::memory_order_relaxed) &&
        std::chrono::steady_clock::now() - last_probe >= std::chrono::seconds(1)) {
        send_ping_probes();
    }
```

Immediately before the final `reset_last_error();` of `Run()`:

```cpp
    publish_ping_snapshot(std::chrono::steady_clock::now());
```

- [ ] **Step 5: Build both architectures**

Run: `powershell -File scripts/build-feraxia-overlay.ps1`
Expected: Release `api_experimental` for x64 and Win32 build with no new errors (upstream warnings are known).

- [ ] **Step 6: Commit**

```bash
git add dll/dll/network.h dll/network.cpp
git commit -m "feat(network): measure peer ping from PING/PONG with optional 1s probes"
```

---

### Task 4: Settings and INI keys for the ping HUD

**Files:**
- Modify: `dll/dll/settings.h` (near line 385 and 397)
- Modify: `dll/settings_parser.cpp` (near lines 1694 and 1709)
- Modify: `post_build/steam_settings.EXAMPLE/configs.overlay.EXAMPLE.ini` (near lines 64 and 80)

**Interfaces:**
- Consumes: nothing.
- Produces (used by Tasks 6 and 7): `Settings::overlay_show_checkbox_ping` (default `true`) and `Settings::overlay_always_show_ping` (default `false`).

- [ ] **Step 1: Add the fields to `settings.h`**

After `bool overlay_show_checkbox_playtime = true;`:

```cpp
    bool overlay_show_checkbox_ping = true;
```

After `bool overlay_always_show_playtime = false;`:

```cpp
    bool overlay_always_show_ping = false;
```

- [ ] **Step 2: Parse both keys in `settings_parser.cpp`**

After the `show_checkbox_playtime` pair:

```cpp
    settings_client->overlay_show_checkbox_ping = ini.GetBoolValue("overlay::general", "show_checkbox_ping", settings_client->overlay_show_checkbox_ping);
    settings_server->overlay_show_checkbox_ping = ini.GetBoolValue("overlay::general", "show_checkbox_ping", settings_server->overlay_show_checkbox_ping);
```

After the `overlay_always_show_playtime` pair:

```cpp
    settings_client->overlay_always_show_ping = ini.GetBoolValue("overlay::general", "overlay_always_show_ping", settings_client->overlay_always_show_ping);
    settings_server->overlay_always_show_ping = ini.GetBoolValue("overlay::general", "overlay_always_show_ping", settings_server->overlay_always_show_ping);
```

- [ ] **Step 3: Document the keys in the example INI**

After `overlay_always_show_playtime=0`:

```ini
# 1=always show the slowest peer ping in the overlay HUD
# measured from the PING/PONG exchange between peers, shows -- until a peer answers
# default=0
overlay_always_show_ping=0
```

After `show_checkbox_playtime=...` (use the same comment style as its neighbors):

```ini
# 1=show the ping checkbox in the overlay
# default=1
show_checkbox_ping=1
```

- [ ] **Step 4: Verify**

Run: `grep -n "ping" dll/dll/settings.h dll/settings_parser.cpp post_build/steam_settings.EXAMPLE/configs.overlay.EXAMPLE.ini`
Expected: 2 lines in `settings.h`, 4 in `settings_parser.cpp`, 2 key lines in the INI plus comments.

- [ ] **Step 5: Commit**

```bash
git add dll/dll/settings.h dll/settings_parser.cpp post_build/steam_settings.EXAMPLE/configs.overlay.EXAMPLE.ini
git commit -m "feat(overlay): add ping HUD settings"
```

---

### Task 5: Translations for the ping strings

**Files:**
- Modify: `overlay_experimental/overlay/steam_overlay_translations.h` (append before the closing `#endif`)
- Modify: `overlay_experimental/steam_overlay.cpp` (`create_fonts`, near line 482)

**Interfaces:**
- Consumes: nothing.
- Produces (used by Tasks 6 and 7): `translationPingDisplay[lang]` equal to `"Ping: "` and `translationPingCheckbox[lang]` equal to `"Ping"`, 31 languages each, same text in all (the existing `FPS` strings do the same).

- [ ] **Step 1: Generate the two arrays from the FPS ones**

```bash
F=overlay_experimental/overlay/steam_overlay_translations.h
gen() { # $1 = source array, $2 = new array, $3 = old text, $4 = new text
  S=$(grep -n "^const char $1\[" $F | cut -d: -f1)
  E=$(awk -v s=$S 'NR>s && /^};/ {print NR; exit}' $F)
  sed -n "${S},${E}p" $F | sed "s/$1/$2/; s/u8\"$3\"/u8\"$4\"/"
}
{ echo; gen translationFpsCheckbox translationPingCheckbox 'FPS' 'Ping'; echo; gen translationFpsDisplay translationPingDisplay 'FPS: ' 'Ping: '; } > /tmp/ping_translations.txt
LAST=$(grep -n '^#endif // _STEAM_OVERLAY_TRANSLATIONS_H' $F | cut -d: -f1)
sed -i "$((LAST-1))r /tmp/ping_translations.txt" $F
```

- [ ] **Step 2: Verify the arrays**

Run: `grep -c 'u8"Ping' overlay_experimental/overlay/steam_overlay_translations.h`
Expected: `62` (31 per array). Run `grep -n "translationPing" overlay_experimental/overlay/steam_overlay_translations.h` and confirm both arrays appear before `#endif`.

- [ ] **Step 3: Register the strings for the font atlas**

In `create_fonts`, after `font_builder.AddText(translationFpsDisplay[i]);`:

```cpp
        font_builder.AddText(translationPingCheckbox[i]);
        font_builder.AddText(translationPingDisplay[i]);
```

- [ ] **Step 4: Commit**

```bash
git add overlay_experimental/overlay/steam_overlay_translations.h overlay_experimental/steam_overlay.cpp
git commit -m "feat(overlay): add ping translations"
```

---

### Task 6: Ping in the stats HUD

**Files:**
- Modify: `overlay_experimental/overlay/feraxia_theme.h` (add `ping_color` inside `namespace feraxia`)
- Modify: `overlay_experimental/overlay/steam_overlay_stats.h`
- Modify: `overlay_experimental/steam_overlay_stats.cpp`

**Interfaces:**
- Consumes: `PingTier`, `ping_tier` (Task 2), `overlay_always_show_ping` (Task 4), `translationPingDisplay` and `translationFrametimeUnitDisplay` (Task 5).
- Produces (used by Task 7): `bool Steam_Overlay_Stats::show_ping` and `int Steam_Overlay_Stats::ping_ms` (`-1` means unknown). `show_any_stats()` includes `show_ping`.

- [ ] **Step 1: Add `ping_color` to `feraxia_theme.h`**

Add `#include "dll/ping_tracker.h"` with the other includes, then after `color()`:

```cpp
// Normal text color for a good ping, amber for fair, crimson for poor, dim grey when unknown.
inline ImVec4 ping_color(PingTier tier, const ImVec4 &normal, float alpha = 1.f) {
    switch (tier) {
        case PingTier::good: return normal;
        case PingTier::fair: return color(0xd9a441, alpha);
        case PingTier::poor: return color(0xe01b24, alpha);
        default: return color(0x929399, alpha);
    }
}
```

- [ ] **Step 2: Extend `Steam_Overlay_Stats`**

In `steam_overlay_stats.h`, after `bool show_playtime = false;`:

```cpp
    bool show_ping = false;
    int ping_ms = -1; // slowest connected peer, set by Steam_Overlay each frame, -1 = unknown
```

In `steam_overlay_stats.cpp`, add `show_ping = settings->overlay_always_show_ping;` to the constructor and change `show_any_stats()` to:

```cpp
    return show_fps || show_frametime || show_playtime || show_ping;
```

- [ ] **Step 3: Draw the ping at the end of the HUD**

At the top of `steam_overlay_stats.cpp` add `#include "overlay/feraxia_theme.h"` and `#include "dll/ping_tracker.h"`. In `render_stats`, after `const auto stats_txt = stats_txt_buff.str();` replace the sizing and drawing so the ping is a separate colored segment:

```cpp
    std::string ping_txt;
    if (show_ping) {
        ping_txt = translationPingDisplay[current_language];
        ping_txt += ping_ms >= 0
            ? std::to_string(ping_ms) + translationFrametimeUnitDisplay[current_language]
            : std::string("--");
    }
    const std::string separator = (!stats_txt.empty() && !ping_txt.empty()) ? " | " : "";
    const std::string full_txt = stats_txt + separator + ping_txt;

    // set FPS box width/height based on text size
    const auto msg_box = ImGui::CalcTextSize(full_txt.c_str(), full_txt.c_str() + full_txt.size());
```

Keep the existing `stats_box`, `SetNextWindowSize`, anchor and `SetNextWindowPos` code unchanged. Replace the single `ImGui::TextWrapped("%s", stats_txt.c_str());` inside the window with:

```cpp
        if (!stats_txt.empty()) {
            ImGui::TextUnformatted((stats_txt + separator).c_str());
            if (!ping_txt.empty()) ImGui::SameLine(0.f, 0.f);
        }
        if (!ping_txt.empty()) {
            const auto tier = ping_ms >= 0 ? ping_tier(ping_ms) : PingTier::none;
            ImGui::TextColored(
                feraxia::ping_color(tier, ImGui::GetStyleColorVec4(ImGuiCol_Text), settings->overlay_appearance.stats_text_a),
                "%s", ping_txt.c_str());
        }
```

- [ ] **Step 4: Build to check it compiles**

Run: `powershell -File scripts/build-feraxia-overlay.ps1`
Expected: both architectures build.

- [ ] **Step 5: Commit**

```bash
git add overlay_experimental/overlay/feraxia_theme.h overlay_experimental/overlay/steam_overlay_stats.h overlay_experimental/steam_overlay_stats.cpp
git commit -m "feat(overlay): show colored ping in the stats HUD"
```

---

### Task 7: Overlay wiring: snapshot cache, probes, checkbox, friend row ping

**Files:**
- Modify: `overlay_experimental/overlay/steam_overlay.h` (includes, private members, one method)
- Modify: `overlay_experimental/steam_overlay.cpp` (`overlay_render_proc` ~line 1823, checkbox ~line 1986, friend row ~line 2108)

**Interfaces:**
- Consumes: `Networking::get_peer_pings`, `Networking::set_ping_probes` (Task 3), `stats.show_ping`, `stats.ping_ms` (Task 6), `overlay_show_checkbox_ping` (Task 4), `translationPingCheckbox` (Task 5).
- Produces: nothing for later tasks.

- [ ] **Step 1: Add state to `steam_overlay.h`**

Add `#include <unordered_map>` with the other includes. In the private section after `std::atomic<bool> i_have_lobby = false;`:

```cpp
    // ping cache for the render thread, refreshed twice a second from Networking's snapshot
    std::unordered_map<uint64_t, int> peer_pings{};
    std::chrono::steady_clock::time_point last_ping_refresh{};
    void refresh_peer_pings();
```

- [ ] **Step 2: Add `refresh_peer_pings` to `steam_overlay.cpp`**

Place it just before `Steam_Overlay::overlay_render_proc()`:

```cpp
void Steam_Overlay::refresh_peer_pings()
{
    const auto now = std::chrono::steady_clock::now();
    if (now - last_ping_refresh < std::chrono::milliseconds(500)) return;
    last_ping_refresh = now;

    // probes cost one tiny UDP packet per peer per second, only run them while ping is visible
    network->set_ping_probes(stats.show_ping || show_overlay);
    peer_pings = network->get_peer_pings();

    int worst = -1;
    for (const auto &entry : friends) {
        const auto found = peer_pings.find(entry.first.id());
        if (found != peer_pings.end() && found->second > worst) worst = found->second;
    }
    stats.ping_ms = worst;
}
```

- [ ] **Step 3: Call it every frame**

In `overlay_render_proc`, immediately before `if (stats.show_any_stats()) {`:

```cpp
    refresh_peer_pings();
```

- [ ] **Step 4: Add the checkbox**

In `render_main_window`, after the Playtime checkbox block:

```cpp
        if (settings->overlay_show_checkbox_ping) {
            if (ImGui::Checkbox(translationPingCheckbox[current_language], &stats.show_ping)) {
                allow_renderer_frame_processing(stats.show_ping);
            }
        }
```

- [ ] **Step 5: Add the ping to each friend row**

In the `std::for_each` friends lambda, after the `if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0)) { ... }` block and before `ImGui::PopID();` (it must come after, because the context menu and click test use the Selectable as the last item):

```cpp
                    char ping_label[32];
                    const auto found_ping = peer_pings.find(i.first.id());
                    if (found_ping == peer_pings.end()) {
                        snprintf(ping_label, sizeof(ping_label), "--");
                    } else {
                        snprintf(ping_label, sizeof(ping_label), "%d%s", found_ping->second, translationFrametimeUnitDisplay[current_language]);
                    }
                    ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(ping_label).x - ImGui::GetStyle().FramePadding.x);
                    const auto row_tier = found_ping == peer_pings.end() ? PingTier::none : ping_tier(found_ping->second);
                    ImGui::TextColored(feraxia::ping_color(row_tier, ImGui::GetStyleColorVec4(ImGuiCol_Text)), "%s", ping_label);
```

- [ ] **Step 6: Build and run all native tests**

Run: `powershell -File scripts/build-feraxia-overlay.ps1`, then `tests\feraxia-overlay\run-layout-tests.cmd`, `tests\feraxia-overlay\run-ui-tests.cmd`, `tests\feraxia-overlay\run-ping-tests.cmd`
Expected: both architectures build, and all three suites print `PASS`.

- [ ] **Step 7: Commit**

```bash
git add overlay_experimental/overlay/steam_overlay.h overlay_experimental/steam_overlay.cpp
git commit -m "feat(overlay): wire peer ping into the HUD, checkbox and friends list"
```

---

### Task 8: Verification and handoff

**Files:**
- Modify: `tests/feraxia-overlay/README.md` (list `run-ping-tests.cmd`)
- Modify: `FERAXIA.md` (one paragraph: how ping is measured and its limits)

**Interfaces:**
- Consumes: everything above.
- Produces: verified candidate for integration, not a release.

- [ ] **Step 1: Document**

Add to `FERAXIA.md`: ping is the smoothed PING to PONG round trip over UDP between peers, refreshed every 5 s from the normal broadcast and every 1 s while the overlay is open or the ping HUD is on. It shows `--` until a peer answers over UDP and goes stale after 15 s. It measures the peer-to-peer path, not a Steam server. Add `run-ping-tests.cmd` to the README test list.

- [ ] **Step 2: Clean build and static checks**

Run: `powershell -File scripts/build-feraxia-overlay.ps1` and `git diff --check`
Expected: both architectures build, no whitespace errors. Record the DLL sizes and SHA256 in `build/feraxia-artifact-verification.json` the same way the earlier build did.

- [ ] **Step 3: Manual two-PC check (cannot be automated)**

On two PCs over the VPN, same app ID, build copied next to the game:
1. Start both. Open the overlay with the toggle key. Expected: the friend row shows `--` for a few seconds, then a value close to the VPN round trip (compare with `ping <other PC VPN IP>`).
2. Tick Ping in the rail. Expected: the HUD shows `Ping: NN ms`, amber over 60 ms, crimson over 150 ms.
3. Close the overlay and untick Ping. Expected: no probe traffic (check with a packet capture on UDP if available).
4. Kill the other peer. Expected: ping goes to `--` within 15 s, and the friend disappears at the existing 20 s timeout.
5. Send a chat message and an invite both ways. Expected: unchanged behavior.
6. Unlock the test achievement of a rare title. Expected: bronze border visible.

- [ ] **Step 4: Record the handoff**

Write the shared-workspace handoff (completed work, files changed, tests run, limits, exact next step). State plainly that native builds and unit tests do not prove in-game rendering, invite compatibility or ping accuracy until step 3 is done.

- [ ] **Step 5: Commit**

```bash
git add FERAXIA.md tests/feraxia-overlay/README.md
git commit -m "docs: describe ping measurement and tests"
```

---

## Self-review

- **Spec coverage:** FPS (already present, now also selectable with the existing checkbox) and ping HUD: Tasks 3, 4, 6, 7. Ping per friend: Task 7 step 5. Defaults for Copy ID and Screenshots: Task 1. Rare border: Task 1. Colors and tiers: Tasks 2 and 6. Option 1 measurement with no wire change: Task 3. Thread safety: Task 3 snapshot plus Task 7 cache.
- **Not covered here (separate plan):** the v2 visual changes (user card, stat tiles, friend state tags, achievements layout). They need a read of the achievements and user-info render code first.
- **Placeholder scan:** none. Task 4 step 3 says to copy the comment style of the neighboring INI key, which is a style instruction, not missing content.
- **Type consistency:** `PingTracker::sent/pong/ms`, `PingTier`, `ping_tier`, `Networking::set_ping_probes/get_peer_pings`, `Steam_Overlay_Stats::show_ping/ping_ms`, `feraxia::ping_color(PingTier, const ImVec4&, float)` are used with the same names and signatures in every task.
- **Known risks:** `GetContentRegionMax().x` positioning in the friend row may need adjustment once seen in the D3D11 preview. The HUD ping text uses the stats text alpha only for non-good tiers. `Networking::connections` ids can hold several Steam IDs per connection, so all of them map to the same ping.
