#include <windows.h>

#include <commdlg.h>
#include <shlwapi.h>
#include <xmllite.h>

#include <cassert>
#include <string>
#include <vector>

#define IDM_FILE_OPEN 4000
#define IDM_FILE_SAVE 4001
#define IDM_APP_EXIT 4002
#define IDM_HELP_HELP 4003
#define IDM_APP_ABOUT 4004

using tstring = std::basic_string<TCHAR>;

template <typename T>
struct CComPtr {

    CComPtr() = default;
    CComPtr(T *ptr)
        : ptr(ptr) {}

    CComPtr(const CComPtr &) = delete;
    CComPtr(CComPtr &&) = default;

    ~CComPtr() {
        ptr->Release();
    }

    T *operator->() {
        return ptr;
    }

    T **operator&() {
        return &ptr;
    }

    T &operator*() {
        return *ptr;
    }

    operator T *() {
        return ptr;
    }

    T *ptr = nullptr;
};

class App {
    static App *instance;

public:
    HINSTANCE hInstance;
    OPENFILENAME ofn;
    TCHAR file_name[MAX_PATH];
    TCHAR title[MAX_PATH];
    CComPtr<IStream> file_stream;
    CComPtr<IXmlReader> xml_reader;
    RECT rect;

public:
    App() {
        assert(App::instance == nullptr);
        App::instance = this;
    }

    int run(HINSTANCE instance, PWSTR cmd_line, int cmd_show) {
        constexpr wchar_t class_name[]  = L"NSE Main Window";

        WNDCLASS wc {};

        wc.lpfnWndProc = window_proc;
        wc.hInstance = instance;
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = class_name;
        wc.hbrBackground = (HBRUSH)(GetStockObject(HOLLOW_BRUSH));

        RegisterClass(&wc);

        auto menu = CreateMenu();
        auto menuPopup = CreateMenu();
        AppendMenu(menuPopup, MF_STRING, IDM_FILE_OPEN, L"&Open...");
        AppendMenu(menuPopup, MF_STRING, IDM_FILE_SAVE, L"&Save");
        AppendMenu(menuPopup, MF_SEPARATOR, 0, nullptr);
        AppendMenu(menuPopup, MF_STRING, IDM_APP_EXIT, L"E&xit");
        AppendMenu(menu, MF_POPUP, (UINT_PTR)menuPopup, L"&File");

        menuPopup = CreateMenu();
        AppendMenu(menuPopup, MF_STRING, IDM_HELP_HELP, L"&Help");
        AppendMenu(menuPopup, MF_STRING, IDM_APP_ABOUT, L"&About");
        AppendMenu(menu, MF_POPUP, (UINT_PTR)menuPopup, L"&Help");

        MENUINFO mi {};
        mi.cbSize = sizeof(mi);
        mi.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
        mi.hbrBack = (HBRUSH)(GetStockObject(HOLLOW_BRUSH));
        SetMenuInfo(menu, &mi);

        auto hwnd = CreateWindowEx(
            0, // Optional window styles.
            class_name, // Window class
            L"NSE",
            WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_BORDER,

            // Position and size
            CW_USEDEFAULT, CW_USEDEFAULT, 640u, 480u,

            nullptr,   // Parent window
            menu,   // Menu
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

        auto app = App::instance;

        switch (msg) {
        case WM_CREATE:
            app->create_buttons(hwnd, lparam);
            app->init_file_ops(hwnd);
            return 0;

        case WM_SIZE:
            app->rect.left = 24 * dx;
            app->rect.top = 2 * dy;
            app->rect.right = LOWORD(lparam);
            app->rect.bottom = HIWORD(lparam);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT:
            return 0;

        case WM_COMMAND: {
            switch (LOWORD(wparam)) {
            case IDM_APP_EXIT:
                SendMessage(hwnd, WM_CLOSE, 0, 0);
                return 0;
            case IDM_FILE_OPEN:
                if (app->open_file_dialog(hwnd)) {
                    if (!app->read_file(hwnd)) {
                        app->ok_message(hwnd, L"Failed to read file %s", app->title);
                        app->title[0] = '\0';
                        app->file_name[0] = '\0';
                    } else {
                        app->set_caption(hwnd);
                        app->show_comps(hwnd);
                        return 0;
                    }
                }
                break;
            default: break;
            }
        }
        }
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

    void ok_message(HWND hwnd, const TCHAR *msg, const TCHAR *title) {
        TCHAR buf[MAX_PATH + 64];
        wsprintf(buf, msg, title[0] == '\0' ? L"(untitled)" : title);
        MessageBox(hwnd, buf, L"NSE", MB_OK | MB_ICONEXCLAMATION);
    }

    BOOL open_file_dialog(HWND hwnd) {
        ofn.hwndOwner = hwnd;
        ofn.lpstrFile = file_name;
        ofn.lpstrFileTitle = title;
        ofn.Flags = OFN_CREATEPROMPT;

        return GetOpenFileName(&ofn);
    }

    BOOL save_file_dialog(HWND hwnd, PTSTR filename, PTSTR title) {
        ofn.hwndOwner = hwnd;
        ofn.lpstrFile = filename;
        ofn.lpstrFileTitle = title;
        ofn.Flags = OFN_OVERWRITEPROMPT;

        return GetSaveFileName(&ofn);
    }

    void init_file_ops(HWND hwnd) {
        static TCHAR filter[] = L"XML Files (*.xml)\0*.xml\0" \
            L"All Files (*.*)\0*.*\0\0";
        ofn.lStructSize = sizeof(decltype(ofn));
        ofn.hwndOwner = hwnd;
        ofn.hInstance = nullptr;
        ofn.lpstrFilter = filter;
        ofn.lpstrCustomFilter = nullptr;
        ofn.nMaxCustFilter = 0;
        ofn.nFilterIndex = 0;
        ofn.lpstrFile = nullptr;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFileTitle = nullptr;
        ofn.nMaxFileTitle = MAX_PATH;
        ofn.lpstrInitialDir = nullptr;
        ofn.lpstrTitle = nullptr;
        ofn.Flags = 0;
        ofn.nFileOffset = 0;
        ofn.nFileExtension = 0;
        ofn.lpstrDefExt = L"xml";
        ofn.lCustData = 0L;
        ofn.lpfnHook = nullptr;
        ofn.lpTemplateName = nullptr;

        file_name[0] = '\0';
        title[0] = '\0';
    }

    void set_caption(HWND hwnd) {
        TCHAR cap[64 + MAX_PATH];
        wsprintf(cap, L"%s - %s", L"NSE",
            title[0] == '\0' ? L"(untitled)" : title);
        SetWindowText(hwnd, cap);
    }

    void show_comps(HWND hwnd) {
        auto cx = LOWORD(GetDialogBaseUnits());
        auto cy = HIWORD(GetDialogBaseUnits());
        auto hdc = GetDC(hwnd);
        SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
        for (const auto &c : comp_names) {
            ScrollWindow(hwnd, 0, -cy, &rect, &rect);
            TextOut(hdc, 24 * cx, cy * (rect.bottom / cy - 1),
                    c.c_str(), c.size());
        }
        ReleaseDC(hwnd, hdc);
        ValidateRect(hwnd, &rect);
    }

    bool read_file(HWND hwnd) {
        auto hr = SHCreateStreamOnFile(file_name, STGM_READ, &file_stream);
        if (FAILED(hr)) {
            return false;
        }

        hr = CreateXmlReader(__uuidof(IXmlReader), (void **)&xml_reader, nullptr);
        if (FAILED(hr)) {
            return false;
        }

        hr = xml_reader->SetProperty(XmlReaderProperty_DtdProcessing, DtdProcessing_Prohibit);
        if (FAILED(hr)) {
            return false;
        }

        hr = xml_reader->SetInput(file_stream);
        if (FAILED(hr)) {
            return false;
        }

        XmlNodeType node_type;
        while (S_OK == (hr = xml_reader->Read(&node_type))) {
            switch (node_type) {
            case XmlNodeType_Element: {
                const TCHAR *local_name;
                if (!FAILED(xml_reader->GetLocalName(&local_name, nullptr))) {
                    comp_names.push_back(tstring(local_name));
                }
                break;
            }
            default:
                break;
            }
        }

        return true;
    }

private:
    std::vector<tstring> comp_names;
};

App *App::instance = nullptr;

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR cmd_line, int cmd_show)
{
    App app;
    app.hInstance = instance;
    return app.run(instance, cmd_line, cmd_show);
}
