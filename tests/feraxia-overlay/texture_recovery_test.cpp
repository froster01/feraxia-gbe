#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_dx11.h"
#include "overlay_experimental/fonts/unifont.hpp"
#include <d3d11.h>
#include <cstdio>

// Exercise the real renderer: invalidating an already-prepared frame must
// request texture recreation before GetTexID(), without another NewFrame().
int main()
{
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context))) {
        std::puts("FAIL: could not create DX11 WARP device");
        return 2;
    }
    ImFontAtlas atlas;
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    config.OversampleH = config.OversampleV = 1;
    ImFont* fonts[4];
    for (int i = 0; i < 4; ++i)
        fonts[i] = atlas.AddFontFromMemoryCompressedTTF(
            unifont_compressed_data, unifont_compressed_size, 16.0f + 2.0f * i, &config);
    ImGui::CreateContext(&atlas);
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(1280, 720);
    io.DeltaTime = 1.0f / 60.0f;
    if (!ImGui_ImplDX11_Init(device, context))
        return 2;
    int recovered = 0;
    for (int frame = 0; frame < 120; ++frame) {
        ImGui_ImplDX11_NewFrame();
        ImFontAtlasUpdateNewFrame(&atlas, ImGui::GetFrameCount(),
            (io.BackendFlags & ImGuiBackendFlags_RendererHasTextures) != 0);
        ImGui::NewFrame();
        if (frame >= 5 && (frame < 25 || frame >= 90)) {
            ImGui::Begin("Friends and invite texture recovery");
            for (auto* font : fonts) {
                ImGui::PushFont(font);
                ImGui::TextUnformatted("Feraxia Friends Invite Join Accept Decline");
                if (frame >= 10)
                    for (unsigned cp = 0x4e00; cp < 0x4e00 + 800; ++cp) {
                        char text[5] = {};
                        ImTextCharToUtf8(text, cp);
                        ImGui::TextUnformatted(text);
                    }
                ImGui::PopFont();
            }
            ImGui::End();
        }
        ImGui::Render();
        auto* draw = ImGui::GetDrawData();
        if (frame == 8 || frame == 13 || frame == 91) {
            ImGui_ImplDX11_InvalidateDeviceObjects();
            // Repeated invalidation must also remain safe.
            ImGui_ImplDX11_InvalidateDeviceObjects();
            if (!ImGui_ImplDX11_CreateDeviceObjects())
                return 2;
            int invalidated_commands = 0;
            for (auto* list : draw->CmdLists)
                for (const auto& command : list->CmdBuffer) {
                    const auto* texture = command.TexRef._TexData;
                    if (command.ElemCount && !command.UserCallback && texture &&
                            texture->TexID == ImTextureID_Invalid)
                        ++invalidated_commands;
                    if (command.ElemCount && !command.UserCallback && texture &&
                            texture->TexID == ImTextureID_Invalid &&
                            texture->Status != ImTextureStatus_WantCreate) {
                        std::printf("FAIL: frame %d live texture has invalid ID and no recreation request (status=%d)\n",
                            frame, static_cast<int>(texture->Status));
                        return 1;
                    }
                }
            if (invalidated_commands == 0) {
                std::puts("FAIL: recovery fixture did not invalidate a live draw texture");
                return 1;
            }
            ++recovered;
        }
        ImGui_ImplDX11_RenderDrawData(draw);
        for (auto* list : draw->CmdLists)
            for (const auto& command : list->CmdBuffer)
                if (command.ElemCount && !command.UserCallback &&
                        command.GetTexID() == ImTextureID_Invalid)
                    return 1;
    }
    ImTextureData retired;
    retired.WantDestroyNextFrame = true;
    retired.SetStatus(ImTextureStatus_Destroyed);
    if (retired.Status != ImTextureStatus_Destroyed) {
        std::puts("FAIL: intentionally retired texture was recreated");
        return 1;
    }
    ImGui_ImplDX11_Shutdown();
    ImGui::DestroyContext();
    context->Release();
    device->Release();
    std::printf("PASS: 120 DX11 WARP frames, %d same-frame texture recoveries, atlas growth, hidden/reopened panel and intentional retirement\n", recovered);
    return 0;
}
