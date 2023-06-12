#include <windows.h>
#include <cassert>

class App {
    static App *instance;

public:
    App() {
        assert(App::instance == nullptr);
        App::instance = this;
    }

    int run(HINSTANCE instance, PWSTR cmd_line, int cmd_show) {
        // Register the window class.
        constexpr wchar_t class_name[]  = L"NSE Main Window";

        WNDCLASS wc = {};


        wc.lpfnWndProc = window_proc;
        wc.hInstance = instance;
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = class_name;
        wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));

        RegisterClass(&wc);

        HWND hwnd = CreateWindowEx(
            0, // Optional window styles.
            class_name, // Window class
            L"Button tester",
            WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_BORDER,

            // Position and size
            CW_USEDEFAULT, CW_USEDEFAULT, 640u, 480u,

            nullptr,   // Parent window
            nullptr,   // Menu
            instance,  // Instance handle
            nullptr    // Additional application data
            );

        if (hwnd == nullptr) {
            return 1;
        }

        ShowWindow(hwnd, cmd_show);

        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return 0;
    }

private:
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        auto dx = LOWORD(GetDialogBaseUnits());
        auto dy = HIWORD(GetDialogBaseUnits());

        static RECT rect;
        switch (msg) {
        case WM_CREATE:
            App::instance->create_buttons(hwnd, lparam);
            return 0;

        case WM_SIZE:
            rect.left = 24 * dx;
            rect.top = 2 * dy;
            rect.right = LOWORD(lparam);
            rect.bottom = HIWORD(lparam);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT: {
            static auto l1 = TEXT("message                  wParam           lParam");
            static auto l2 = TEXT("_______                   ______            ______");

            PAINTSTRUCT ps;
            auto hdc = BeginPaint(hwnd, &ps);
            SetBkMode(hdc, TRANSPARENT);
            TextOut(hdc, 24 * dx, dy, l1, lstrlen(l1));
            TextOut(hdc, 24 * dx, dy, l2, lstrlen(l2));
            EndPaint(hwnd, &ps);
        }
            return 0;

        case WM_DRAWITEM:
        case WM_COMMAND: {
            static auto format = TEXT("%-16s%04X-%04X       %04X-%04X");
            TCHAR buf[50];
            auto len = wsprintf(buf, format,
                                msg == WM_DRAWITEM ? TEXT("WM_DRAWITEM") : TEXT("WM_COMMAND"),
                                HIWORD(wparam), LOWORD(wparam),
                                HIWORD(lparam), LOWORD(lparam));

            ScrollWindow(hwnd, 0, -dy, &rect, &rect);
            auto hdc = GetDC(hwnd);
            TextOut(hdc, 24 * dx, dy * (rect.bottom / dy - 1), buf, len);
            ValidateRect(hwnd, &rect);
            ReleaseDC(hwnd, hdc);
        }
        }
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

    struct button_spec {
        int style;
        WCHAR* caption;
    };

    constexpr static button_spec ButtonSpec[] = {
        { BS_PUSHBUTTON, TEXT("Push button") },
        { BS_DEFPUSHBUTTON, TEXT("Def push button") },
        { BS_CHECKBOX, TEXT("Checkbox") },
        { BS_AUTOCHECKBOX, TEXT("Auto checkbox") },
        { BS_RADIOBUTTON, TEXT("Radio") },
        { BS_3STATE, TEXT("3State radio") },
        { BS_AUTO3STATE, TEXT("Auto 3State radio") },
        { BS_GROUPBOX, TEXT("Groupbox") },
        { BS_AUTORADIOBUTTON, TEXT("Auto radio") },
        { BS_OWNERDRAW, TEXT("Own redraw") },
    };

    HWND buttons[sizeof ButtonSpec];

    void create_buttons(HWND hwnd, LPARAM lparam) {
        float dx = LOWORD(GetDialogBaseUnits());
        float dy = HIWORD(GetDialogBaseUnits());
        auto i = 0u;
        for (const auto& btn : ButtonSpec) {
            buttons[i] = CreateWindow(TEXT("button"),
                                      btn.caption,
                                      WS_CHILD | WS_VISIBLE | btn.style,
                                      dx, dy * (1 + 2 * i),
                                      20 * dx, 7 * dy / 4,
                                      hwnd,
                                      reinterpret_cast<HMENU>(i),
                                      reinterpret_cast<LPCREATESTRUCT>(lparam)->hInstance,
                                      nullptr);
            i++;
        }
    }
};

App *App::instance = nullptr;

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR cmd_line, int cmd_show)
{
    App app;
    return app.run(instance, cmd_line, cmd_show);
}
