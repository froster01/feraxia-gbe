#include "imgui.h"
#include "imgui_internal.h"
#include "../../overlay_experimental/overlay/feraxia_theme.h"
#include <cstdio>
#include <cmath>
int main() {
    int failures = 0;
    auto check = [&](bool condition, const char *message) {
        if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); ++failures; }
    };
    for (ImVec2 display : {ImVec2(320, 240), ImVec2(640, 480), ImVec2(1280, 720), ImVec2(3840, 2160)}) {
        for (float scale : {1.f, 1.5f, 3.f}) {
            ImGui::CreateContext();
            auto &io = ImGui::GetIO();
            io.IniFilename = nullptr;
            io.DisplaySize = display;
            io.DeltaTime = 1.f / 60.f;
            io.FontGlobalScale = scale;
            unsigned char *pixels; int width, height;
            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
            feraxia::apply_theme(ImGui::GetStyle());
            check(std::abs(ImGui::GetStyle().Colors[ImGuiCol_Text].x - 201.f / 255.f) < .001f, "titanium text palette");
            check(std::abs(ImGui::GetStyle().Colors[ImGuiCol_ButtonActive].x - 224.f / 255.f) < .001f, "crimson active palette");
            for (int frame = 0; frame < 3; ++frame) {
                ImGui::NewFrame();
                const auto l = feraxia::layout(display.x, display.y, ImGui::GetFontSize());
                bool show = true;
                if (feraxia::begin_shell(l, "Shift + Tab  /  Close", show)) {
                    feraxia::begin_navigation(l);
                    feraxia::navigation_button("Achievements", true);
                    feraxia::navigation_button("Screenshots");
                    feraxia::navigation_button("Settings");
                    feraxia::begin_content(l);
                    ImGui::Text("Friends / invitations");
                    ImGui::TextWrapped("No peers connected. Native presentation fixture; no networking simulated.");
                    feraxia::end_content(l);
                }
                ImGui::End();
                for (auto *window : ImGui::GetCurrentContext()->Windows) {
                    if (!window->Active || window->IsFallbackWindow) continue;
                    check(window->Size.x > 0 && window->Size.y > 0, "native window/child sizes positive");
                    check(window->Pos.x >= -.01f && window->Pos.y >= -.01f, "native window origin fits viewport");
                    check(window->Pos.x + window->Size.x <= display.x + .01f && window->Pos.y + window->Size.y <= display.y + .01f, "native shell and child bounds fit viewport");
                }
                ImGui::Render();
                check(ImGui::GetDrawData()->Valid, "native draw frame is valid");
                check(ImGui::GetDrawData()->TotalVtxCount > 0, "native shell emitted vertices");
            }
            ImGui::DestroyContext();
        }
    }
    ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DeltaTime = 1.f / 60.f;
    unsigned char *pixels; int atlas_width, atlas_height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &atlas_width, &atlas_height);
    for (int frame = 0; frame < 5; ++frame) {
        io.DisplaySize = frame == 0 ? ImVec2(1920, 1080) : ImVec2(640, 480);
        io.FontGlobalScale = frame == 0 ? 1.f : 2.f;
        ImGui::NewFrame();
        if (frame == 2) ImGui::SetWindowPos("Persistent auxiliary", ImVec2(20, 20));
        if (frame == 3) ImGui::SetWindowPos("Persistent auxiliary", ImVec2(630, 460));
        if (frame == 4) ImGui::SetNextWindowCollapsed(true);
        feraxia::auxiliary_window(900, 700);
        if (frame == 0) ImGui::SetNextWindowPos(ImVec2(1000, 350));
        const bool expanded = feraxia::begin_auxiliary("Persistent auxiliary");
        const auto pos = ImGui::GetWindowPos();
        const auto size = ImGui::GetWindowSize();
        check(pos.x >= 0 && pos.y >= 0 && pos.x + size.x <= io.DisplaySize.x + .01f && pos.y + size.y <= io.DisplaySize.y + .01f,
            "persistent auxiliary remains entirely inside resized viewport");
        if (frame == 2) check(pos.x == 20 && pos.y == 20, "in-bounds user drag remains unchanged");
        auto *aux = ImGui::GetCurrentWindow();
        if (expanded) check(aux->InnerRect.Min.x >= pos.x - .01f && aux->InnerRect.Min.y >= pos.y - .01f &&
            aux->InnerRect.Max.x <= pos.x + size.x + .01f && aux->InnerRect.Max.y <= pos.y + size.y + .01f,
            "auxiliary inner rectangle follows clamped origin on the same frame");
        if (expanded) check(aux->InnerClipRect.Min.x >= pos.x - .01f && aux->InnerClipRect.Max.x <= pos.x + size.x + .01f,
            "auxiliary content clip follows clamped origin on the same frame");
        if (expanded) ImGui::TextUnformatted("Auxiliary content");
        ImGui::End();
        bool decorations_fit = true;
        for (const auto &vertex : aux->DrawList->VtxBuffer)
            if (vertex.pos.x < pos.x - 2.f || vertex.pos.x > pos.x + size.x + 2.f ||
                vertex.pos.y < pos.y - 2.f || vertex.pos.y > pos.y + size.y + 2.f) decorations_fit = false;
        check(decorations_fit, "auxiliary decorations and content draw at the clamped origin");
        ImGui::Render();
    }
    ImGui::DestroyContext();
    std::printf("%s: real ImGui shell frames at 12 viewport/font combinations\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
