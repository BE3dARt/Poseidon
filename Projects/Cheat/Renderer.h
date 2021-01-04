#pragma once

#include "cheat.h"
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_impl_win32.h>
#include <HookLib/HookLib.h>


void Cheat::Renderer::Drawing::RenderText(const char* text, const FVector2D& pos, const ImVec4& color, const bool outlined = true, const bool centered = true)
{
    if (!text) return;
    auto ImScreen = *reinterpret_cast<const ImVec2*>(&pos);
    if (centered)
    {
        auto size = ImGui::CalcTextSize(text);
        ImScreen.x -= size.x * 0.5f;
        ImScreen.y -= size.y;
    }
    auto window = ImGui::GetCurrentWindow();

    if (outlined) { window->DrawList->AddText(nullptr, 0.f, ImVec2(ImScreen.x - 1.f, ImScreen.y + 1.f), ImGui::GetColorU32(IM_COL32_BLACK), text); }

    window->DrawList->AddText(nullptr, 0.f, ImScreen, ImGui::GetColorU32(color), text);

}

void Cheat::Renderer::Drawing::Render2DBox(const FVector2D& top, const FVector2D& bottom, const float height, const float width, const ImVec4& color)
{
    ImGui::GetCurrentWindow()->DrawList->AddRect({ top.X - width * 0.5f, top.Y }, { top.X + width * 0.5f, bottom.Y }, ImGui::GetColorU32(color), 0.f, 15, 1.5f);
}

bool Cheat::Renderer::Drawing::Render3DBox(AController* const controller, const FVector& origin, const FVector& extent, const FRotator& rotation, const ImVec4& color)
{
    FVector vertex[2][4];
    vertex[0][0] = { -extent.X, -extent.Y,  -extent.Z };
    vertex[0][1] = { extent.X, -extent.Y,  -extent.Z };
    vertex[0][2] = { extent.X, extent.Y,  -extent.Z };
    vertex[0][3] = { -extent.X, extent.Y, -extent.Z };

    vertex[1][0] = { -extent.X, -extent.Y, extent.Z };
    vertex[1][1] = { extent.X, -extent.Y, extent.Z };
    vertex[1][2] = { extent.X, extent.Y, extent.Z };
    vertex[1][3] = { -extent.X, extent.Y, extent.Z };

    FVector2D screen[2][4];
    FTransform const Transform(rotation);
    for (auto k = 0; k < 2; k++)
    {
        for (auto i = 0; i < 4; i++)
        {
            auto& vec = vertex[k][i];
            vec = Transform.TransformPosition(vec) + origin;
            if (!controller->ProjectWorldLocationToScreen(vec, screen[k][i])) return false;
        }

    }

    auto ImScreen = reinterpret_cast<ImVec2(&)[2][4]>(screen);

    auto window = ImGui::GetCurrentWindow();
    for (auto i = 0; i < 4; i++)
    {
        window->DrawList->AddLine(ImScreen[0][i], ImScreen[0][(i + 1) % 4], ImGui::GetColorU32(color));
        window->DrawList->AddLine(ImScreen[1][i], ImScreen[1][(i + 1) % 4], ImGui::GetColorU32(color));
        window->DrawList->AddLine(ImScreen[0][i], ImScreen[1][i], ImGui::GetColorU32(color));
    }

    return true;
}

bool Cheat::Renderer::Drawing::RenderSkeleton(AController* const controller, USkeletalMeshComponent* const mesh, const FMatrix& comp2world, const std::pair<const BYTE*, const BYTE>* skeleton, int size, const ImVec4& color)
{

    for (auto s = 0; s < size; s++)
    {
        auto& bone = skeleton[s];
        FVector2D previousBone;
        for (auto i = 0; i < skeleton[s].second; i++)
        {
            FVector loc;
            if (!mesh->GetBone(bone.first[i], comp2world, loc)) return false;
            FVector2D screen;
            if (!controller->ProjectWorldLocationToScreen(loc, screen)) return false;
            if (previousBone.Size() != 0) {
                auto ImScreen1 = *reinterpret_cast<ImVec2*>(&previousBone);
                auto ImScreen2 = *reinterpret_cast<ImVec2*>(&screen);
                ImGui::GetCurrentWindow()->DrawList->AddLine(ImScreen1, ImScreen2, ImGui::GetColorU32(color));
            }
            previousBone = screen;
        }
    }

    return true;
}


extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


LRESULT WINAPI Cheat::Renderer::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam) && bIsOpen) return true;
    if (bIsOpen)
    {
        ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
        LPTSTR win32_cursor = IDC_ARROW;
        switch (imgui_cursor)
        {
        case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
        case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
        case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
        case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
        case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
        case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
        case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
        case ImGuiMouseCursor_Hand:         win32_cursor = IDC_HAND; break;
        case ImGuiMouseCursor_NotAllowed:   win32_cursor = IDC_NO; break;
        }
        SetCursorOriginal(LoadCursorA(nullptr, win32_cursor));

    }
    if (!bIsOpen || uMsg == WM_KEYUP) return CallWindowProcA(WndProcOriginal, hwnd, uMsg, wParam, lParam);
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

HCURSOR WINAPI Cheat::Renderer::SetCursorHook(HCURSOR hCursor)
{
    if (bIsOpen) return 0;
    return SetCursorOriginal(hCursor);
}

BOOL WINAPI Cheat::Renderer::SetCursorPosHook(int X, int Y)
{
    if (bIsOpen) return FALSE;
    return SetCursorPosOriginal(X, Y);
}

void Cheat::Renderer::HookInput()
{
    RemoveInput();
    WndProcOriginal = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(gameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));
    Logger::Log("WndProcOriginal = %p\n", WndProcOriginal);
}

void Cheat::Renderer::RemoveInput()
{
    if (WndProcOriginal)
    {
        SetWindowLongPtrA(gameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcOriginal));
        WndProcOriginal = nullptr;
    }
}

inline bool Cheat::Renderer::Init()
{
    HMODULE dxgi = GetModuleHandleA("dxgi.dll");
    Logger::Log("dxgi: %p\n", dxgi);
    static BYTE PresentSig[] = { 0x55, 0x57, 0x41, 0x56, 0x48, 0x8d, 0x6c, 0x24, 0x90, 0x48, 0x81, 0xec, 0x70, 0x01 };
    //static BYTE PresentHead[] = { 0x48, 0x89, 0x5c, 0x24, 0x10 };
    //BYTE* fnPresent = Tools::PacthFn(dxgi, PresentSig, sizeof(PresentSig), PresentHead, sizeof(PresentHead));
    fnPresent = reinterpret_cast<decltype(fnPresent)>(Tools::FindFn(dxgi, PresentSig, sizeof(PresentSig)));
    Logger::Log("IDXGISwapChain::Present: %p\n", fnPresent);
    if (!fnPresent) return false;


    static BYTE ResizeSig[] = { 0x48, 0x81, 0xec, 0xc0, 0x00, 0x00, 0x00, 0x48, 0xc7, 0x45, 0x1f };
    //static BYTE ResizeHead[] = { 0x48, 0x8b, 0xc4, 0x55, 0x41, 0x54 };  
    //BYTE* fnResize = Tools::PacthFn(dxgi, ResizeSig, sizeof(ResizeSig), ResizeHead, sizeof(ResizeHead));
    fnResize = reinterpret_cast<decltype(fnResize)>(Tools::FindFn(dxgi, ResizeSig, sizeof(ResizeSig)));
    Logger::Log("IDXGISwapChain::ResizeBuffers: %p\n", fnResize);
    if (!fnResize) return false;


    if (!SetHook(fnPresent, PresentHook, reinterpret_cast<void**>(&PresentOriginal)))
    {
        return false;
    };

    Logger::Log("PresentHook: %p\n", PresentHook);
    Logger::Log("PresentOriginal: %p\n", PresentOriginal);

    if (!SetHook(fnResize, ResizeHook, reinterpret_cast<void**>(&ResizeOriginal)))
    {
        return false;
    };

    Logger::Log("ResizeHook: %p\n", ResizeHook);
    Logger::Log("ResizeOriginal: %p\n", ResizeOriginal);

    if (!SetHook(SetCursorPos, SetCursorPosHook, reinterpret_cast<void**>(&SetCursorPosOriginal)))
    {
        Logger::Log("Can't hook SetCursorPos\n");
        return false;
    };

    if (!SetHook(SetCursor, SetCursorHook, reinterpret_cast<void**>(&SetCursorOriginal)))
    {
        Logger::Log("Can't hook SetCursor\n");
        return false;
    };

    return true;
}

inline bool Cheat::Renderer::Remove()
{
    Renderer::RemoveInput();
    if (!RemoveHook(PresentOriginal) || !RemoveHook(ResizeOriginal) || !RemoveHook(SetCursorPosOriginal) || !RemoveHook(SetCursorOriginal))
    {
        return false;
    }
    if (renderTargetView)
    {
        ImGui_ImplDX11_Shutdown();
        ImGui::DestroyContext();
        renderTargetView->Release();
        renderTargetView = nullptr;
    }
    if (context)
    {
        context->Release();
        context = nullptr;
    }
    if (device)
    {
        device->Release();
        device = nullptr;
    }
    return true;
}
