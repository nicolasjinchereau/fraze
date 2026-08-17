/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <Window.h>
#include <Graphics.h>
#include <fraze/program/Program.h>

namespace fraze {

Window::Window(const TypeInfo* typeInfo, Program* program, Object* window, std::string_view title, const IVec2& pos, const IVec2& size, bool resizable)
    : Object(typeInfo), program(program), window(window), title(title), pos(pos), size(size), resizable(resizable)
{
}

Window::~Window()
{
    Close();
}

void Window::Show()
{
    if (!hWnd)
    {
        hWnd = CreateNativeWindow();
    }

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    visible = true;
}

void Window::Hide()
{
    ShowWindow(hWnd, SW_HIDE);
    UpdateWindow(hWnd);
    visible = false;
}

void Window::Close()
{
    if (hWnd)
    {
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)NULL);
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)&DefWindowProc);
        DestroyWindow(hWnd);
        hWnd = NULL;
    }
}

int Window::PumpMessage()
{
    int status = 0;

    MSG msg;
    if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if(msg.message == WM_QUIT)
        {
            status = -1;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            status = 1;
        }
    }

    return status;
}

void Window::SetResizable(bool resizable)
{
    this->resizable = resizable;

    if (hWnd)
    {
        int windowStyle = (int)GetWindowLongPtr(hWnd, GWL_STYLE);
        int resizableStyle = WS_SIZEBOX | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

        if(resizable)
            windowStyle |= resizableStyle;
        else
            windowStyle &= ~resizableStyle;

        SetWindowLongPtr(hWnd, GWL_STYLE, windowStyle);
    }
}

void Window::SetTitle(const std::string& title)
{
    this->title = title;
    if (hWnd)
        SetWindowText(hWnd, title.c_str());
}

void Window::SetPos(const IVec2& pos)
{
    this->pos = pos;
    auto flags = SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER;
    SetWindowPos(hWnd, NULL, (int)pos.x, (int)pos.y, 0, 0, flags);
}

void Window::SetSize(const IVec2& size)
{
    this->size = size;
    auto flags = SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER;
    SetWindowPos(hWnd, NULL, 0, 0, (int)size.x, (int)size.y, flags);
}

void Window::OnCreate() {

}

void Window::OnShow() {

}

void Window::OnHide() {

}

void Window::OnDestroy() {

}

void Window::OnClose() {
    PostQuitMessage(0);
}

void Window::OnMove(IVec2& newPos)
{

}

void Window::OnResize(IVec2& newSize)
{

}

HWND Window::CreateNativeWindow()
{
    auto className = "FrazeWindow";
    int windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;

    if(resizable)
        windowStyle |= WS_SIZEBOX | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

    RECT outerRect;
    outerRect.left = (int)pos.x;
    outerRect.top = (int)pos.y;
    outerRect.right = (int)(pos.x + size.x);
    outerRect.bottom = (int)(pos.y + size.y);

    AdjustWindowRect(&outerRect, windowStyle, false);

    int outerX = outerRect.left;
    int outerY = outerRect.top;
    int outerWidth = outerRect.right - outerRect.left;
    int outerHeight = outerRect.bottom - outerRect.top;

    HINSTANCE hInst = GetModuleHandle(NULL);

    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc = &CreateWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(void*);
    wcex.hInstance = hInst;
    wcex.hIcon = LoadIcon(GetModuleHandle(NULL), IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = CreateSolidBrush(0);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = className;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if(!RegisterClassEx(&wcex))
        throw Exception("failed to register window class");

    HWND hWnd = CreateWindow(
        className,
        title.c_str(),
        windowStyle,
        outerX,
        outerY,
        outerWidth,
        outerHeight,
        nullptr,
        nullptr,
        hInst,
        this);

    if(hWnd == NULL)
        throw Exception("call to CreateWindow failed");

    hDC = GetDC(hWnd);
    if(hDC == NULL)
        throw Exception("call to get window device context");

    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32, // bit depth
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        24, // z-buffer depth
        0, // stencil-buffer depth
        0, 0, 0, 0, 0, 0,
    };

    int pixelFormat = ChoosePixelFormat(hDC, &pfd);
    SetPixelFormat(hDC, pixelFormat, &pfd);

    return hWnd;
}

LRESULT CALLBACK Window::CreateWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CREATE)
    {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        Window* nativeWindow = (Window*)cs->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)nativeWindow);
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)&WndProc);
        nativeWindow->hWnd = hWnd;
        return WndProc(hWnd, msg, wParam, lParam);
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK Window::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto nativeWindow = (Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    std::string tmp;

    switch (msg)
    {
    case WM_CREATE:
        nativeWindow->OnCreate();
        return 0;

    case WM_DESTROY:
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)NULL);
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)&DefWindowProc);
        hWnd = NULL;
        nativeWindow->OnDestroy();
        return 0;

    case WM_SHOWWINDOW:
        if (wParam) {
            nativeWindow->visible = true;
            nativeWindow->OnShow();
        }
        else {
            nativeWindow->visible = false;
            nativeWindow->OnHide();
        }
        break;

    case WM_CLOSE:
        nativeWindow->OnClose();
        return 0;

    case WM_MOVE:
    {
        IVec2 newPos = IVec2(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        nativeWindow->OnMove(newPos);
        nativeWindow->pos = newPos;
        return 0;
    }
    case WM_SIZE:
    {
        IVec2 newSize = IVec2(LOWORD(lParam), HIWORD(lParam));

        if(nativeWindow->size.x != newSize.x || nativeWindow->size.y != newSize.y)
        {
            nativeWindow->OnResize(newSize);
            nativeWindow->size = newSize;
            nativeWindow->UpdateSurface();
            nativeWindow->program->Invoke("Window.OnResize", nativeWindow->window, (Integer)newSize.x, (Integer)newSize.y);
        }
        return 0;
    }
    case WM_SETTEXT:
        //nativeWindow->config.title = GetWindowTitle(hWnd);
        break;

    case WM_KEYDOWN:
    {
        if((lParam & 0x40000000) == 0) // avoid auto-repeat
        {
            nativeWindow->program->Invoke("Window.OnKeyDown", nativeWindow->window, (Integer)Window::TranslateKey((int)wParam));
            return 0;
        }
        break;
    }
    case WM_KEYUP:
    {
        nativeWindow->program->Invoke("Window.OnKeyUp", nativeWindow->window, (Integer)Window::TranslateKey((int)wParam));
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        if (nativeWindow->buttonsDown++ == 0)
            SetCapture(hWnd);

        nativeWindow->program->Invoke("Window.OnPointerDown",
            nativeWindow->window,
            Integer(GET_X_LPARAM(lParam)),
            Integer(nativeWindow->GetWindowHeight(hWnd) - GET_Y_LPARAM(lParam) - 1),
            Integer(0));

        return 0;
    }
    case WM_RBUTTONDOWN:
    {
        if (nativeWindow->buttonsDown++ == 0)
            SetCapture(hWnd);

        nativeWindow->program->Invoke("Window.OnPointerDown",
            nativeWindow->window,
            Integer(GET_X_LPARAM(lParam)),
            Integer(nativeWindow->GetWindowHeight(hWnd) - GET_Y_LPARAM(lParam) - 1),
            Integer(1));

        return 0;
    }
    case WM_MOUSEMOVE:
    {
        nativeWindow->program->Invoke("Window.OnPointerMove",
            nativeWindow->window,
            Integer(GET_X_LPARAM(lParam)),
            Integer(nativeWindow->GetWindowHeight(hWnd) - GET_Y_LPARAM(lParam) - 1),
            Integer(0));
        return 0;
    }
    case WM_LBUTTONUP:
    {
        nativeWindow->program->Invoke("Window.OnPointerUp",
            nativeWindow->window,
            Integer(GET_X_LPARAM(lParam)),
            Integer(nativeWindow->GetWindowHeight(hWnd) - GET_Y_LPARAM(lParam) - 1),
            Integer(0));

        if (--nativeWindow->buttonsDown == 0)
            ReleaseCapture();

        return 0;
    }
    case WM_RBUTTONUP:
    {
        nativeWindow->program->Invoke("Window.OnPointerUp",
            nativeWindow->window,
            Integer(GET_X_LPARAM(lParam)),
            Integer(nativeWindow->GetWindowHeight(hWnd) - GET_Y_LPARAM(lParam) - 1),
            Integer(1));

        if (--nativeWindow->buttonsDown == 0)
            ReleaseCapture();

        return 0;
    }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

constexpr int _VK_OEM_1      = 0xBA;
constexpr int _VK_OEM_PLUS   = 0xBB;
constexpr int _VK_OEM_COMMA  = 0xBC;
constexpr int _VK_OEM_MINUS  = 0xBD;
constexpr int _VK_OEM_PERIOD = 0xBE;
constexpr int _VK_OEM_2      = 0xBF;
constexpr int _VK_OEM_3      = 0xC0;
constexpr int _VK_OEM_4      = 0xDB;
constexpr int _VK_OEM_5      = 0xDC;
constexpr int _VK_OEM_6      = 0xDD;
constexpr int _VK_OEM_7      = 0xDE;
constexpr int _VK_OEM_8      = 0xDF;

Keycode Window::TranslateKey(int keycode)
{
    switch (keycode)
    {
    case 'A': return Keycode::A;
    case 'B': return Keycode::B;
    case 'C': return Keycode::C;
    case 'D': return Keycode::D;
    case 'E': return Keycode::E;
    case 'F': return Keycode::F;
    case 'G': return Keycode::G;
    case 'H': return Keycode::H;
    case 'I': return Keycode::I;
    case 'J': return Keycode::J;
    case 'K': return Keycode::K;
    case 'L': return Keycode::L;
    case 'M': return Keycode::M;
    case 'N': return Keycode::N;
    case 'O': return Keycode::O;
    case 'P': return Keycode::P;
    case 'Q': return Keycode::Q;
    case 'R': return Keycode::R;
    case 'S': return Keycode::S;
    case 'T': return Keycode::T;
    case 'U': return Keycode::U;
    case 'V': return Keycode::V;
    case 'W': return Keycode::W;
    case 'X': return Keycode::X;
    case 'Y': return Keycode::Y;
    case 'Z': return Keycode::Z;
    case '0': return Keycode::Num0;
    case '1': return Keycode::Num1;
    case '2': return Keycode::Num2;
    case '3': return Keycode::Num3;
    case '4': return Keycode::Num4;
    case '5': return Keycode::Num5;
    case '6': return Keycode::Num6;
    case '7': return Keycode::Num7;
    case '8': return Keycode::Num8;
    case '9': return Keycode::Num9;
    case VK_F1: return Keycode::F1;
    case VK_F2: return Keycode::F2;
    case VK_F3: return Keycode::F3;
    case VK_F4: return Keycode::F4;
    case VK_F5: return Keycode::F5;
    case VK_F6: return Keycode::F6;
    case VK_F7: return Keycode::F7;
    case VK_F8: return Keycode::F8;
    case VK_F9: return Keycode::F9;
    case VK_F10: return Keycode::F10;
    case VK_F11: return Keycode::F11;
    case VK_F12: return Keycode::F12;
    case VK_BROWSER_BACK: return Keycode::Back;
    case VK_BROWSER_FORWARD: return Keycode::Forward;
    case VK_VOLUME_UP: return Keycode::VolumeUp;
    case VK_VOLUME_DOWN: return Keycode::VolumeDown;
    case VK_VOLUME_MUTE: return Keycode::VolumeMute;
    case VK_SPACE: return Keycode::Space;
    case VK_BACK: return Keycode::Backspace;
    case VK_DELETE: return Keycode::Del;
    case VK_LEFT: return Keycode::LeftArrow;
    case VK_RIGHT: return Keycode::RightArrow;
    case VK_UP: return Keycode::UpArrow;
    case VK_DOWN: return Keycode::DownArrow;
    case VK_MENU: return Keycode::Alt;
    case VK_CONTROL: return Keycode::Ctrl;
    case VK_SHIFT: return Keycode::Shift;
    case VK_RETURN: return Keycode::Enter;
    case VK_ESCAPE: return Keycode::Escape;
    case _VK_OEM_PLUS: return Keycode::Equals;
    case _VK_OEM_MINUS: return Keycode::Minus;
    case _VK_OEM_4: return Keycode::LeftBracket;
    case _VK_OEM_6: return Keycode::RightBracket;
    case _VK_OEM_7: return Keycode::Quote;
    case _VK_OEM_1: return Keycode::Semicolon;
    case _VK_OEM_2: return Keycode::Slash;
    case _VK_OEM_5: return Keycode::Backslash;
    case _VK_OEM_COMMA: return Keycode::Comma;
    case _VK_OEM_PERIOD: return Keycode::Period;
    case _VK_OEM_3: return Keycode::Grave;
    case VK_TAB: return Keycode::Tab;
    }

    return (Keycode)((int)Keycode::Unknown + keycode);
}

int Window::GetWindowHeight(HWND hWnd)
{
    RECT rc;
    GetClientRect(hWnd, &rc);
    return rc.bottom - rc.top;
}

void Window::UpdateSurface()
{
    if(graphics)
    {
        graphics->ResizeSurface((uint32_t)size.x, (uint32_t)size.y, &resources);
    }
}

} // fraze
