#pragma once
#include <algorithm>
#include "feraxia_layout.h"
#include "../../dll/dll/ping_tracker.h"
#ifndef IMGUI_VERSION
#include "InGameOverlay/ImGui/imgui.h"
#endif
#if __has_include("InGameOverlay/ImGui/imgui_internal.h")
#include "InGameOverlay/ImGui/imgui_internal.h"
#else
#include "imgui_internal.h"
#endif
namespace feraxia {
inline ImVec4 color(unsigned rgb, float alpha = 1.f) {
    return ImVec4(float((rgb >> 16) & 255) / 255.f, float((rgb >> 8) & 255) / 255.f,
        float(rgb & 255) / 255.f, alpha);
}
// Normal text color for a good ping, amber for fair, crimson for poor, dim grey when unknown.
inline ImVec4 ping_color(PingTier tier, const ImVec4 &normal, float alpha = 1.f) {
    switch (tier) {
        case PingTier::good: return normal;
        case PingTier::fair: return color(0xd9a441, alpha);
        case PingTier::poor: return color(0xe01b24, alpha);
        default: return color(0x929399, alpha);
    }
}
inline void apply_theme(ImGuiStyle &s) {
    s.WindowRounding = 12.f;
    s.ChildRounding = 8.f;
    s.FrameRounding = 5.f;
    s.PopupRounding = 8.f;
    s.GrabRounding = 4.f;
    s.ScrollbarRounding = 6.f;
    s.WindowBorderSize = 1.f;
    s.ChildBorderSize = 1.f;
    s.WindowPadding = ImVec2(18.f, 16.f);
    s.FramePadding = ImVec2(12.f, 8.f);
    s.ItemSpacing = ImVec2(10.f, 10.f);
    s.Colors[ImGuiCol_Text] = color(0xc9cacc);
    s.Colors[ImGuiCol_TextDisabled] = color(0x929399);
    s.Colors[ImGuiCol_WindowBg] = color(0x131315);
    s.Colors[ImGuiCol_ChildBg] = color(0x0a0a0b);
    s.Colors[ImGuiCol_PopupBg] = color(0x131315);
    s.Colors[ImGuiCol_Border] = color(0x303035);
    s.Colors[ImGuiCol_TitleBg] = color(0x0a0a0b);
    s.Colors[ImGuiCol_TitleBgActive] = color(0x1e1e21);
    s.Colors[ImGuiCol_TitleBgCollapsed] = color(0x131315);
    s.Colors[ImGuiCol_FrameBg] = color(0x1e1e21);
    s.Colors[ImGuiCol_FrameBgHovered] = color(0x35353b);
    s.Colors[ImGuiCol_FrameBgActive] = color(0x49494f);
    s.Colors[ImGuiCol_Button] = color(0x1e1e21);
    s.Colors[ImGuiCol_ButtonHovered] = color(0x493036);
    s.Colors[ImGuiCol_ButtonActive] = color(0xe01b24);
    s.Colors[ImGuiCol_Header] = color(0x5c2229);
    s.Colors[ImGuiCol_HeaderHovered] = color(0x783039);
    s.Colors[ImGuiCol_HeaderActive] = color(0xe01b24);
    s.Colors[ImGuiCol_CheckMark] = color(0xe01b24);
    s.Colors[ImGuiCol_SliderGrab] = color(0xe01b24);
    s.Colors[ImGuiCol_SliderGrabActive] = color(0xf2444d);
    s.Colors[ImGuiCol_Separator] = color(0x303035);
    s.Colors[ImGuiCol_SeparatorHovered] = color(0xe01b24);
    s.Colors[ImGuiCol_SeparatorActive] = color(0xe01b24);
    s.Colors[ImGuiCol_ResizeGrip] = color(0x49494f, .6f);
    s.Colors[ImGuiCol_ResizeGripHovered] = color(0xe01b24, .7f);
    s.Colors[ImGuiCol_ResizeGripActive] = color(0xe01b24);
    s.Colors[ImGuiCol_ScrollbarBg] = color(0x0a0a0b);
    s.Colors[ImGuiCol_ScrollbarGrab] = color(0x35353b);
    s.Colors[ImGuiCol_ScrollbarGrabHovered] = color(0x49494f);
    s.Colors[ImGuiCol_ScrollbarGrabActive] = color(0xe01b24);
    s.Colors[ImGuiCol_TextSelectedBg] = color(0xe01b24, .35f);
    s.Colors[ImGuiCol_NavHighlight] = color(0xe01b24);
    s.Colors[ImGuiCol_ModalWindowDimBg] = color(0x0a0a0b, .7f);
}
// The same shell primitives are exercised by the standalone native UI fixture.
inline bool begin_shell(const Layout &shell, const char *shortcut, bool &show) {
    ImGui::SetNextWindowPos(ImVec2(shell.x, shell.y));
    ImGui::SetNextWindowSize(ImVec2(shell.width, shell.height));
    const bool visible = ImGui::Begin("Feraxia###FeraxiaOverlay", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus);
    if (visible) {
        ImGui::TextColored(color(0xe01b24), "FERAXIA");
        const float return_width = ImGui::CalcTextSize(shortcut).x + ImGui::GetStyle().FramePadding.x * 2.f;
        if (ImGui::GetContentRegionAvail().x > return_width + ImGui::GetFontSize() * 8.f)
            ImGui::SameLine(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - return_width);
        if (ImGui::Button(shortcut, ImVec2((std::max)(1.f, (std::min)(return_width, ImGui::GetContentRegionAvail().x)), 0))) show = false;
        ImGui::Separator();
    }
    return visible;
}
inline void begin_navigation(const Layout &shell) {
    const float compact_height = (std::max)(1.f, ImGui::GetContentRegionAvail().y * .4f);
    ImGui::BeginChild("##feraxia_navigation",
        shell.wide ? ImVec2(shell.rail_width, 0) : ImVec2(0, compact_height), true);
}
inline void begin_content(const Layout &shell) {
    ImGui::EndChild();
    if (shell.wide) ImGui::SameLine();
    ImGui::BeginChild("##feraxia_content", ImVec2(0, 0), true);
}
inline void end_content(const Layout &) {
    ImGui::EndChild();
}
inline bool navigation_button(const char *label, bool selected = false) {
    if (selected) ImGui::PushStyleColor(ImGuiCol_Button, color(0x81222b));
    const bool pressed = ImGui::Button(label, ImVec2((ImGui::GetContentRegionAvail().x > 1.f ? ImGui::GetContentRegionAvail().x : 1.f), 0));
    if (selected) ImGui::PopStyleColor();
    return pressed;
}
inline bool begin_auxiliary(const char *name, bool *open = nullptr, ImGuiWindowFlags flags = 0) {
    // Begin emits decorations and clip rectangles. Clamp beforehand so the current
    // draw frame, hit testing and returned position all agree after a resize/drag.
    // The pinned renderer installs imgui_internal.h; no extra per-window cache.
    ImGuiContext &context = *ImGui::GetCurrentContext();
    if (ImGuiWindow *window = ImGui::FindWindowByName(name)) {
        if (window->LastFrameActive == context.FrameCount - 1) {
            ImVec2 expected = window->SizeFull;
            const auto &next = context.NextWindowData;
            if (next.HasFlags & ImGuiNextWindowDataFlags_HasSizeConstraint) {
                const ImRect &bounds = next.SizeConstraintRect;
                expected.x = (std::max)(bounds.Min.x, (std::min)(expected.x, bounds.Max.x));
                expected.y = (std::max)(bounds.Min.y, (std::min)(expected.y, bounds.Max.y));
            }
            expected.x = (std::max)(expected.x, context.Style.WindowMinSize.x);
            expected.y = (std::max)(expected.y, context.Style.WindowMinSize.y);
            bool collapsed = window->Collapsed;
            if ((next.HasFlags & ImGuiNextWindowDataFlags_HasCollapsed) &&
                (next.CollapsedCond == ImGuiCond_Always || next.CollapsedCond == 0)) collapsed = next.CollapsedVal;
            if (collapsed) expected.y = ImGui::GetFontSize() + context.Style.FramePadding.y * 2.f;
            const ImVec2 display = context.IO.DisplaySize;
            const ImVec2 bounded((std::max)(0.f, (std::min)(window->Pos.x, display.x - expected.x)),
                (std::max)(0.f, (std::min)(window->Pos.y, display.y - expected.y)));
            if (bounded.x != window->Pos.x || bounded.y != window->Pos.y)
                ImGui::SetNextWindowPos(bounded, ImGuiCond_Always);
        }
    }
    return ImGui::Begin(name, open, flags);
}
// Keep all auxiliary windows inside the current viewport after a resolution change.
inline void auxiliary_window(float width, float height) {
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float w = (std::max)(1.f, (std::min)(width, display.x * .92f));
    const float h = (std::max)(1.f, (std::min)(height, display.y * .92f));
    ImGui::SetNextWindowSizeConstraints(ImVec2((std::min)(w, 180.f), (std::min)(h, 120.f)), ImVec2(w, h));
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(display.x * .5f, display.y * .5f), ImGuiCond_Appearing, ImVec2(.5f, .5f));
}
}
