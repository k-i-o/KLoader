#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include "vendors/imgui/imgui.h"
#include "vendors/imgui/backends/imgui_impl_win32.h"
#include "vendors/imgui/backends/imgui_impl_dx11.h"
#include <thread>
#include <chrono>
#include <tchar.h>  // Include per TEXT macro

#include <iostream>

#include <filesystem>
#include "injector/config.hpp"
#include "injector/util.h"
#include "injector/inject.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
IDXGISwapChain* pSwapChain = NULL;
ID3D11RenderTargetView* pRenderTargetView = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void RenderWindow()
{
    bool menuOpen = true;
    if (ImGui::Begin("Main Window", &menuOpen)) {
        ImGui::Text("froio");
    }
    ImGui::End();
}

static Config config;

bool OpenGame(HANDLE* phProcess, HANDLE* phThread)
{
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};

    auto procName = config.GamePath.substr(config.GamePath.find_last_of("\\") + 1);
    auto cmdLine = config.GamePath + " " + config.LaunchOptions;

    if (!CreateProcess(config.GamePath.c_str(), (LPSTR)cmdLine.c_str(),
        nullptr, nullptr, FALSE,
        CREATE_SUSPENDED, nullptr, nullptr, &si, &pi))
    {
        std::cerr << "CreateProcess failed: " << util::GetLastErrorAsString() << std::endl;
        return false;
    }

    *phProcess = pi.hProcess;
    *phThread = pi.hThread;

    return true;
}

void RenderLogin()
{
    bool menuOpen = true;
    if (ImGui::Begin("Login Window", &menuOpen)) {
        if (ImGui::Button("Inject and execute game")) {

            std::string configPath = "config.ini";
            if (!std::filesystem::exists(configPath) || !config.Load(configPath.c_str()))
            {
                auto gamePathOpt = util::SelectFile("Executable Files (*.exe)\0*.exe\0", "Select the game executable");
                if (gamePathOpt)
                {
                    config.GamePath = gamePathOpt.value();
                    std::cout << "Game Path Selected: " << config.GamePath << std::endl;
                }

                auto dllPathOpt = util::SelectFile("DLL Files (*.dll)\0*.dll\0", "Select the first DLL to inject");
                if (dllPathOpt)
                {
                    config.DLLPath_1 = dllPathOpt.value();
                    std::cout << "DLL Path 1 Selected: " << config.DLLPath_1 << std::endl;
                }

                config.Save(configPath.c_str());
            }
            else
            {
                std::cout << "Config loaded successfully" << std::endl;
                std::cout << "Path to game: " << config.GamePath << std::endl;
            }

            HANDLE hProcess, hThread;
            if (!OpenGame(&hProcess, &hThread))
            {
                std::cerr << "Failed to open process" << std::endl;
                system("pause");
                return;
            }

            if (!config.DLLPath_1.empty())
            {
                auto shuffledPath = util::ShuffleDllName(config.DLLPath_1);
                std::cout << "Shuffled DLL 1 path: " << shuffledPath << std::endl;
#ifdef USE_MANUAL_MAP
                Inject(hProcess, config.DLLPath_1, InjectionType::ManualMap);
#else
                Inject(hProcess, config.DLLPath_1);
#endif
            }

            if (!config.DLLPath_2.empty())
            {
#ifdef USE_MANUAL_MAP
                Inject(hProcess, config.DLLPath_2, InjectionType::ManualMap);
#else
                Inject(hProcess, config.DLLPath_2);
#endif
            }

            if (!config.DLLPath_3.empty())
            {
#ifdef USE_MANUAL_MAP
                Inject(hProcess, config.DLLPath_3, InjectionType::ManualMap);
#else
                Inject(hProcess, config.DLLPath_3);
#endif
            }
        }
    }
    ImGui::End();
}

int main(int, char**)
{
    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, TEXT("ImGui Example"), NULL };
    RegisterClassEx(&wc);
    window = CreateWindow(wc.lpszClassName, TEXT("Dear ImGui DirectX11 Example"), WS_POPUP, 0, 0, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // Enable window transparency
    SetWindowLong(window, GWL_EXSTYLE, GetWindowLong(window, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(window, RGB(0, 0, 0), 0, LWA_COLORKEY);

    // Initialize Direct3D
    if (!CreateDeviceD3D(window))
    {
        CleanupDeviceD3D();
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Set the window to full screen
    SetWindowLong(window, GWL_STYLE, 0);
    ShowWindow(window, SW_SHOWDEFAULT);
    SetWindowPos(window, HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);

    UpdateWindow(window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(pDevice, pContext);

    // Main loop
    MSG msg;
    while (true)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                break;
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderLogin();
        //RenderWindow();

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // Set alpha to 0 for transparency
        pContext->OMSetRenderTargets(1, &pRenderTargetView, NULL);
        pContext->ClearRenderTargetView(pRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        pSwapChain->Present(1, 0); // Present with vsync

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(window);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2,
        D3D11_SDK_VERSION, &sd, &pSwapChain, &pDevice, &featureLevel, &pContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (pSwapChain) { pSwapChain->Release(); pSwapChain = NULL; }
    if (pContext) { pContext->Release(); pContext = NULL; }
    if (pDevice) { pDevice->Release(); pDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    pDevice->CreateRenderTargetView(pBackBuffer, NULL, &pRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (pRenderTargetView) { pRenderTargetView->Release(); pRenderTargetView = NULL; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (pDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
