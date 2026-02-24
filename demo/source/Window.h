/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/common/Exception.h>
#include <fraze/common/Object.h>
#include <WindowsPlatform.h>
#include <Types.h>
#include <string>

namespace fraze {

class Graphics;
class Program;

struct WindowResources
{
    ComPtr<ID3D11RenderTargetView> renderTargetView;
    ComPtr<ID3D11DepthStencilView> depthStencilView;
    ComPtr<IDXGISwapChain1> swapChain;
};

class Window : public Object
{
    std::string title;
    IVec2 pos{};
    IVec2 size{};
    HWND hWnd = nullptr;
    HDC hDC = nullptr;
    bool visible = false;
    bool resizable = true;
    int buttonsDown = 0;

    // graphics
    WindowResources resources;

    friend Graphics;
    friend Program;
    Graphics* graphics{};
public:
    Program* program{};
    Object* window{};

    Window(const TypeInfo* typeInfo, Program* program, Object* window, std::string_view title, const IVec2& pos, const IVec2& size, bool resizable = true);
    ~Window();

    void Show();
    void Hide();
    void Close();
    int PumpMessage();
    bool IsVisible() const { return visible; }
    
    void SetResizable(bool resizable);
    bool IsResizable() const { return resizable; }

    void SetTitle(const std::string& title);
    std::string GetTitle() const { return title; }

    void SetPos(const IVec2& pos);
    IVec2 GetPosition() const { return pos; }

    void SetSize(const IVec2& size);
    IVec2 GetSize() const { return size; }

    uintptr_t GetHandle() const { return (uintptr_t)hWnd; }

    void OnCreate();
    void OnShow();
    void OnHide();
    void OnDestroy();
    void OnClose();
    void OnMove(IVec2& newPos);
    void OnResize(IVec2& newSize);
    
    static Keycode TranslateKey(int keycode);
    int GetWindowHeight(HWND hWnd);
private:
    HWND CreateNativeWindow();
    static LRESULT CALLBACK CreateWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void UpdateSurface();
};

} // fraze
