// Vardian.cpp : Definiert den Einstiegspunkt für die Anwendung.
//

#include "framework.h"
#include "Vardian.h"
#include <string>
#include "rokid_interface.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_prober.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <os/os_time.h>

#define MAX_LOADSTRING 100

// Globale Variablen:
HINSTANCE hInst;                                // Aktuelle Instanz
WCHAR szTitle[MAX_LOADSTRING];                  // Titelleistentext
WCHAR szWindowClass[MAX_LOADSTRING];            // Der Klassenname des Hauptfensters.
static struct xrt_device* rokid_device = NULL;  // Pointer to Rokid Max
UINT_PTR timerId = NULL;                        // timer to copy part of screen to Rokid Max
RECT clientRect;                                // position and size of drawing area in window
const UINT timerInterval = 16;                  // close to the refresh rate @60hz
RECT virtualScreenRect;                         // values of the virtual screen
POINT unscaledMousePosition;                     // unscaled mouse position from a mouse hook
HDC hdcWindow = NULL;                           // DC to copy to
HDC hdcScreen = NULL;                           // DC from the whole virtual screen
HWND hWnd = NULL;                               // main window

// Vorwärtsdeklarationen der in diesem Codemodul enthaltenen Funktionen:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
BOOL                DeInitInstance();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
bool                InitializeRokidWindow(HWND hWnd);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Hier Code einfügen.

    // Globale Zeichenfolgen initialisieren
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_VARDIAN, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Anwendungsinitialisierung ausführen:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_VARDIAN));

    // Find Rokid Max device and start screen copy functionality
    InitializeRokidWindow(hWnd);

    MSG msg;

    // Hauptnachrichtenschleife:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (!DeInitInstance()) {
        return FALSE;
    }


    return (int) msg.wParam;
}



//
//  FUNKTION: MyRegisterClass()
//
//  ZWECK: Registriert die Fensterklasse.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VARDIAN));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_VARDIAN);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//Callback to get the unscaled mouse position
// TODO: exchange this function by another one - I did not find unother solution
LRESULT CALLBACK mouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    PMSLLHOOKSTRUCT p = (PMSLLHOOKSTRUCT)lParam;
    unscaledMousePosition.x = p->pt.x;
    unscaledMousePosition.y = p->pt.y;

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

//
//   FUNKTION: InitInstance(HINSTANCE, int)
//
//   ZWECK: Speichert das Instanzenhandle und erstellt das Hauptfenster.
//
//   KOMMENTARE:
//
//        In dieser Funktion wird das Instanzenhandle in einer globalen Variablen gespeichert, und das
//        Hauptprogrammfenster wird erstellt und angezeigt.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Instanzenhandle in der globalen Variablen speichern

   // set size of Rokid Max
   RECT hostWindowRect = { 0,0,1920,1080 };

   hWnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED /* | WS_EX_TOOLWINDOW */,
       szWindowClass, szTitle, 
       WS_SIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN | WS_CAPTION | WS_MAXIMIZEBOX,
       hostWindowRect.left,
       hostWindowRect.top,
       hostWindowRect.right - hostWindowRect.left,
       hostWindowRect.bottom - hostWindowRect.top,
       nullptr, nullptr, hInstance, nullptr);
   // TODO: Das war der originale Aufruf - löschen?
   // HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
   //   CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   // Make the window opaque.
   SetLayeredWindowAttributes(hWnd, 0, 255, LWA_ALPHA);

   HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, mouseHookProc, hInstance, NULL);

   ShowWindow(hWnd, SW_SHOWMINIMIZED /*TODO Maybe later: , SW_HIDE*/);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//   FUNKTION: DeInitInstance()
//
//   ZWECK: Stop Timer and deinitialize Rokid Max handle
//
//   KOMMENTARE:
//
//        In dieser Funktion wird das Instanzenhandle in einer globalen Variablen gespeichert, und das
//        Hauptprogrammfenster wird erstellt und angezeigt.
//
BOOL DeInitInstance()
{
    KillTimer(NULL, timerId);
    if (rokid_device != NULL) {
        rokid_device[0].destroy(rokid_device);
    }

    return TRUE;
}


//
//  FUNKTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ZWECK: Verarbeitet Meldungen für das Hauptfenster.
//
//  WM_COMMAND  - Verarbeiten des Anwendungsmenüs
//  WM_PAINT    - Darstellen des Hauptfensters
//  WM_DESTROY  - Ausgeben einer Beendenmeldung und zurückkehren
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Menüauswahl analysieren:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Zeichencode, der hdc verwendet, hier einfügen...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_DISPLAYCHANGE:
        // A monitor could be removed
        // wait some time to take care of starting Rokid Max USB
        
        // wait some time and initialize window
        os_nanosleep(2000000000);
        InitializeRokidWindow(hWnd);
        break;
    case WM_SIZE:
        if (hWnd != NULL)
        {
            // read actual drawing area size
            GetClientRect(hWnd, &clientRect);
            // Resize the control to fill the window.
            SetWindowPos(hWnd, NULL,
                clientRect.left, clientRect.top, clientRect.right, clientRect.bottom, 0);
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Meldungshandler für Infofeld.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

struct monitor_struct_typ {
    HMONITOR rokid_handle = NULL;
    LONG left = 0;
    LONG right = 0;
    LONG top = 0;
    LONG bottom = 0;
    bool found = 0;
    DISPLAY_DEVICE device = {};
    std::wstring szDevice;
    DEVMODE DevMode;
};

// detaching the monitor is not possible anymore!
static bool get_rokid_monitor_handle(struct monitor_struct_typ& monitor_struct) {
    UINT pathCount;
    UINT modeCount;
    bool rokid_monitor_found = false;
    LONG retValue = GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount);
    if (retValue == 0) {
        std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);
        std::vector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);
        if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(), &modeCount, modes.data(), nullptr) == 0) {
            // enum all monitors => (handle, device name)>
            std::unordered_map<std::wstring, struct monitor_struct_typ> monitors;
            EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hmon, HDC hdc, LPRECT rc, LPARAM lp)
                {
                    MONITORINFOEX mi = {};
                    mi.cbSize = sizeof(MONITORINFOEX);
                    GetMonitorInfo(hmon, &mi);
                    struct monitor_struct_typ tempMonitor;
                    tempMonitor.rokid_handle = hmon;
                    tempMonitor.szDevice = mi.szDevice;
                    tempMonitor.left = mi.rcMonitor.left;
                    tempMonitor.right = mi.rcMonitor.right;
                    tempMonitor.top = mi.rcMonitor.top;
                    tempMonitor.bottom = mi.rcMonitor.bottom;
                    tempMonitor.found = false;

                    auto monitors = (std::unordered_map<std::wstring, struct monitor_struct_typ>*)lp;
                    monitors->insert(std::pair<std::wstring, struct monitor_struct_typ>(mi.szDevice, tempMonitor));

                    return TRUE;
                }, (LPARAM)&monitors);

            // for each path, get GDI device name and compare with monitor device name
            for (UINT i = 0; i < pathCount; i++)
            {
                DISPLAYCONFIG_TARGET_DEVICE_NAME deviceName = {};
                deviceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
                deviceName.header.size = sizeof(DISPLAYCONFIG_TARGET_DEVICE_NAME);
                deviceName.header.adapterId = paths[i].targetInfo.adapterId;
                deviceName.header.id = paths[i].targetInfo.id;
                if (DisplayConfigGetDeviceInfo((DISPLAYCONFIG_DEVICE_INFO_HEADER*)&deviceName))
                    continue;

                std::wstring the_out;

                the_out += std::wstring(L"Monitor Friendly Name : '") + deviceName.monitorFriendlyDeviceName + L"'\n";
                the_out += std::wstring(L"Monitor Device Path   : '") + deviceName.monitorDevicePath + L"'\n";

                DISPLAYCONFIG_SOURCE_DEVICE_NAME sourceName = {};
                sourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
                sourceName.header.size = sizeof(DISPLAYCONFIG_SOURCE_DEVICE_NAME);
                sourceName.header.adapterId = paths[i].targetInfo.adapterId;
                sourceName.header.id = paths[i].sourceInfo.id;
                if (DisplayConfigGetDeviceInfo((DISPLAYCONFIG_DEVICE_INFO_HEADER*)&sourceName))
                    continue;

                the_out += std::wstring(L"GDI Device Name       : '") + sourceName.viewGdiDeviceName + L"'\n";

                // find the monitor with this device name
                auto element = monitors.find(sourceName.viewGdiDeviceName);
                if (element != monitors.end()) {
                    the_out += std::wstring(L"Monitor Handle        : '") + std::to_wstring((unsigned long long)element->second.rokid_handle) + L"'\n";

                    OutputDebugString(the_out.c_str());


                    if (std::wstring(deviceName.monitorFriendlyDeviceName) == std::wstring(L"Rokid Max")) {
                        monitor_struct = element->second;
                        monitor_struct.found = true;

                        rokid_monitor_found = true;
                    }
                }
            }

            if (rokid_monitor_found) {
                // get current setting of Rokid Max
                ZeroMemory(&monitor_struct.DevMode, sizeof(DEVMODE));
                monitor_struct.DevMode.dmSize = sizeof(DEVMODE);
                if (!EnumDisplaySettings(monitor_struct.szDevice.c_str(), ENUM_CURRENT_SETTINGS, &monitor_struct.DevMode)) {
                    // TODO error handling
                }

                // move Rokid Max to the far right side of the screen
                DEVMODE tempMode = monitor_struct.DevMode;
                tempMode.dmPosition.x = GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN);
                tempMode.dmPosition.y = 0;
                ChangeDisplaySettingsEx(monitor_struct.szDevice.c_str(), &tempMode, NULL, 0, NULL);

                //request the device setting again
                if (!EnumDisplaySettings(monitor_struct.szDevice.c_str(), ENUM_CURRENT_SETTINGS, &monitor_struct.DevMode)) {
                    // TODO error handling
                }
            }
        }
    }

    return rokid_monitor_found;
}

//
// FUNCTION: UpdateRokidWindow()
//
// PURPOSE: Sets the source rectangle and updates the window. Called by a timer.
//
void CALLBACK UpdateRokidWindow(HWND hWnd, UINT /*uMsg*/, UINT_PTR /*idEvent*/, DWORD /*dwTime*/)
{
    // output dimensions at rokid max display
    static RECT sourceRect = { 500, 500, 2420, 1580 };
    static uint64_t last_timestamp = os_monotonic_get_ns();

    std::wstring the_out;

    // Get the position of the Rokid Max
    if (rokid_device != NULL) {
        static LONG collect_x_movement = 0;
        static LONG collect_y_movement = 0;
        struct xrt_space_relation out_relation;

        rokid_device->get_tracked_pose(rokid_device,
            XRT_INPUT_GENERIC_HEAD_POSE,
            0,
            &out_relation);

        const LONG mult = 500;
        const LONG y_threshold = 100;
        const LONG x_threshold = 50;

        collect_y_movement += (LONG)(out_relation.angular_velocity.y * mult);
        collect_x_movement += (LONG)(out_relation.angular_velocity.x * mult);

        // if a specific amount of time is over and threshold not reached then delete collectors
        // This is done becase 3DOF tracking of Rokid Max also moves if glasses are not moved
        uint64_t time_diff = os_monotonic_get_ns() - last_timestamp;
        if (time_diff > 1000000000 &&
            collect_y_movement <= y_threshold &&
            collect_x_movement <= x_threshold) {
            // reset timestamp
            last_timestamp = os_monotonic_get_ns();
            collect_x_movement = 0;
            collect_y_movement = 0;
        }

        LONG x_size = clientRect.right - clientRect.left;
        LONG y_size = clientRect.bottom - clientRect.top;

        // only move in 50 pixel steps
        if (abs(collect_y_movement) > y_threshold) {
            sourceRect.left -= collect_y_movement;
            collect_y_movement = 0;

            // Don't scroll outside whole virtual desktop area.
            if (sourceRect.left < virtualScreenRect.left)
            {
                sourceRect.left = virtualScreenRect.left;
            }
            else if (sourceRect.left > virtualScreenRect.right)
            {
                sourceRect.left = virtualScreenRect.right;
            }

            sourceRect.right = sourceRect.left + x_size;
        }

        if (abs(collect_x_movement) > x_threshold) {
            sourceRect.top -= collect_x_movement;
            collect_x_movement = 0;

            if (sourceRect.top < virtualScreenRect.top)
            {
                sourceRect.top = virtualScreenRect.top;
            }
            else if (sourceRect.top > virtualScreenRect.bottom)
            {
                sourceRect.top = virtualScreenRect.bottom;
            }
            sourceRect.bottom = sourceRect.top + y_size;
        }

        the_out = the_out + L"Source Rectangle: " +
            L" sourceRect.left:  " + std::to_wstring(sourceRect.left) +
            L" sourceRect.top:  " + std::to_wstring(sourceRect.top) +
            L" sourceRect.right:  " + std::to_wstring(sourceRect.right) +
            L" sourceRect.bottom:  " + std::to_wstring(sourceRect.bottom) +
            L" out_relation.angular_velocity.x:  " + std::to_wstring(out_relation.angular_velocity.x) +
            L" out_relation.angular_velocity.y:  " + std::to_wstring(out_relation.angular_velocity.y) +
            L" out_relation.angular_velocity.x:  " + std::to_wstring(out_relation.angular_velocity.z) +
            L"\n";
        OutputDebugString(the_out.c_str());


        // TODO: insert stretchBlt here
        // TODO: insert Cursor copy here
        // Set the source rectangle for the magnifier control.
//        MagSetWindowSource(hwndMag, sourceRect);
            // This is the best stretch mode.
        SetStretchBltMode(hdcWindow, HALFTONE);

        // The source DC is the entire screen, and the destination DC is the current window (HWND).
        if (!StretchBlt(hdcWindow,
            clientRect.left, clientRect.top,
            clientRect.right, clientRect.bottom,
            hdcScreen,
            sourceRect.left, sourceRect.top,
            sourceRect.right,
            sourceRect.bottom,
            SRCCOPY))
        {
            MessageBox(hWnd, L"StretchBlt has failed", L"Failed", MB_OK);
        }


        CURSORINFO cursor = { sizeof(cursor) };
        GetCursorInfo(&cursor);
        if (cursor.flags == CURSOR_SHOWING) {
            ICONINFO info = { sizeof(info) };
            GetIconInfo(cursor.hCursor, &info);
            const int x = cursor.ptScreenPos.x - clientRect.left; // -info.xHotspot;
            const int y = cursor.ptScreenPos.y - clientRect.top; // -info.yHotspot;
            BITMAP bmpCursor = { 0 };
            GetObject(info.hbmColor, sizeof(bmpCursor), &bmpCursor);
            DrawIconEx(hdcWindow, x, y, cursor.hCursor, bmpCursor.bmWidth, bmpCursor.bmHeight,
                0, NULL, DI_NORMAL);
        }

        // Reclaim topmost status, to prevent unmagnified menus from remaining in view. 
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

        // Force redraw.
        // TODO: Do we need the following line
//        InvalidateRect(hWnd, NULL, TRUE);
    }
}


bool InitializeRokidWindow(HWND hWnd) {
    OutputDebugString(L"Begin InitializeRokidWindow");
    static bool running = false;

    std::wstring the_out;

    if (running) {
        KillTimer(NULL, timerId);
        timerId = NULL;

        if (rokid_device != NULL) {
            rokid_device[0].destroy(rokid_device);
            rokid_device = NULL;
        }

        // show window minimized
        ShowWindow(hWnd, SW_SHOWMINIMIZED /*TODO Maybe later: , SW_HIDE*/);

        running = false;
    }

    struct xrt_prober_device* the_rokid_devlist = new struct xrt_prober_device[1];
    the_rokid_devlist[0].product_id = ROKID_PID;
    the_rokid_devlist[0].vendor_id = ROKID_VID;
    the_rokid_devlist[0].bus = XRT_BUS_TYPE_USB;

    std::unique_ptr<struct xrt_device*> out_rokid_ptr =
        std::make_unique<struct xrt_device*>(new struct xrt_device[1]);

    if (rokid_found(NULL, &the_rokid_devlist, 1, 0, NULL, out_rokid_ptr.get()) == 1) {
        rokid_device = out_rokid_ptr.get()[0];
    }
    else {
        return false;
    }

    // get all paths and figure out monitor handle for "Rokid Max"
    struct monitor_struct_typ monitor_struct;

    // find Rokid Max and move Rokid Max monitor to most right
    if (get_rokid_monitor_handle(monitor_struct) == false) {
        return false;
    }

    ShowWindow(hWnd, SW_SHOWNORMAL);

    if (SetWindowPos(hWnd, HWND_TOPMOST, monitor_struct.DevMode.dmPosition.x, monitor_struct.DevMode.dmPosition.y,
        monitor_struct.DevMode.dmPelsWidth / 2,
        monitor_struct.DevMode.dmPelsHeight,
        SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOACTIVATE) == 0) {
        return false;
    }

    RECT mainWindowRect;
    GetWindowRect(hWnd, &mainWindowRect);
    the_out = std::wstring(L"Main Window Rectangle: ") +
        L" mainWindowRect.left:  " + std::to_wstring(mainWindowRect.left) +
        L" mainWindowRect.top:  " + std::to_wstring(mainWindowRect.top) +
        L" mainWindowRect.right:  " + std::to_wstring(mainWindowRect.right) +
        L" mainWindowRect.bottom:  " + std::to_wstring(mainWindowRect.bottom) +
        L"\n";
    OutputDebugString(the_out.c_str());

    GetWindowRect(hWnd, &mainWindowRect);
    the_out = std::wstring(L"Main Window Rectangle: ") +
        L" monitor_struct.DevMode.dmPosition.x:  " + std::to_wstring(monitor_struct.DevMode.dmPosition.x) +
        L" monitor_struct.DevMode.dmPosition.y:  " + std::to_wstring(monitor_struct.DevMode.dmPosition.y) +
        L" monitor_struct.DevMode.dmPelsWidth:  " + std::to_wstring(monitor_struct.DevMode.dmPelsWidth) +
        L" monitor_struct.DevMode.dmPelsHeight:  " + std::to_wstring(monitor_struct.DevMode.dmPelsHeight) +
        L"\n";
    OutputDebugString(the_out.c_str());

    // call update window after creating window and maybe rokid handles
    if (UpdateWindow(hWnd) == false) {
        return false;
    }

    // change source rect size, because show window could be changed
    GetClientRect(hWnd, &clientRect);

    // reset values for virtual screen
    LONG x_size = clientRect.right - clientRect.left;
    LONG y_size = clientRect.bottom - clientRect.top;

    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    unsigned int vScreenWidth = GetSystemMetricsForDpi(SM_CXVIRTUALSCREEN, 96 /* 100% scaling*/);
    unsigned int vScreenHeight = GetSystemMetricsForDpi(SM_CYVIRTUALSCREEN, 96 /* 100% scaling*/);

    virtualScreenRect.left = GetSystemMetricsForDpi(SM_XVIRTUALSCREEN, 96) - x_size / 3;
    virtualScreenRect.top = GetSystemMetricsForDpi(SM_YVIRTUALSCREEN, 96) - y_size / 3;
    virtualScreenRect.right = GetSystemMetrics(SM_XVIRTUALSCREEN) + vScreenWidth - x_size + x_size / 3;
    virtualScreenRect.bottom = GetSystemMetrics(SM_YVIRTUALSCREEN) + vScreenHeight - y_size + y_size / 3;

    // prevent Window from being copied
    SetWindowDisplayAffinity(hWnd, WDA_EXCLUDEFROMCAPTURE);

    hdcWindow = GetDC(hWnd);
    hdcScreen = GetDC(NULL);

    // Create a timer to update the control. But only if Rokid Max is connected
    timerId = SetTimer(hWnd, 0, timerInterval, UpdateRokidWindow);

    running = true;

    return true;
}
