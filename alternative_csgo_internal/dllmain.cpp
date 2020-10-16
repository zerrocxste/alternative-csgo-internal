#include "includes.h"
#include "offsets_parser/offsets_parser.h"
#include "math/qangle.h"
#include "math/vector.h"
#include "math/matrix4x4.h"

std::unique_ptr<COffsets>m_pOffsets = std::make_unique<COffsets>();

using fEndScene = HRESULT(WINAPI*)(LPDIRECT3DDEVICE9);
using fSetCursorPos = BOOL(WINAPI*)(int, int);

fEndScene pEndScene;
fSetCursorPos pSetCursorPos;

bool menu_open = false;
WNDPROC wndprocOriginal;
HWND hCSGO;

bool unhook = false;

bool rcs = false;
bool esp = false;
bool esp_teammates = false;
bool glow_esp = false;
bool glow_esp_teammates = false;

enum GAMESTATE
{
    STATE_NONE,
    CHALLENGE,
    CONNECTED,
    NEW,
    PRESPAWN,
    SPAWN,
    FULL_CONNECTED,
    CHANGELEVEL
};

struct local_player_s
{
    Matrix4x4* vMatrix;
    QAngle* QMyAngle;
    DWORD dwLocalPlayer;
    DWORD dwClientState;
    QAngle* QPunchAngle;
    int* iShortFired;
    int* iHealth;
    int* iTeam;
    int* iGameState;
};
local_player_s g_local;

struct players_s
{
    DWORD dwPlayerEntity;
    int iTeam;
    int iHealth;
    bool iDormant;
    Vector vOrigin;
};
players_s g_players[64];

namespace console
{
    void attach(LPCSTR pszConsoleName)
    {
        int hConHandle = 0;
        HANDLE lStdHandle = 0;
        FILE* fp = 0;
        AllocConsole();
        freopen("CON", "w", stdout);
        SetConsoleTitle(pszConsoleName);
        HWND hwnd = ::GetConsoleWindow();
        if (hwnd != NULL)
        {
            HMENU hMenu = ::GetSystemMenu(hwnd, FALSE);
            if (hMenu != NULL)
            {
                DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
                DeleteMenu(hMenu, SC_MINIMIZE, MF_BYCOMMAND);
                DeleteMenu(hMenu, SC_MAXIMIZE, MF_BYCOMMAND);
            }
        }
        lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        hConHandle = _open_osfhandle(PtrToUlong(lStdHandle), _O_TEXT);
        fp = _fdopen(hConHandle, "w");
        *stdout = *fp;
        setvbuf(stdout, NULL, _IONBF, 0);
    }

    void detach()
    {
        FreeConsole();
    }
}

namespace menu
{
    void initImGui(HWND h, LPDIRECT3DDEVICE9 device)
    {
        ImGui::CreateContext();
        ImGui_ImplDX9_Init(device);
        ImGui_ImplWin32_Init(h);

        auto& io = ImGui::GetIO();
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", 15.0f, NULL, io.Fonts->GetGlyphRangesCyrillic());

        ImGui::StyleColorsLight();
    }

    static bool a;
    static int b;
    static float c[4];

    void draw()
    {
        ImGui::Begin("alternative hack for counter-stike: global offensive", nullptr, ImVec2(300, 200), 1.0f);

        ImGui::Checkbox("rcs", &rcs);
        ImGui::Checkbox("esp", &esp);
        ImGui::Checkbox("teammates esp", &esp_teammates);

        ImGui::End(); 
    }
}

namespace game_utils
{
    bool WorldToScreen(const Vector& vIn, float* flOut)
    {
        flOut[0] = g_local.vMatrix->m[0][0] * vIn.x + g_local.vMatrix->m[0][1] * vIn.y + g_local.vMatrix->m[0][2] * vIn.z + g_local.vMatrix->m[0][3];
        flOut[1] = g_local.vMatrix->m[1][0] * vIn.x + g_local.vMatrix->m[1][1] * vIn.y + g_local.vMatrix->m[1][2] * vIn.z + g_local.vMatrix->m[1][3];

        float w = g_local.vMatrix->m[3][0] * vIn.x + g_local.vMatrix->m[3][1] * vIn.y + g_local.vMatrix->m[3][2] * vIn.z + g_local.vMatrix->m[3][3];

        if (w < 0.01)
            return false;

        float invw = 1.0f / w;

        flOut[0] *= invw;
        flOut[1] *= invw;

        int width, height;

        auto io = ImGui::GetIO();
        width = io.DisplaySize.x;
        height = io.DisplaySize.y;

        float x = (float)width / 2;
        float y = (float)height / 2;

        x += 0.5 * flOut[0] * (float)width + 0.5;
        y -= 0.5 * flOut[1] * (float)height + 0.5;

        flOut[0] = x;
        flOut[1] = y;

        return true;
    }
}

namespace temp_data
{
    DWORD dwPlayerEntity[64];
    int* iTeamPlayers[64];
    int* iHealthPlayers[64];
    bool* iDormant[64];
    Vector* vOrigin[64];
    void clear()
    {
        ZeroMemory(dwPlayerEntity, sizeof(dwPlayerEntity));
        ZeroMemory(iTeamPlayers, sizeof(iTeamPlayers));
        ZeroMemory(iHealthPlayers, sizeof(iHealthPlayers));
        ZeroMemory(iDormant, sizeof(iDormant));
        ZeroMemory(vOrigin, sizeof(vOrigin));
    }
}

namespace render
{
    void AddRect(const ImVec2& position, const ImVec2& size, const ImColor& color, float rounding = 0.f)
    {
        auto window = ImGui::GetCurrentWindow();

        window->DrawList->AddRect(position, size, ImGui::ColorConvertFloat4ToU32(color), rounding);
    }
    void DrawBox(float x, float y, float w, float h, const ImColor& color)
    {
        AddRect(ImVec2(x, y), ImVec2(x + w, y + h), color);
    }
    void AddRectFilled(const ImVec2& position, const ImVec2& size, const ImColor& color, float rounding = 0.f)
    {
        auto window = ImGui::GetCurrentWindow();

        window->DrawList->AddRectFilled(position, size, ImGui::ColorConvertFloat4ToU32(color), rounding);
    }
    void DrawFillArea(float x, float y, float w, float h, const ImColor& color)
    {
        AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), color);
    }
    void PlayerESP(int i)
    {
        ImColor PlayerColor;

        if (g_players[i].iTeam == 2)
            PlayerColor = ImColor(200, 0, 0);
        else if (g_players[i].iTeam == 3)
            PlayerColor = ImColor(0, 0, 200);

        float flScreenPosTop[2];
        float flScreenPosBottom[2];
        if (game_utils::WorldToScreen(Vector(g_players[i].vOrigin.x, g_players[i].vOrigin.y, g_players[i].vOrigin.z + 72.f), flScreenPosTop)
            && game_utils::WorldToScreen(g_players[i].vOrigin, flScreenPosBottom))
        {
            float h = flScreenPosBottom[1] - flScreenPosTop[1];
            float w = h / 2;
            float x = flScreenPosBottom[0] - w / 2;
            float y = flScreenPosTop[1];

            DrawBox(x, y, w, h, PlayerColor);

            float health = g_players[i].iHealth;
            health = ImClamp(health, 0.f, 100.f);

            const auto size = h / 100.f * health;
            const auto thickness = 2.f;

            DrawFillArea(x - thickness - 1.9f, y + h, thickness, -size, ImColor(0.f, 1.f, 0.f));
            DrawBox(x - thickness - 2.9f, y - 1.f, thickness + 2.f, h + 2.f, ImColor(0.f, 0.f, 0.f));
        }
    }
    void DrawOverlay()
    {
        if (*g_local.iGameState != FULL_CONNECTED)
            return;

        if (esp)
        {
            for (int id = 1; id < 64; id++)
            {
                if (g_players[id].iHealth <= 0)
                    continue;

                if (g_players[id].iDormant == true)
                    continue;

                if (!esp_teammates && g_players[id].iTeam == *g_local.iTeam)
                    continue;

                PlayerESP(id);
            }
        }      
    }
}

HRESULT WINAPI EndSceneHooked(LPDIRECT3DDEVICE9 device)
{
    static bool init = false;
    if (!init)
    {
        menu::initImGui(hCSGO, device);
        init = true;
    }

    if (!unhook)
    {
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4());
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::Begin("##BackBuffer", static_cast<bool*>(0), ImVec2(), 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings);
        ImGui::SetWindowPos(ImVec2(), ImGuiCond_Always);
        ImGui::SetWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);
        render::DrawOverlay();
        ImGui::GetCurrentWindow()->DrawList->PushClipRectFullScreen();
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        if (menu_open) {
            menu::draw();
            ImGui::GetIO().MouseDrawCursor = true;
        }
        else
        {
            ImGui::GetIO().MouseDrawCursor = false;
        }

        ImGui::Render();

        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    }

    return pEndScene(device);
}

LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

HRESULT WINAPI wndproc_hooked(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!unhook)
    {
        if (uMsg == WM_KEYDOWN && wParam == VK_INSERT)
        {
            menu_open = !menu_open;
            return TRUE;
        }

        if (menu_open)
        {
            ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam);
            return TRUE;
        }
    } 

    CallWindowProc(wndprocOriginal, hwnd, uMsg, wParam, lParam);
}

BOOL WINAPI SetCursorPosHooked(int x, int y)
{
    if (menu_open && !unhook)
        return TRUE;

    return pSetCursorPos(x, y);
}

void SetupHack(HMODULE hModule)
{
    console::attach("debug");

    hCSGO = FindWindowA(NULL, "Counter-Strike: Global Offensive");

    LPDIRECT3D9 pD3D;
    if ((pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
    {
        std::cout << "failed to create dx9 context\n";
        FreeLibraryAndExitThread(hModule, 1);
    }

    D3DPRESENT_PARAMETERS d3dpp;
    d3dpp.hDeviceWindow = hCSGO;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = FALSE;

    LPDIRECT3DDEVICE9 Device;
    if (FAILED(pD3D->CreateDevice(0, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &Device)))
    {
        std::cout << "failed to create device #1 -> retry...\n";
        d3dpp.Windowed = !d3dpp.Windowed;
        if (FAILED(pD3D->CreateDevice(0, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &Device)))
        {
            std::cout << "failed to create device #2\n";
            pD3D->Release();
            FreeLibraryAndExitThread(hModule, 1);
        }
    }

    void** pVTable = *reinterpret_cast<void***>(Device);

    if (Device)
        Device->Release(), Device = nullptr;

    if (MH_Initialize() != MH_OK)
    {
        std::cout << "error initialize minhook\n";
        MH_Uninitialize();
        FreeLibraryAndExitThread(hModule, 1);
    }
  
    MH_CreateHook((PVOID)pVTable[42], &EndSceneHooked, (PVOID*)&pEndScene);
    MH_EnableHook((PVOID)pVTable[42]);
    std::cout << "pEndScene hook installed: 0x" << pEndScene << std::endl;
    MH_CreateHook((PVOID)SetCursorPos, &SetCursorPosHooked, (PVOID*)&pSetCursorPos);
    MH_EnableHook((PVOID)SetCursorPos);
    std::cout << "pSetCursorPos hook installed: 0x" << pSetCursorPos << std::endl;
    wndprocOriginal = reinterpret_cast<WNDPROC>(SetWindowLong(hCSGO, GWL_WNDPROC, reinterpret_cast<LONG_PTR>(wndproc_hooked)));
    std::cout << "wndprocOriginal hook installed: 0x" << wndprocOriginal << std::endl;

    HRESULT hrRes = URLDownloadToFile(0, "https://raw.githubusercontent.com/frk1/hazedumper/master/csgo.toml", "C://Offsets.ini", 0, 0);

    std::cout << "download offset result: " << hrRes << std::endl;

    m_pOffsets->LoadOffsets();
    
    Sleep(1500);
    while (!GetAsyncKeyState(VK_DELETE))
    {
        if (*g_local.iGameState == SPAWN)
        {
            temp_data::clear();
        }
        else if (*g_local.iGameState == FULL_CONNECTED)
        {  
            for (int id = 0; id < 64; id++)
            {
                if (temp_data::dwPlayerEntity[id] > 0)
                {
                    g_players[id].iTeam = *temp_data::iTeamPlayers[id];
                    g_players[id].iHealth = *temp_data::iHealthPlayers[id];
                    g_players[id].iDormant = *temp_data::iDormant[id];
                    g_players[id].vOrigin = *temp_data::vOrigin[id];
                }     
            }
            
            if (rcs)
            {
                static QAngle vOldPunch = { 0.f, 0.f, 0.f };
                QAngle vNewPunch = *g_local.QPunchAngle * 2.f;
                vNewPunch -= vOldPunch;
                vNewPunch *= 0.1f;
                vNewPunch += vOldPunch;
                QAngle P = vNewPunch - vOldPunch;
                QAngle QResult = *g_local.QMyAngle - P;
                QResult.Normalize();
                if (!g_local.QPunchAngle->IsZero2D())
                {
                    *g_local.QMyAngle = QResult;
        
                }
                vOldPunch = vNewPunch;
            }
        }    
        Sleep(10);
    }
    unhook = true;
    menu_open = false;
    SetWindowLong(hCSGO, GWL_WNDPROC, (LONG_PTR)wndprocOriginal);
    std::cout << "wndrpoc unhooked\n";
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    std::cout << "imgui shutdown\n";
    Sleep(100);
    MH_DisableHook((PVOID)pVTable[42]);
    MH_RemoveHook((PVOID)pVTable[42]);
    std::cout << "endscene unhooked\n";
    MH_DisableHook((PVOID)SetCursorPos);
    MH_RemoveHook((PVOID)SetCursorPos);
    std::cout << "setcursorpos unhooked\n";
    MH_Uninitialize();
    std::cout << "minhook uninitialized\nfree library...\n\n";
    Sleep(6000);
    FreeLibraryAndExitThread(hModule, 0);
}

void CollectData(void)
{
    DWORD client = (DWORD)GetModuleHandle("client.dll");
    DWORD engine = (DWORD)GetModuleHandle("engine.dll");
    while (!unhook)
    {
        g_local.vMatrix = (Matrix4x4*)(client + m_pOffsets->dwViewMatrix);
        g_local.dwClientState = *(DWORD*)(engine + m_pOffsets->dwClientState);
        g_local.QMyAngle = (QAngle*)(g_local.dwClientState + m_pOffsets->dwClientState_ViewAngles);
        g_local.dwLocalPlayer = *(DWORD*)(client + m_pOffsets->dwLocalPlayer);
        g_local.QPunchAngle = (QAngle*)(g_local.dwLocalPlayer + m_pOffsets->m_aimPunchAngle);
        g_local.iShortFired = (int*)(g_local.dwLocalPlayer + m_pOffsets->m_iShotsFired);
        g_local.iHealth = (int*)(g_local.dwLocalPlayer + m_pOffsets->m_iHealth);
        g_local.iTeam = (int*)(g_local.dwLocalPlayer + m_pOffsets->m_iTeamNum);
        g_local.iGameState = (int*)(g_local.dwClientState + m_pOffsets->dwClientState_State);

        for (int id = 0; id < 64; id++)
        {
            temp_data::dwPlayerEntity[id] = *(DWORD*)(client + m_pOffsets->dwEntityList + (id * 0x10));
            temp_data::iTeamPlayers[id] = (int*)(temp_data::dwPlayerEntity[id] + m_pOffsets->m_iTeamNum);
            temp_data::iHealthPlayers[id] = (int*)(temp_data::dwPlayerEntity[id] + m_pOffsets->m_iHealth);
            temp_data::iDormant[id] = (bool*)(temp_data::dwPlayerEntity[id] + m_pOffsets->m_bDormant);
            temp_data::vOrigin[id] = (Vector*)(temp_data::dwPlayerEntity[id] + m_pOffsets->m_vecOrigin);
        }
        Sleep(5000);
    }
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)SetupHack, hModule, 0, 0);
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)CollectData, 0, 0, 0);      
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

