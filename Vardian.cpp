// Vardian.cpp : Definiert den Einstiegspunkt für die Anwendung.
// 
// Source code created with the help of ChatGPT. It was a great help for me.
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
#include <util/u_logging.h>
#include <util/u_debug.h>
#include <locale>
#include <codecvt>
#include <shellapi.h>

#define MAX_LOADSTRING 100

#define WM_MYMESSAGE (WM_USER + 100)

// Globale Variablen:
HINSTANCE hInst;                                // Aktuelle Instanz
WCHAR szTitle[MAX_LOADSTRING];                  // Titelleistentext
WCHAR szWindowClass[MAX_LOADSTRING];            // Der Klassenname des Hauptfensters.
static struct xrt_device* rokid_device = NULL;  // Pointer to Rokid Max
UINT_PTR timerId = NULL;                        // timer to copy part of screen to Rokid Max
const UINT timerInterval = 16;                  // close to the refresh rate @60hz
RECT virtualScreenRectWithoutRokidMax;          // values of the virtual screen without the X axis from Rokid Max
HDC hdcWindow = NULL;                           // DC to copy to
HDC hdcScreen = NULL;                           // DC from the whole virtual screen
HWND hWnd = NULL;                               // main window
bool running = false;
RECT sourceRect = { 500, 500, 2420, 1580 };

// Declare global variables
NOTIFYICONDATA nid;

// Vorwärtsdeklarationen der in diesem Codemodul enthaltenen Funktionen:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
BOOL                DeInitInstance();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Properties(HWND, UINT, WPARAM, LPARAM);
bool                InitializeRokidWindow(HWND hWnd);
void                AddTaskbarIcon();
void                RemoveTaskbarIcon();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // set log option
    _putenv("XRT_LOG=trace");
    _putenv("ROKID_LOG=trace");

    wchar_t my_filename[MAX_PATH]; 
    GetModuleFileName(NULL, my_filename, MAX_PATH);

    std::wstring my_filename_str(my_filename);
    FILE* new_stdout;
    FILE* new_stderr;
    _wfreopen_s(&new_stdout, (my_filename_str + L".out.log" ).c_str(), L"w", stdout);
    _wfreopen_s(&new_stderr, (my_filename_str + L".err.log").c_str(), L"w", stderr);

    // TODO: Hier Code einfügen.
    U_LOG(U_LOGGING_TRACE, "Start Vardian");

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
 
    // Add taskbar icon
    AddTaskbarIcon();

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

    // Remove taskbar icon
    RemoveTaskbarIcon();

    if (!DeInitInstance()) {
        return FALSE;
    }

    U_LOG(U_LOGGING_TRACE, "Finish Vardian");

    fclose( new_stdout );
    fclose( new_stderr );

    return (int) msg.wParam;
}



//
//  FUNKTION: MyRegisterClass()
//
//  ZWECK: Registriert die Fensterklasse.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex{};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VARDIAN));
    wcex.hCursor        = LoadCursor(nullptr, IDC_NO);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_VARDIAN);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
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

   // create small window
   hWnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED /* WS_EX_TOOLWINDOW */,
       szWindowClass, szTitle, 
       /* WS_SIZEBOX | WS_SYSMENU | */ WS_CLIPCHILDREN | /* WS_CAPTION  |  WS_MAXIMIZEBOX | */ WS_POPUP,
       0,
       0,
       0,
       0,
       nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
       U_LOG(U_LOGGING_ERROR, "Could not create main window.");
       return FALSE;
   }

   // Make the window opaque.
   SetLayeredWindowAttributes(hWnd, 0, 255, LWA_ALPHA);

   ShowWindow(hWnd, SW_SHOWMINIMIZED );
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
            case 1:
                // Exit the program
                DestroyWindow(hWnd);
                break;
            case 2:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case 3:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_PROPPAGE_MEDIUM), hWnd, Properties);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        if (running) {
            PAINTSTRUCT ps;

            HDC hdc = BeginPaint(hWnd, &ps);

            RECT intersectSource;

            IntersectRect(&intersectSource, &sourceRect, &virtualScreenRectWithoutRokidMax);

            SIZE intersectSize = { 0,0 };
            intersectSize.cx = intersectSource.right - intersectSource.left;
            intersectSize.cy = intersectSource.bottom - intersectSource.top;

            // client rect is always at 0,0. Thus, move the intersectSource to client area
            RECT intersectClientRect = intersectSource;
            OffsetRect(&intersectClientRect, -sourceRect.left, -sourceRect.top);

            // The source DC is the entire screen, and the destination DC is the current window (HWND).
            if (!BitBlt(hdc /*hdcWindow*/,
                intersectClientRect.left, intersectClientRect.top,
                intersectClientRect.right, intersectClientRect.bottom,
                hdcScreen,
                intersectSource.left, intersectSource.top,
                SRCCOPY))
            {
                std::string the_out = std::string("BitBlt failed: ") +
                    " intersectSource.left, top, width x heigt:  " + std::to_string(intersectSource.left) +
                    ", " + std::to_string(intersectSource.top) +
                    ", " + std::to_string(intersectSource.right - intersectSource.left) + " x " + std::to_string(intersectSource.bottom - intersectSource.top) +
                    " intersectClientRect.left, top, width x height  " + std::to_string(intersectClientRect.left) +
                    ", " + std::to_string(intersectClientRect.top) +
                    ", " + std::to_string(intersectSize.cx) + " x " + std::to_string(intersectSize.cy) +
                    " ps.rcPaint.left, top, width x height  " + std::to_string(ps.rcPaint.left) +
                    ", " + std::to_string(ps.rcPaint.top) +
                    ", " + std::to_string(ps.rcPaint.right - ps.rcPaint.left) + " x " + std::to_string(ps.rcPaint.bottom - ps.rcPaint.top);
                U_LOG(U_LOGGING_TRACE, the_out.c_str());
            }

            HRGN clientRgn = CreateRectRgnIndirect(&intersectClientRect);
            HRGN targetWindowRgn = CreateRectRgnIndirect(&ps.rcPaint);
            HRGN destRgn = CreateRectRgn(0, 0, 1, 1);

            // we only have to fill a region if it is a COMPLEXREGION or a simple rect
            int regionType = CombineRgn(destRgn, targetWindowRgn, clientRgn, RGN_XOR);

            if ((regionType == SIMPLEREGION) || (regionType == COMPLEXREGION)) {
                FillRgn(hdc, destRgn, (HBRUSH)GetStockObject(BLACK_BRUSH));
            }

            DeleteObject(clientRgn);
            DeleteObject(targetWindowRgn);
            DeleteObject(destRgn);

            // Draw frame around valid virtual screen
            RECT frameRect = virtualScreenRectWithoutRokidMax;
            OffsetRect(&frameRect, -sourceRect.left, -sourceRect.top);

            FrameRect(hdc, &frameRect, (HBRUSH)GetStockObject(GRAY_BRUSH));

            SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            CURSORINFO cursor = { sizeof(cursor) };
            GetCursorInfo(&cursor);

            if (cursor.flags == CURSOR_SHOWING) {
                ICONINFO info = { sizeof(info) };
                GetIconInfo(cursor.hCursor, &info);
                const int x = cursor.ptScreenPos.x - sourceRect.left - info.xHotspot;
                const int y = cursor.ptScreenPos.y - sourceRect.top - info.yHotspot;
                BITMAP bmpCursor = { 0 };
                GetObject(info.hbmColor, sizeof(bmpCursor), &bmpCursor);
                DrawIconEx(hdc, x, y, cursor.hCursor, bmpCursor.bmWidth, bmpCursor.bmHeight,
                    0, NULL, DI_NORMAL);
            }

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
    case WM_WINDOWPOSCHANGED:
    case WM_SIZE:
        // change size of source rect
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        sourceRect.right = sourceRect.left + clientRect.right - clientRect.left;
        sourceRect.bottom = sourceRect.top + clientRect.bottom - clientRect.top;
        break;
    case WM_MYMESSAGE:
        // Handle taskbar icon events
        switch (lParam)
        {
        case WM_LBUTTONDBLCLK:
            // Restore window on double click
            ShowWindow(hWnd, SW_RESTORE);
            break;
        case WM_RBUTTONDOWN:
            // Show a simple menu on right click
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, 3, L"Properties");
            AppendMenu(hMenu, MF_STRING, 2, L"About");
            AppendMenu(hMenu, MF_STRING, 1, L"Exit");
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hWnd);
            TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);
            break;
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
    {
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDHOMEPAGE) {
            ShellExecute(0, 0, L"https://github-nico-code.github.io/Vardian/", 0, 0, SW_SHOW);
        }
        else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Meldungshandler für Properties-Dialog.
INT_PTR CALLBACK Properties(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BTN_PROP_OK || LOWORD(wParam) == IDC_BTN_PROP_CANCEL )
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
    std::wstring szDevice = L"";
    DEVMODE DevMode = {};
};


std::string ws2s(const wchar_t* pcs)
{
    int res;
    char buf[0x400];
    // clear buffer
    memset(buf, 0, 0x400);
    char* pbuf = buf;
    std::shared_ptr<char[]> shared_pbuf;

    res = WideCharToMultiByte(CP_ACP, 0, pcs, -1, buf, sizeof(buf), NULL, NULL);

    if (0 == res && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        res = WideCharToMultiByte(CP_ACP, 0, pcs, -1, NULL, 0, NULL, NULL);

        shared_pbuf = std::shared_ptr<char[]>(new char[res]);

        pbuf = shared_pbuf.get();

        res = WideCharToMultiByte(CP_ACP, 0, pcs, -1, pbuf, res, NULL, NULL);

        return std::string(pbuf);
    }
    else if (0 == res)
    {
        return std::string("´ws2s_ERROR");
    }
    else {
        return std::string(pbuf);
    }
}
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

                std::string the_out;

                the_out += std::string("Monitor Friendly Name : '") + ws2s( deviceName.monitorFriendlyDeviceName ) + "'\n";
                the_out += std::string("Monitor Device Path   : '") + ws2s( deviceName.monitorDevicePath ) + "'\n";

                DISPLAYCONFIG_SOURCE_DEVICE_NAME sourceName = {};
                sourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
                sourceName.header.size = sizeof(DISPLAYCONFIG_SOURCE_DEVICE_NAME);
                sourceName.header.adapterId = paths[i].targetInfo.adapterId;
                sourceName.header.id = paths[i].sourceInfo.id;
                if (DisplayConfigGetDeviceInfo((DISPLAYCONFIG_DEVICE_INFO_HEADER*)&sourceName))
                    continue;

                the_out += std::string("GDI Device Name       : '") + ws2s(sourceName.viewGdiDeviceName) + "'\n";

                // find the monitor with this device name
                auto element = monitors.find(sourceName.viewGdiDeviceName);
                if (element != monitors.end()) {
                    the_out += std::string("Monitor Handle        : '") + std::to_string((unsigned long long)element->second.rokid_handle) + "'\n";

                    U_LOG(U_LOGGING_TRACE, the_out.c_str());


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

        LONG x_size = sourceRect.right - sourceRect.left;
        LONG y_size = sourceRect.bottom - sourceRect.top;

        // only move in 50 pixel steps
        if (abs(collect_y_movement) > y_threshold) {
            sourceRect.left -= collect_y_movement;
            collect_y_movement = 0;

            // Don't scroll outside whole virtual desktop area.
            if (sourceRect.left < virtualScreenRectWithoutRokidMax.left - x_size / 3)
            {
                sourceRect.left = virtualScreenRectWithoutRokidMax.left - x_size / 3;
            }
            // do not move in Rokid Max Screen with copy area
            else if (sourceRect.left > virtualScreenRectWithoutRokidMax.right - x_size + x_size / 3)
            {
                sourceRect.left = virtualScreenRectWithoutRokidMax.right - x_size + x_size / 3;
            }

            sourceRect.right = sourceRect.left + x_size;
        }

        if (abs(collect_x_movement) > x_threshold) {
            sourceRect.top -= collect_x_movement;
            collect_x_movement = 0;

            if (sourceRect.top < virtualScreenRectWithoutRokidMax.top - y_size / 3)
            {
                sourceRect.top = virtualScreenRectWithoutRokidMax.top - y_size / 3;
            }
            else if (sourceRect.top > virtualScreenRectWithoutRokidMax.bottom - y_size + y_size / 3)
            {
                sourceRect.top = virtualScreenRectWithoutRokidMax.bottom - y_size + y_size / 3;
            }
            sourceRect.bottom = sourceRect.top + y_size;
        }

        // Force redraw.
        InvalidateRect(hWnd, NULL, FALSE);

        // Reclaim topmost status, to prevent unmagnified menus from remaining in view. 
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
    }
}


bool InitializeRokidWindow(HWND hWnd) {
    std::string the_out;

    if (running) {
        KillTimer(NULL, timerId);
        timerId = NULL;

        if (rokid_device != NULL) {
            rokid_device[0].destroy(rokid_device);
            rokid_device = NULL;
        }


        // shrink window size to 0x0
        SetWindowPos(hWnd, 0, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SW_SHOWMINIMIZED);

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
        U_LOG(U_LOGGING_TRACE, "Rokid Max USB device found.");

    }
    else {
        U_LOG(U_LOGGING_ERROR, "Did not find Rokid Max.");
        return false;
    }

    // get all paths and figure out monitor handle for "Rokid Max"
    struct monitor_struct_typ monitor_struct;

    // find Rokid Max and move Rokid Max monitor to most right
    if (get_rokid_monitor_handle(monitor_struct) == false) {
        U_LOG(U_LOGGING_ERROR, "Could not find Rokid Max Monitor handle.");
        return false;
    }

    U_LOG(U_LOGGING_TRACE, "Rokid Max Monitor handle found.");

    // call update window after creating window and maybe rokid handles
    if (UpdateWindow(hWnd) == false) {
        U_LOG(U_LOGGING_ERROR, "'UpdateWindow' failed.");
        return false;
    }

    U_LOG(U_LOGGING_TRACE, "'UpdateWindow' successful.");

    ShowWindow(hWnd, SW_SHOWNORMAL);

    // change source rect size, because show window could be changed
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    sourceRect.right = sourceRect.left + clientRect.right - clientRect.left;
    sourceRect.bottom = sourceRect.top + clientRect.bottom - clientRect.top;

    // reset values for virtual screen
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    unsigned int vScreenWidth = GetSystemMetricsForDpi(SM_CXVIRTUALSCREEN, 96 /* 100% scaling*/);
    unsigned int vScreenHeight = GetSystemMetricsForDpi(SM_CYVIRTUALSCREEN, 96 /* 100% scaling*/);

    virtualScreenRectWithoutRokidMax.left = GetSystemMetricsForDpi(SM_XVIRTUALSCREEN, 96);
    virtualScreenRectWithoutRokidMax.top = GetSystemMetricsForDpi(SM_YVIRTUALSCREEN, 96);
    virtualScreenRectWithoutRokidMax.right = GetSystemMetrics(SM_XVIRTUALSCREEN) + vScreenWidth - monitor_struct.DevMode.dmPelsWidth;
    virtualScreenRectWithoutRokidMax.bottom = GetSystemMetrics(SM_YVIRTUALSCREEN) + vScreenHeight;

    the_out = std::string("Main Window Rectangle: ") +
        " virtualScreenRectWithoutRokidMax left, top, width x heigth:  " + std::to_string(virtualScreenRectWithoutRokidMax.left) +
        ", " + std::to_string(virtualScreenRectWithoutRokidMax.top) +
        ", " + std::to_string(virtualScreenRectWithoutRokidMax.right - virtualScreenRectWithoutRokidMax.left) +
        " x " + std::to_string(virtualScreenRectWithoutRokidMax.bottom - virtualScreenRectWithoutRokidMax.top);
    U_LOG(U_LOGGING_TRACE, the_out.c_str());

    // prevent Window from being copied
    SetWindowDisplayAffinity(hWnd, WDA_EXCLUDEFROMCAPTURE);

    if (SetWindowPos(hWnd, HWND_TOPMOST, monitor_struct.DevMode.dmPosition.x, monitor_struct.DevMode.dmPosition.y,
        // I do not know why the size 3840 has to be halved
        (monitor_struct.DevMode.dmPelsWidth==3840)?monitor_struct.DevMode.dmPelsWidth / 2: monitor_struct.DevMode.dmPelsWidth,
        monitor_struct.DevMode.dmPelsHeight,
        SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOACTIVATE) == 0)
    {
        U_LOG(U_LOGGING_ERROR, "Could not SetWindowPos to pos (%i,%i) with size (%i, %i).",
            monitor_struct.DevMode.dmPosition.x, monitor_struct.DevMode.dmPosition.y,
            monitor_struct.DevMode.dmPelsWidth, monitor_struct.DevMode.dmPelsHeight);
        return false;
    }

    U_LOG(U_LOGGING_TRACE, "SetWindowPos to pos (%i,%i) with size (%i, %i).",
        monitor_struct.DevMode.dmPosition.x, monitor_struct.DevMode.dmPosition.y,
        monitor_struct.DevMode.dmPelsWidth, monitor_struct.DevMode.dmPelsHeight);

    hdcWindow = GetDC(hWnd);
    hdcScreen = GetDC(NULL);

    // Create a timer to update the control. But only if Rokid Max is connected
    timerId = SetTimer(hWnd, 0, timerInterval, UpdateRokidWindow);

    U_LOG(U_LOGGING_TRACE, "Timer started.");

    running = true;

    return true;
}

// Function to add taskbar icon
void AddTaskbarIcon()
{
    // Initialize NOTIFYICONDATA structure
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = IDI_MYICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_MYMESSAGE;
    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_VARDIAN));
    lstrcpy(nid.szTip, L"Right mouse click for menu.");

    // Add the icon
    Shell_NotifyIcon(NIM_ADD, &nid);
}

// Function to remove taskbar icon
void RemoveTaskbarIcon()
{
    // Remove the icon
    Shell_NotifyIcon(NIM_DELETE, &nid);
}