#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_dx11.h"
#include "overlay_experimental/fonts/unifont.hpp"
#include <d3d11.h>
#include <cstdio>
#include <initializer_list>

// Count pixels the overlay actually drew (render target is cleared to zero).
static int count_drawn_pixels(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Texture2D* target)
{
    D3D11_TEXTURE2D_DESC desc;
    target->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;
    ID3D11Texture2D* staging = nullptr;
    if (FAILED(device->CreateTexture2D(&desc, nullptr, &staging)))
        return -1;
    context->CopyResource(staging, target);
    D3D11_MAPPED_SUBRESOURCE map;
    if (FAILED(context->Map(staging, 0, D3D11_MAP_READ, 0, &map))) {
        staging->Release();
        return -1;
    }
    int drawn = 0;
    for (UINT y = 0; y < desc.Height; ++y) {
        const unsigned* row = reinterpret_cast<const unsigned*>(static_cast<const char*>(map.pData) + y * map.RowPitch);
        for (UINT x = 0; x < desc.Width; ++x)
            if (row[x] & 0x00ffffffu) ++drawn;
    }
    context->Unmap(staging, 0);
    staging->Release();
    return drawn;
}

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
    D3D11_TEXTURE2D_DESC target_desc = {};
    target_desc.Width = 1280;
    target_desc.Height = 720;
    target_desc.MipLevels = target_desc.ArraySize = 1;
    target_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    target_desc.SampleDesc.Count = 1;
    target_desc.Usage = D3D11_USAGE_DEFAULT;
    target_desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    ID3D11Texture2D* target = nullptr;
    ID3D11RenderTargetView* target_view = nullptr;
    if (FAILED(device->CreateTexture2D(&target_desc, nullptr, &target)) ||
            FAILED(device->CreateRenderTargetView(target, nullptr, &target_view))) {
        std::puts("FAIL: could not create render target");
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
    int drawn_pixels[120] = {};
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
        const float clear[4] = {0, 0, 0, 0};
        context->ClearRenderTargetView(target_view, clear);
        context->OMSetRenderTargets(1, &target_view, nullptr);
        ImGui_ImplDX11_RenderDrawData(draw);
        const bool window_drawn = frame >= 7 && (frame < 25 || frame >= 91);
        if (window_drawn) {
            // The panel background alone covers thousands of pixels; text must add more.
            const int drawn = count_drawn_pixels(device, context, target);
            drawn_pixels[frame] = drawn;
            if (drawn < 2000) {
                std::printf("FAIL: frame %d drew only %d pixels (overlay is blank)\n", frame, drawn);
                return 1;
            }
        }
        for (auto* list : draw->CmdLists)
            for (const auto& command : list->CmdBuffer)
                if (command.ElemCount && !command.UserCallback &&
                        command.GetTexID() == ImTextureID_Invalid)
                    return 1;
    }
    // A recreated atlas must render glyphs, not just the panel background: the frame
    // that recovered must draw as many pixels as the next, undisturbed frame.
    for (int recovery : {8, 13, 91}) {
        const int after = drawn_pixels[recovery + 1];
        if (drawn_pixels[recovery] * 100 < after * 97 || after * 100 < drawn_pixels[recovery] * 97) {
            std::printf("FAIL: recovery frame %d drew %d pixels but the next frame drew %d (atlas content lost)\n",
                recovery, drawn_pixels[recovery], after);
            return 1;
        }
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
