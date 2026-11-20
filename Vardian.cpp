// Vardian.cpp : Definiert den Einstiegspunkt für die Anwendung.
// 
// Source code created with the help of ChatGPT. It was a great help for me.
//

#include "framework.h"
#include "Vardian.h"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <locale>
#include <codecvt>
#include <shellapi.h>
#include "helpers.h"
#include "Rokid.h"
#include "ViewportTracking.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cwchar>
#include <cwctype>
#include <iterator>
#include <regex>
#include "easylogging++.h"
#include <vector>
#include <valarray>

INITIALIZE_EASYLOGGINGPP

#define MAX_LOADSTRING 100

#define WM_MYMESSAGE (WM_USER + 100)

constexpr UINT ID_TRAY_EXIT = 1;
constexpr UINT ID_TRAY_ABOUT = 2;
constexpr UINT ID_TRAY_SETTINGS = 3;
constexpr UINT ID_TRAY_RECENTER = 4;
constexpr int ID_HOTKEY_RECENTER = 1;

constexpr int IDC_HORIZONTAL_SENSITIVITY = 2001;
constexpr int IDC_VERTICAL_SENSITIVITY = 2002;
constexpr int IDC_SMOOTHING = 2003;
constexpr int IDC_DEAD_ZONE = 2004;
constexpr int IDC_LOCK_VERTICAL = 2005;
constexpr int IDC_APPLY_TRACKING = 2006;
constexpr int IDC_RECENTER_TRACKING = 2007;

constexpr wchar_t TRACKING_SETTINGS_WINDOW_CLASS[] = L"VardianTrackingSettings";

//! Helper define to make code more readable.
#define U_1_000_000_000 (1000 * 1000 * 1000)

static inline uint64_t
get_ns(void) noexcept
{
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);

    static int64_t qpc_frequency = 0;
    if (qpc_frequency == 0) {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        qpc_frequency = freq.QuadPart;
    }

    return static_cast<uint64_t>(
        (static_cast<long double>(qpc.QuadPart) * U_1_000_000_000) /
        static_cast<long double>(qpc_frequency));
}



// Globale Variablen:
HINSTANCE hInst;                                // Aktuelle Instanz
WCHAR szTitle[MAX_LOADSTRING];                  // Titelleistentext
WCHAR szWindowClass[MAX_LOADSTRING];            // Der Klassenname des Hauptfensters.
Rokid rokid_device;  // Pointer to Rokid Max
UINT_PTR timerId = 0;                           // timer to copy part of screen to Rokid Max
uint64_t lastViewportUpdateTimestamp = 0;
constexpr UINT timerInterval = 16;                  // close to the refresh rate @60hz
RECT virtualScreenRectWithoutRokidMax;          // values of the virtual screen without the X axis from Rokid Max
//HWND hWnd = NULL;                               // main window
bool running = false;
RECT sourceRect = { 500, 500, 2420, 1580 };
HDC hdcScreen = NULL;
HWND mainWindow = NULL;
ViewportTrackingState viewportTracking;
std::wstring trackingIniPath;
HWND trackingSettingsWindow = NULL;

// Declare global variables
NOTIFYICONDATA nid;

// Vorwärtsdeklarationen der in diesem Codemodul enthaltenen Funktionen:
ATOM                MyRegisterClass(HINSTANCE hInstance) noexcept;
BOOL                InitInstance(HINSTANCE, int, HWND& hWnd);
BOOL                DeInitInstance();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM) noexcept;
LRESULT CALLBACK    TrackingSettingsWndProc(HWND, UINT, WPARAM, LPARAM);
bool                InitializeRokidWindow(HWND hWnd);
void                AddTaskbarIcon(HWND hWnd) noexcept;
void                RemoveTaskbarIcon() noexcept;
void                ShowTrackingSettingsWindow(HWND owner);
void                RecenterViewport() noexcept;

std::wstring GetTrackingIniPath()
{
    wchar_t module_path[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(NULL, module_path, MAX_PATH);
    std::wstring path(module_path, length);
    const size_t separator = path.find_last_of(L"\\/");
    if (separator != std::wstring::npos) {
        path.resize(separator + 1);
    }
    else {
        path.clear();
    }
    path += L"vardian.ini";
    return path;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    wchar_t my_filename[MAX_PATH]; 
    GetModuleFileName(NULL, my_filename, MAX_PATH);

    std::wstring my_filename_str(my_filename);
    my_filename_str += L".err.log";

    el::Configurations defaultConf;
    defaultConf.setToDefault();
    // Values are always std::string
    defaultConf.set(el::Level::Global,
        el::ConfigurationType::Format, "%datetime %fbase %func %level >> %msg");
    defaultConf.set(el::Level::Global, el::ConfigurationType::ToFile, "true");
    defaultConf.set(el::Level::Global, el::ConfigurationType::ToStandardOutput, "true");
    defaultConf.set(el::Level::Global, el::ConfigurationType::Filename, ws2s(my_filename_str) );
    defaultConf.set(el::Level::Global, el::ConfigurationType::MaxLogFileSize, "2097152");

    // default logger uses default configurations
    el::Loggers::reconfigureLogger("default", defaultConf);


    // TODO: Hier Code einfügen.
    LOG(INFO) << "Start Vardian";

    trackingIniPath = GetTrackingIniPath();
    viewportTracking.set_settings(load_viewport_tracking_settings(trackingIniPath));

    // Globale Zeichenfolgen initialisieren
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_VARDIAN, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    HWND hWnd = nullptr;

    // Anwendungsinitialisierung ausführen:
    if (!InitInstance (hInstance, nCmdShow, hWnd))
    {
        return FALSE;
    }

    if (!RegisterHotKey(hWnd, ID_HOTKEY_RECENTER,
        MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, L'R')) {
        LOG(WARNING) << "Could not register Ctrl+Alt+R recenter hotkey: "
            << getErrorCodeDescription(GetLastError());
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_VARDIAN));

    // Find Rokid Max device and start screen copy functionality
    InitializeRokidWindow(hWnd);
 
    // Add taskbar icon
    AddTaskbarIcon(hWnd);

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

    LOG(INFO) << "Finish Vardian";

    return (int) msg.wParam;
}



//
//  FUNKTION: MyRegisterClass()
//
//  ZWECK: Registriert die Fensterklasse.
//
ATOM MyRegisterClass(HINSTANCE hInstance) noexcept
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
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, HWND& hWnd)
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
       LOG(ERROR) << "Could not create main window.";
       return FALSE;
   }

   mainWindow = hWnd;

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
    if (mainWindow != NULL && timerId != 0) {
        KillTimer(mainWindow, timerId);
        timerId = 0;
    }
    rokid_device.stop();

    return TRUE;
}

/// <summary>
   /// Get Intersection point
   /// </summary>
   /// <param name="a1">a1 is line1 start</param>
   /// <param name="a2">a2 is line1 end</param>
   /// <param name="b1">b1 is line2 start</param>
   /// <param name="b2">b2 is line2 end</param>
   /// <returns></returns>
bool Intersects(const POINT& p0, const POINT& p1, const POINT& p2, const POINT& p3, POINT& i)
{
        double denom, s_numer, t_numer, t;

        LONG s10_x, s10_y, s32_x, s32_y, s02_x, s02_y;
        s10_x = p1.x - p0.x;
        s10_y = p1.y - p0.y;
        s32_x = p3.x - p2.x;
        s32_y = p3.y - p2.y;

        denom = s10_x * s32_y - s32_x * s10_y;
        if (denom == 0)
            return false; // Collinear
        bool denomPositive = denom > 0;

        s02_x = p0.x - p2.x;
        s02_y = p0.y - p2.y;
        s_numer = s10_x * s02_y - s10_y * s02_x;
        if ((s_numer < 0) == denomPositive)
            return false; // No collision

        t_numer = s32_x * s02_y - s32_y * s02_x;
        if ((t_numer < 0) == denomPositive)
            return false; // No collision

        if (((s_numer > denom) == denomPositive) || ((t_numer > denom) == denomPositive))
            return false; // No collision
        // Collision detected
        t = t_numer / denom;
        i.x = p0.x + (t * s10_x);
        i.y = p0.y + (t * s10_y);

        return true;
}

bool Intersects_Once(const POINT& a1, const POINT& a2, const RECT& rect, POINT& result, bool& resultTop, bool& resultBottom, bool& resultLeft, bool& resultRight ) {
    POINT left_top = { rect.left, rect.top };
    POINT left_bottom = { rect.left, rect.bottom };
    POINT right_top = { rect.right, rect.top };
    POINT right_bottom = { rect.right, rect.bottom };


    // test all four edges of the rectangle
    resultLeft = Intersects(a1, a2, left_top, left_bottom, result);
    resultBottom = Intersects(a1, a2, left_bottom, right_bottom, result);
    resultRight = Intersects(a1, a2, right_bottom, right_top, result);
    resultTop = Intersects(a1, a2, right_top, left_top, result);
    if (resultTop xor resultBottom xor resultLeft xor resultRight ) {
        return true;
    }
    else {
        return false;
    }
}

struct MYICON_INFO
{
    int     nWidth;
    int     nHeight;
    int     nBitsPerPixel;
};

MYICON_INFO MyGetIconInfo(HICON hIcon);

// =======================================

MYICON_INFO MyGetIconInfo(HICON hIcon)
{
    MYICON_INFO myinfo;
    ZeroMemory(&myinfo, sizeof(myinfo));

    ICONINFO info;
    ZeroMemory(&info, sizeof(info));

    BOOL bRes = FALSE;

    bRes = GetIconInfo(hIcon, &info);
    if (!bRes)
        return myinfo;

    BITMAP bmp;
    ZeroMemory(&bmp, sizeof(bmp));

    if (info.hbmColor)
    {
        const int nWrittenBytes = GetObject(info.hbmColor, sizeof(bmp), &bmp);
        if (nWrittenBytes > 0)
        {
            myinfo.nWidth = bmp.bmWidth;
            myinfo.nHeight = bmp.bmHeight;
            myinfo.nBitsPerPixel = bmp.bmBitsPixel;
        }
    }
    else if (info.hbmMask)
    {
        // Icon has no color plane, image data stored in mask
        const int nWrittenBytes = GetObject(info.hbmMask, sizeof(bmp), &bmp);
        if (nWrittenBytes > 0)
        {
            myinfo.nWidth = bmp.bmWidth;
            myinfo.nHeight = bmp.bmHeight / 2;
            myinfo.nBitsPerPixel = 1;
        }
    }

    if (info.hbmColor)
        DeleteObject(info.hbmColor);
    if (info.hbmMask)
        DeleteObject(info.hbmMask);

    return myinfo;
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
            const int wmId = LOWORD(wParam);
            // Menüauswahl analysieren:
            switch (wmId)
            {
            case ID_TRAY_EXIT:
                // Exit the program
                DestroyWindow(hWnd);
                break;
            case ID_TRAY_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case ID_TRAY_SETTINGS:
                ShowTrackingSettingsWindow(hWnd);
                break;
            case ID_TRAY_RECENTER:
                RecenterViewport();
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
                const BOOL bitblt_result = BitBlt(hdc,
                    intersectClientRect.left, intersectClientRect.top,
                    intersectClientRect.right, intersectClientRect.bottom,
                    hdcScreen,
                    intersectSource.left, intersectSource.top,
                    SRCCOPY);
                if (!bitblt_result) {
                    std::string the_out = std::string("BitBlt failed: ") +
                        " intersectSource.left, top, width x heigt:  " + std::to_string(intersectSource.left) +
                        ", " + std::to_string(intersectSource.top) +
                        ", " + std::to_string(intersectSource.right - intersectSource.left) + " x " + std::to_string(intersectSource.bottom - intersectSource.top) +
                        " intersectClientRect.left, top, width x height  " + std::to_string(intersectClientRect.left) +
                        ", " + std::to_string(intersectClientRect.top) +
                        ", " + std::to_string(intersectSize.cx) + " x " + std::to_string(intersectSize.cy) +
                        " ps.rcPaint.left, top, width x height  " + std::to_string(ps.rcPaint.left) +
                        ", " + std::to_string(ps.rcPaint.top) +
                        ", " + std::to_string(ps.rcPaint.right - ps.rcPaint.left) + " x " + std::to_string(ps.rcPaint.bottom - ps.rcPaint.top) +
                        ", GetLastError: " + getErrorCodeDescription(GetLastError());
                    LOG(ERROR) << the_out.c_str();
                }

                HRGN clientRgn = CreateRectRgnIndirect(&intersectClientRect);
                HRGN targetWindowRgn = CreateRectRgnIndirect(&ps.rcPaint);
                HRGN destRgn = CreateRectRgn(0, 0, 1, 1);

                // we only have to fill a region if it is a COMPLEXREGION or a simple rect
                const int regionType = CombineRgn(destRgn, targetWindowRgn, clientRgn, RGN_XOR);

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
                if (GetCursorInfo(&cursor) == FALSE) {
                    LOG(ERROR) << std::format("GetCursorInfo failed with error: {}",
                        getErrorCodeDescription(GetLastError()));
                }
                
                if (cursor.flags == CURSOR_SHOWING) {
                    ICONINFO info = { sizeof(info) };
                    if (GetIconInfo(cursor.hCursor, &info) == FALSE) {
                        LOG(ERROR) << std::format("GetIconInfo failed with error: {}",
                            getErrorCodeDescription(GetLastError()));
                    }

                    const int x = cursor.ptScreenPos.x - sourceRect.left - info.xHotspot;
                    const int y = cursor.ptScreenPos.y - sourceRect.top - info.yHotspot;

                    // is the cursor inside drawing area
                    if ((cursor.ptScreenPos.x >= intersectSource.left) and (cursor.ptScreenPos.x <= intersectSource.right) and
                        (cursor.ptScreenPos.y >= intersectSource.top) and (cursor.ptScreenPos.y <= intersectSource.bottom)) {
                        // mouse inside drawing area ==> draw cursor bitmap
                        BITMAP bmpCursor = { 0 };

                        const int result = GetObject(info.hbmColor, sizeof(bmpCursor), &bmpCursor);
                        if (result == 0) {
                            const DWORD last_error = GetLastError();
                            if (last_error != 0) {
                                // it seams to be 0 if cursor bitmap was already there - I do not know
                                LOG(ERROR) << std::format("GetObject for Cursor bitmap failed with error: {}",
                                    getErrorCodeDescription(GetLastError()));
                            }
                        }
                        if (DrawIconEx(hdc, x, y, cursor.hCursor, bmpCursor.bmWidth, bmpCursor.bmHeight,
                            0, NULL, DI_NORMAL) == 0)
                        {
                            LOG(ERROR) << std::format("DrawIconEx failed with error: {}",
                                getErrorCodeDescription(GetLastError()));
                        }
                    }
                    else {
                        // mouse curoor outsite drawing area ==> draw direction mouse cursor dependend on position of mouse outside sourceRect
                        // https://learn.microsoft.com/en-us/windows/win32/menurc/about-cursors
                        // calculate middle of sourceRect
                        POINT middlePoint;
                        middlePoint.x = intersectSource.left + (intersectSource.right - intersectSource.left) / 2;
                        middlePoint.y = intersectSource.top + (intersectSource.bottom - intersectSource.top) / 2;

                        POINT mousePoint;
                        mousePoint.x = cursor.ptScreenPos.x - info.xHotspot;
                        mousePoint.y = cursor.ptScreenPos.y - info.yHotspot;

                        POINT intersectionPoint;
                        bool resultTop = false;
                        bool resultBottom = false;
                        bool resultLeft = false;
                        bool resultRight = false;

                        if (Intersects_Once(middlePoint, mousePoint, intersectSource, intersectionPoint, resultTop, resultBottom, resultLeft, resultRight )) {
                            const LONG imagepos_x = intersectionPoint.x - sourceRect.left;
                            const LONG imagepos_y = intersectionPoint.y - sourceRect.top;
                            LONG mult_iconsize_x = 0;
                            LONG mult_iconsize_y = 0;

                            LPWSTR icon_resource_string = NULL;


                            // found one intersection point and intersected rectangle side
                            if (resultLeft) {
                                icon_resource_string = MAKEINTRESOURCE(IDI_ISLEFT);
                                mult_iconsize_x = 1; // upper left corner from icon is the size of the icon away from left border
                                mult_iconsize_y = 0;
                            } else if (resultRight) {
                                icon_resource_string = MAKEINTRESOURCE(IDI_ISRIGHT);
                                mult_iconsize_x = -2; // upper left corner from icon is double the size of the icon away from right border
                                mult_iconsize_y = 0;
                            } else if (resultTop) {
                                icon_resource_string = MAKEINTRESOURCE(IDI_ISTOP);
                                mult_iconsize_x = 0; // upper left corner from icon is double the size of the icon away from right border
                                mult_iconsize_y = 1;
                            } else if (resultBottom) {
                                icon_resource_string = MAKEINTRESOURCE(IDI_ISBOTTOM);
                                mult_iconsize_x = 0; // upper left corner from icon is double the size of the icon away from right border
                                mult_iconsize_y = -2;
                            }

                            HICON new_icon = LoadIcon(hInst, icon_resource_string);
                            const MYICON_INFO icon_info = MyGetIconInfo(new_icon);

                            if (new_icon == NULL)
                            {
                                LOG(ERROR) << std::format("LoadIcon failed with error: {}",
                                    getErrorCodeDescription(GetLastError()));
                            }

                            if (DrawIconEx(hdc, imagepos_x + mult_iconsize_x * icon_info.nWidth, imagepos_y + mult_iconsize_y * icon_info.nHeight,
                                new_icon, 0, 0,
                                0, NULL, DI_NORMAL) == 0)
                            {
                                LOG(ERROR) << std::format("DrawIconEx failed with error: {}",
                                    getErrorCodeDescription(GetLastError()));
                            }
                        }
                    }
                    DeleteObject(info.hbmColor);
                    DeleteObject(info.hbmMask);
                }

                EndPaint(hWnd, &ps);
        }
        break;
    case WM_HOTKEY:
        if (wParam == ID_HOTKEY_RECENTER) {
            RecenterViewport();
        }
        break;
    case WM_DESTROY:
        UnregisterHotKey(hWnd, ID_HOTKEY_RECENTER);
        if (trackingSettingsWindow != NULL) {
            DestroyWindow(trackingSettingsWindow);
        }
        PostQuitMessage(0);
        break;
    case WM_DISPLAYCHANGE:
        // A monitor could be removed
        // wait some time to take care of starting Rokid Max USB
        
        // wait some time and initialize window
        std::this_thread::sleep_for(std::chrono::seconds(2));
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
        {
            // Show a simple menu on right click
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, ID_TRAY_SETTINGS, L"Tracking settings...");
            AppendMenu(hMenu, MF_STRING, ID_TRAY_RECENTER, L"Recenter\tCtrl+Alt+R");
            AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hMenu, MF_STRING, ID_TRAY_ABOUT, L"About");
            AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hWnd);
            TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);
            break;
        }
        default: {} // nothing
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Meldungshandler für Infofeld.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) noexcept
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
    default: {} // nothing
    }
    return (INT_PTR)FALSE;
}

namespace
{
void set_default_control_font(HWND control) noexcept
{
    SendMessageW(control, WM_SETFONT,
        reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
}

HWND create_settings_control(HWND parent, const wchar_t* class_name,
    const wchar_t* text, DWORD style, int x, int y, int width, int height, int id)
{
    HWND control = CreateWindowExW(
        wcscmp(class_name, L"EDIT") == 0 ? WS_EX_CLIENTEDGE : 0,
        class_name, text, WS_CHILD | WS_VISIBLE | style,
        x, y, width, height, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        hInst, nullptr);
    if (control != NULL) {
        set_default_control_font(control);
    }
    return control;
}

void set_double_control(HWND window, int control_id, double value) noexcept
{
    wchar_t text[64]{};
    swprintf_s(text, L"%.6g", value);
    SetDlgItemTextW(window, control_id, text);
}

bool read_double_control(HWND window, int control_id, const wchar_t* display_name,
    double minimum, double maximum, double& value) noexcept
{
    wchar_t text[128]{};
    GetDlgItemTextW(window, control_id, text, static_cast<int>(std::size(text)));

    wchar_t* end = nullptr;
    const double parsed = std::wcstod(text, &end);
    while (end != nullptr && std::iswspace(*end)) {
        ++end;
    }

    if (end == text || (end != nullptr && *end != L'\0') || !std::isfinite(parsed) ||
        parsed < minimum || parsed > maximum) {
        const std::wstring message = std::wstring(display_name) + L" must be between " +
            std::to_wstring(minimum) + L" and " + std::to_wstring(maximum) + L".";
        MessageBoxW(window, message.c_str(), L"Invalid tracking setting", MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(window, control_id));
        return false;
    }

    value = parsed;
    return true;
}

void populate_tracking_settings(HWND window) noexcept
{
    const auto& settings = viewportTracking.settings();
    set_double_control(window, IDC_HORIZONTAL_SENSITIVITY, settings.horizontal_sensitivity);
    set_double_control(window, IDC_VERTICAL_SENSITIVITY, settings.vertical_sensitivity);
    set_double_control(window, IDC_SMOOTHING, settings.smoothing);
    set_double_control(window, IDC_DEAD_ZONE, settings.dead_zone);
    CheckDlgButton(window, IDC_LOCK_VERTICAL,
        settings.lock_vertical ? BST_CHECKED : BST_UNCHECKED);
}

bool apply_tracking_settings(HWND window) noexcept
{
    ViewportTrackingSettings settings = viewportTracking.settings();
    if (!read_double_control(window, IDC_HORIZONTAL_SENSITIVITY,
            L"Horizontal sensitivity", 0.0, 10.0, settings.horizontal_sensitivity) ||
        !read_double_control(window, IDC_VERTICAL_SENSITIVITY,
            L"Vertical sensitivity", 0.0, 10.0, settings.vertical_sensitivity) ||
        !read_double_control(window, IDC_SMOOTHING,
            L"Smoothing", 0.01, 1.0, settings.smoothing) ||
        !read_double_control(window, IDC_DEAD_ZONE,
            L"Dead zone", 0.0, 10000.0, settings.dead_zone)) {
        return false;
    }

    settings.lock_vertical = IsDlgButtonChecked(window, IDC_LOCK_VERTICAL) == BST_CHECKED;
    viewportTracking.set_settings(settings);

    if (!save_viewport_tracking_settings(trackingIniPath, settings)) {
        LOG(WARNING) << "Could not save tracking settings to " << ws2s(trackingIniPath);
        MessageBoxW(window,
            L"The settings are active, but vardian.ini could not be saved.",
            L"Vardian", MB_OK | MB_ICONWARNING);
    }
    return true;
}
}

LRESULT CALLBACK TrackingSettingsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        create_settings_control(hWnd, L"STATIC", L"Horizontal sensitivity (0 - 10):",
            SS_LEFT, 16, 18, 220, 20, 0);
        create_settings_control(hWnd, L"EDIT", L"", ES_AUTOHSCROLL,
            250, 15, 120, 24, IDC_HORIZONTAL_SENSITIVITY);

        create_settings_control(hWnd, L"STATIC", L"Vertical sensitivity (0 - 10):",
            SS_LEFT, 16, 52, 220, 20, 0);
        create_settings_control(hWnd, L"EDIT", L"", ES_AUTOHSCROLL,
            250, 49, 120, 24, IDC_VERTICAL_SENSITIVITY);

        create_settings_control(hWnd, L"STATIC", L"Smoothing (0.01 - 1.0):",
            SS_LEFT, 16, 86, 220, 20, 0);
        create_settings_control(hWnd, L"EDIT", L"", ES_AUTOHSCROLL,
            250, 83, 120, 24, IDC_SMOOTHING);

        create_settings_control(hWnd, L"STATIC", L"Dead zone (pixels/second):",
            SS_LEFT, 16, 120, 220, 20, 0);
        create_settings_control(hWnd, L"EDIT", L"", ES_AUTOHSCROLL,
            250, 117, 120, 24, IDC_DEAD_ZONE);

        create_settings_control(hWnd, L"BUTTON", L"Lock vertical movement",
            BS_AUTOCHECKBOX | WS_TABSTOP, 16, 155, 240, 24, IDC_LOCK_VERTICAL);

        create_settings_control(hWnd, L"BUTTON", L"Apply",
            BS_DEFPUSHBUTTON | WS_TABSTOP, 16, 205, 100, 28, IDC_APPLY_TRACKING);
        create_settings_control(hWnd, L"BUTTON", L"Recenter",
            BS_PUSHBUTTON | WS_TABSTOP, 126, 205, 100, 28, IDC_RECENTER_TRACKING);
        create_settings_control(hWnd, L"BUTTON", L"Close",
            BS_PUSHBUTTON | WS_TABSTOP, 270, 205, 100, 28, IDCANCEL);

        populate_tracking_settings(hWnd);
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_APPLY_TRACKING:
            apply_tracking_settings(hWnd);
            return 0;
        case IDC_RECENTER_TRACKING:
            RecenterViewport();
            return 0;
        case IDCANCEL:
            DestroyWindow(hWnd);
            return 0;
        default:
            break;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;
    case WM_DESTROY:
        trackingSettingsWindow = NULL;
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}

void ShowTrackingSettingsWindow(HWND owner)
{
    if (trackingSettingsWindow != NULL) {
        ShowWindow(trackingSettingsWindow, SW_SHOWNORMAL);
        SetForegroundWindow(trackingSettingsWindow);
        return;
    }

    static bool class_registered = false;
    if (!class_registered) {
        WNDCLASSEXW window_class{};
        window_class.cbSize = sizeof(window_class);
        window_class.lpfnWndProc = TrackingSettingsWndProc;
        window_class.hInstance = hInst;
        window_class.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_VARDIAN));
        window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
        window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        window_class.lpszClassName = TRACKING_SETTINGS_WINDOW_CLASS;
        class_registered = RegisterClassExW(&window_class) != 0;
        if (!class_registered && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            LOG(ERROR) << "Could not register tracking settings window: "
                << getErrorCodeDescription(GetLastError());
            return;
        }
        class_registered = true;
    }

    trackingSettingsWindow = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        TRACKING_SETTINGS_WINDOW_CLASS, L"Vardian tracking settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 405, 285,
        owner, nullptr, hInst, nullptr);

    if (trackingSettingsWindow == NULL) {
        LOG(ERROR) << "Could not create tracking settings window: "
            << getErrorCodeDescription(GetLastError());
        return;
    }

    ShowWindow(trackingSettingsWindow, SW_SHOWNORMAL);
    SetForegroundWindow(trackingSettingsWindow);
}

void RecenterViewport() noexcept
{
    if (rokid_device.is_running()) {
        double discarded_x = 0.0;
        double discarded_y = 0.0;
        double discarded_z = 0.0;
        rokid_device.get_gyro_angles_since_last_call(discarded_x, discarded_y, discarded_z);
    }

    viewportTracking.recenter();
    LOG(INFO) << "Viewport tracking recentered.";
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

// detaching the monitor is not possible anymore!
static bool get_rokid_monitor_handle(struct monitor_struct_typ& monitor_struct) {
    UINT pathCount = 0;
    UINT modeCount = 0;
    bool rokid_monitor_found = false;
    const LONG retValue = GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount);
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

                    auto monitors = reinterpret_cast<std::unordered_map<std::wstring, struct monitor_struct_typ>*>(lp);
                    monitors->insert(std::pair<std::wstring, struct monitor_struct_typ>(mi.szDevice, tempMonitor));

                    return TRUE;
                }, (LPARAM)&monitors);

            // for each path, get GDI device name and compare with monitor device name
            for (UINT i = 0; i < pathCount; i++)
            {
                DISPLAYCONFIG_TARGET_DEVICE_NAME deviceName = {};
                deviceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
                deviceName.header.size = sizeof(DISPLAYCONFIG_TARGET_DEVICE_NAME);
                deviceName.header.adapterId = paths.at(i).targetInfo.adapterId;
                deviceName.header.id = paths.at(i).targetInfo.id;
                if (DisplayConfigGetDeviceInfo((DISPLAYCONFIG_DEVICE_INFO_HEADER*)&deviceName))
                    continue;

                std::string the_out;

                the_out += std::string("Monitor Friendly Name : '") + ws2s( std::wstring( deviceName.monitorFriendlyDeviceName )) + "'\n";
                the_out += std::string("Monitor Device Path   : '") + ws2s( std::wstring( deviceName.monitorDevicePath )) + "'\n";

                DISPLAYCONFIG_SOURCE_DEVICE_NAME sourceName = {};
                sourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
                sourceName.header.size = sizeof(DISPLAYCONFIG_SOURCE_DEVICE_NAME);
                sourceName.header.adapterId = paths[i].targetInfo.adapterId;
                sourceName.header.id = paths[i].sourceInfo.id;
                if (DisplayConfigGetDeviceInfo(reinterpret_cast<DISPLAYCONFIG_DEVICE_INFO_HEADER*>( & sourceName)))
                    continue;

                the_out += std::string("GDI Device Name       : '") + ws2s(std::wstring( sourceName.viewGdiDeviceName )) + "'\n";

                // find the monitor with this device name
                auto element = monitors.find(sourceName.viewGdiDeviceName);
                if (element != monitors.end()) {
                    the_out += std::string("Monitor Handle        : '") + std::to_string(reinterpret_cast<unsigned long long>(element->second.rokid_handle)) + "'\n";

                    LOG(INFO) << the_out.c_str();

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
void CALLBACK UpdateRokidWindow(HWND hWnd, UINT /*uMsg*/, UINT_PTR /*idEvent*/, DWORD /*dwTime*/) noexcept
{
    // Get the position of the Rokid Max
    if (rokid_device.is_running()) {
        double gyro_x = 0.0;
        double gyro_y = 0.0;
        double gyro_z = 0.0;

        rokid_device.get_gyro_angles_since_last_call(gyro_x, gyro_y, gyro_z);

        const uint64_t timestamp = get_ns();
        double dt_seconds = 1.0 / 60.0;
        if (lastViewportUpdateTimestamp != 0) {
            dt_seconds = static_cast<double>(timestamp - lastViewportUpdateTimestamp) /
                static_cast<double>(U_1_000_000_000);
        }
        lastViewportUpdateTimestamp = timestamp;

        sourceRect = viewportTracking.update(gyro_x, gyro_y, dt_seconds,
            sourceRect, virtualScreenRectWithoutRokidMax);

#if defined(_DEBUG)
        static uint64_t last_debug_timestamp = 0;
        if (timestamp - last_debug_timestamp >= 500000000) {
            const auto& debug = viewportTracking.debug_snapshot();
            LOG(DEBUG) << std::format(
                "Tracking raw yaw/pitch: {:.3f}/{:.3f} px/s, filtered: {:.3f}/{:.3f} px/s, "
                "target: {:.2f}/{:.2f}, current: {:.2f}/{:.2f}",
                debug.raw_yaw, debug.raw_pitch,
                debug.filtered_yaw, debug.filtered_pitch,
                debug.target_x, debug.target_y,
                debug.current_x, debug.current_y);
            last_debug_timestamp = timestamp;
        }
#endif

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
        KillTimer(hWnd, timerId);
        timerId = 0;

        if (rokid_device.is_running()) {
            rokid_device.stop();
        }

        if (ReleaseDC(NULL, hdcScreen) != 1) {
            LOG(ERROR) << std::format("ReleaseDC failed with error: {}", getErrorCodeDescription(GetLastError()));
        }

        hdcScreen = NULL;

        // shrink window size to 0x0
        SetWindowPos(hWnd, 0, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SW_SHOWMINIMIZED);

        running = false;
    }

    if (rokid_device.start()) {
        LOG(INFO) << "Rokid Max USB device found.";
    }
    else {
        LOG(ERROR) << "Did not find Rokid Max.";
        return false;
    }

    // get all paths and figure out monitor handle for "Rokid Max"
    struct monitor_struct_typ monitor_struct;

    // find Rokid Max and move Rokid Max monitor to most right
    if (get_rokid_monitor_handle(monitor_struct) == false) {
        LOG(ERROR) << "Could not find Rokid Max Monitor handle.";
        return false;
    }

    LOG(INFO) << "Rokid Max Monitor handle found.";

    // call update window after creating window and maybe rokid handles
    if (UpdateWindow(hWnd) == false) {
        LOG(ERROR) << "'UpdateWindow' failed.";
        return false;
    }

    LOG(INFO) << "'UpdateWindow' successful.";

    ShowWindow(hWnd, SW_SHOWNORMAL);

    // change source rect size, because show window could be changed
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    sourceRect.right = sourceRect.left + clientRect.right - clientRect.left;
    sourceRect.bottom = sourceRect.top + clientRect.bottom - clientRect.top;

    // reset values for virtual screen
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    const LONG vScreenWidth = GetSystemMetricsForDpi(SM_CXVIRTUALSCREEN, 96 /* 100% scaling*/);
    const LONG vScreenHeight = GetSystemMetricsForDpi(SM_CYVIRTUALSCREEN, 96 /* 100% scaling*/);

    virtualScreenRectWithoutRokidMax.left = GetSystemMetricsForDpi(SM_XVIRTUALSCREEN, 96);
    virtualScreenRectWithoutRokidMax.top = GetSystemMetricsForDpi(SM_YVIRTUALSCREEN, 96);
    virtualScreenRectWithoutRokidMax.right = virtualScreenRectWithoutRokidMax.left +
        vScreenWidth - static_cast<LONG>(monitor_struct.DevMode.dmPelsWidth);
    virtualScreenRectWithoutRokidMax.bottom = virtualScreenRectWithoutRokidMax.top + vScreenHeight;

    the_out = std::string("Main Window Rectangle: ") +
        " virtualScreenRectWithoutRokidMax left, top, width x heigth:  " + std::to_string(virtualScreenRectWithoutRokidMax.left) +
        ", " + std::to_string(virtualScreenRectWithoutRokidMax.top) +
        ", " + std::to_string(virtualScreenRectWithoutRokidMax.right - virtualScreenRectWithoutRokidMax.left) +
        " x " + std::to_string(virtualScreenRectWithoutRokidMax.bottom - virtualScreenRectWithoutRokidMax.top);
    LOG(INFO) << the_out.c_str();

    // prevent Window from being copied
    SetWindowDisplayAffinity(hWnd, WDA_EXCLUDEFROMCAPTURE);

    if (SetWindowPos(hWnd, HWND_TOPMOST, monitor_struct.DevMode.dmPosition.x, monitor_struct.DevMode.dmPosition.y,
        // I do not know why the size 3840 has to be halved
        (monitor_struct.DevMode.dmPelsWidth==3840)?monitor_struct.DevMode.dmPelsWidth / 2: monitor_struct.DevMode.dmPelsWidth,
        monitor_struct.DevMode.dmPelsHeight,
        SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOACTIVATE) == 0)
    {
        LOG(ERROR) << std::format("Could not SetWindowPos to pos ({},{}) with size ({}, {}).",
            monitor_struct.DevMode.dmPosition.x, monitor_struct.DevMode.dmPosition.y,
            monitor_struct.DevMode.dmPelsWidth, monitor_struct.DevMode.dmPelsHeight);
        return false;
    }

    LOG(INFO) << std::format("SetWindowPos to pos ({},{}) with size ({}, {}).",
        monitor_struct.DevMode.dmPosition.x, monitor_struct.DevMode.dmPosition.y,
        monitor_struct.DevMode.dmPelsWidth, monitor_struct.DevMode.dmPelsHeight);

    viewportTracking.reset(sourceRect, virtualScreenRectWithoutRokidMax);
    sourceRect = viewportTracking.update(0.0, 0.0, 1.0 / 60.0,
        sourceRect, virtualScreenRectWithoutRokidMax);
    lastViewportUpdateTimestamp = 0;

    // Create a timer to update the control. But only if Rokid Max is connected
    timerId = SetTimer(hWnd, 0, timerInterval, UpdateRokidWindow);

    if (timerId == 0) {
        // timer creation failed
        LOG(ERROR) << std::format("Timer creation failed with Erro: {}", getErrorCodeDescription(GetLastError()));
        return false;
    }

    LOG(INFO) << "Timer started.";

    hdcScreen = GetDC(NULL);

    if (hdcScreen == NULL) {
        LOG(ERROR) << "GetDC failed.";
        return false;
    }

    running = true;

    return true;
}

// Function to add taskbar icon
void AddTaskbarIcon(HWND hWnd) noexcept
{
    // Initialize NOTIFYICONDATA structure
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = IDI_MYICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_MYMESSAGE;
    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_VARDIAN));
    wcscpy_s(nid.szTip, L"Right mouse click for menu.");

    // Add the icon
    Shell_NotifyIcon(NIM_ADD, &nid);
}

// Function to remove taskbar icon
void RemoveTaskbarIcon() noexcept
{
    // Remove the icon
    Shell_NotifyIcon(NIM_DELETE, &nid);
}
