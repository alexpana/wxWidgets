#ifndef _WX_D2D_HANDER_H_
#define _WX_D2D_HANDER_H_

#include <memory>

#include <d2d1.h>

class Direct2DHandler
{
public:
    Direct2DHandler(HWND hWnd);
    ~Direct2DHandler();

    HRESULT Initialize();
    HRESULT OnRender();
    void OnResize(UINT width, UINT height);

private:
    // Creates resources for drawing
    HRESULT EnsureAvailableResources();

    // Releases resources for drawing
    void DiscardDeviceResources();

private:
    HWND m_hwnd;
    ID2D1Factory* m_direct2dFactory;
    ID2D1HwndRenderTarget* m_renderTarget;
    ID2D1SolidColorBrush* m_lightSlateGrayBrush;
    ID2D1LinearGradientBrush* m_linearGradientBrush;
};

#endif