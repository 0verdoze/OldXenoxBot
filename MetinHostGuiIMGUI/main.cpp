#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <d3d11.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>
#include <thread>

#include "MetinGui.h"
#include "resource.h"

#include <stdio.h>
#include <boost/lexical_cast.hpp>

#pragma comment(lib, "d3d11.lib")

// Used for ImGui window
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int main(int, char**)
{
    // if PUBLIC_RELEASE is defined, HostGui and GuestManager(MetinGuestGui in source tree) are merged to a single application
    // to be more specific, compiled exe file of the previous will be embedded to final application
    // and we will launch it here
#ifdef PUBLIC_RELEASE
    const WCHAR lock_file_name[] = L"MetinBotLOCKFILE";
    const WCHAR guest_mgr_name[] = L"MetinBotMGR.exe";

    WCHAR lock_file_path[MAX_PATH]{ };
    WCHAR guest_manager_file_path[MAX_PATH]{ };
    // get path to %TEMP%, lock_file_path will point to it
    DWORD size = GetTempPathW(MAX_PATH, lock_file_path);

    // set guest_manager_file_path to %TEMP%
    memcpy(guest_manager_file_path, lock_file_path, size * sizeof(WCHAR));

    // append file names to both lock_file_path and guest_manager_file_path
    memcpy(lock_file_path + size, lock_file_name, sizeof(lock_file_name));
    memcpy(guest_manager_file_path + size, guest_mgr_name, sizeof(guest_mgr_name));

    // create lock file
    HANDLE hLock = CreateFileW(lock_file_path, (GENERIC_READ | GENERIC_WRITE), FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
    // create GuestManager exe file
    HANDLE hMgr = CreateFileW(guest_manager_file_path, (GENERIC_READ | GENERIC_WRITE), 0, NULL, CREATE_ALWAYS, NULL, NULL);
    if (hLock == INVALID_HANDLE_VALUE || hMgr == INVALID_HANDLE_VALUE)
    {
        MessageBoxExA(NULL, "One instance of this program is already running on this computer", "Info", MB_OK, NULL);
        return 1;
    }

    // find GuestManager exe file in resources
    auto hModule = GetModuleHandle(NULL);
    auto hResInfo = FindResourceEx(hModule, TEXT("EXE"), MAKEINTRESOURCE(IDR_EXE1), MAKELANGID(LANG_POLISH, SUBLANG_DEFAULT));
    if (hResInfo == nullptr)
    {
        MessageBoxExA(NULL, "Unable to find GuestManager resource", "Info", MB_OK, NULL);
        return 1;
    }
    
    auto hRes = LoadResource(hModule, hResInfo); 
    if (hRes == nullptr)
    {
        MessageBoxExA(NULL, "Unable to load GuestManager resource", "Info", MB_OK, NULL);
        return 1;
    }

    // save GuestManager to disk
    DWORD bytesWritten;
    if (!WriteFile(hMgr, hRes, SizeofResource(hModule, hResInfo), &bytesWritten, NULL))
    {
        MessageBoxExA(NULL, "Unable to write GuestManager to file", "Info", MB_OK, NULL);
        return 1;
    }
    
    CloseHandle(hMgr);

    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION processInfo;

    auto test = boost::lexical_cast<std::array<WCHAR, 21>>(GetCurrentProcessId());

    // start GuestManager process
    if (!CreateProcessW(guest_manager_file_path, test.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &processInfo))
    {
        MessageBoxExA(NULL, "Unable to create GuestManager process", "Info", MB_OK, NULL);
        return 2;
    }
#endif

    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Metin Host ImGui"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // instruct to load ASCII and polish letters
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF,
        0x0100, 0x017F,
        0,
    };

    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\Arial.ttf", 14, nullptr, ranges);
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        // Standard windows message polling and handling (inputs, window resize, etc.)
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

#ifdef PUBLIC_RELEASE
        // check if GuestManager has died (this should never happen)
        {
            DWORD exitCode;
            if (GetExitCodeProcess(processInfo.hProcess, &exitCode) && exitCode != STILL_ACTIVE)
            {
                // it died, show message and exit program
                MessageBoxA(hwnd, "Underlying process has died, ui will close now", "Closing", MB_OK | MB_ICONERROR);
                return 1;
            }
        }
#endif

        // if window is minimized (iconic) we will skip drawing next frame
        if (IsIconic(hwnd))
        {
            Sleep(50);
            continue;
        }
        
        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        GuiRun(hwnd);

        // Rendering
        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*)&clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
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
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
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
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
