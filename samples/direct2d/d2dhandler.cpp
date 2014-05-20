#include "d2dhandler.h"

#define RET_ON_FAIL(hr) if ( !SUCCEEDED(hr) ){ return hr; }

Direct2DHandler::Direct2DHandler(HWND hWnd) 
{
    m_hwnd = hWnd;
    m_renderTarget = nullptr;
    Initialize();
}

Direct2DHandler::~Direct2DHandler() 
{
    DiscardDeviceResources();
}

HRESULT Direct2DHandler::Initialize()
{
    return D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &m_direct2dFactory);
}

HRESULT Direct2DHandler::OnRender()
{
    HRESULT hr = S_OK;

    hr = EnsureAvailableResources();

    if ( SUCCEEDED(hr) )
    {
        m_renderTarget->BeginDraw();

        m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

        m_renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

        D2D1_SIZE_F rtSize = m_renderTarget->GetSize();

        D2D1_RECT_F rectangle = D2D1::RectF(0, 0, rtSize.width, rtSize.height);


        // Draw the outline of a rectangle.
        m_renderTarget->FillRectangle(&rectangle, m_linearGradientBrush);

        hr = m_renderTarget->EndDraw();

    }
    if ( hr == D2DERR_RECREATE_TARGET )
    {
        hr = S_OK;
        DiscardDeviceResources();
    }

    return hr;
}

void Direct2DHandler::OnResize(UINT width, UINT height)
{
    EnsureAvailableResources();
    m_renderTarget->Resize(D2D1::SizeU(width, height));
}

HRESULT CreateRenderTarget(const HWND& hwnd, ID2D1Factory *direct2dFactory, ID2D1HwndRenderTarget **renderTarget)
{
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);

    D2D1_SIZE_U size = D2D1::SizeU(
        clientRect.right = clientRect.left,
        clientRect.bottom - clientRect.top);

    return direct2dFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd, size),
        renderTarget);
}

HRESULT CreateLinearGradientBrush(ID2D1RenderTarget *renderTarget, ID2D1LinearGradientBrush **brush)
{
    HRESULT hr;

    ID2D1GradientStopCollection *pGradientStops = NULL;

    D2D1_GRADIENT_STOP gradientStops[2];
    gradientStops[0].color = D2D1::ColorF(D2D1::ColorF::Green, 1);
    gradientStops[0].position = 0.0f;
    gradientStops[1].color = D2D1::ColorF(D2D1::ColorF::Red, 1);
    gradientStops[1].position = 1.0f;

    hr = renderTarget->CreateGradientStopCollection(
        gradientStops,
        2,
        D2D1_GAMMA_2_2,
        D2D1_EXTEND_MODE_WRAP,
        &pGradientStops);

    RET_ON_FAIL(hr);

    hr = renderTarget->CreateLinearGradientBrush(
        D2D1::LinearGradientBrushProperties(D2D1::Point2F(0, 0), D2D1::Point2F(50, 50)),
        pGradientStops,
        brush);

    return hr;
}

HRESULT Direct2DHandler::EnsureAvailableResources()
{
    HRESULT hr = S_OK;

    if ( !m_renderTarget ) 
    {
        hr = CreateRenderTarget(m_hwnd, m_direct2dFactory, &m_renderTarget);

        RET_ON_FAIL(hr);

        hr = m_renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::LightSlateGray),
            &m_lightSlateGrayBrush);

        RET_ON_FAIL(hr);

        hr = CreateLinearGradientBrush(m_renderTarget, &m_linearGradientBrush);
    }

    return hr;
}

template<class I>
inline void SafeRelease(I **ppInterfaceToRelease)
{
    if ( *ppInterfaceToRelease != NULL )
    {
        (*ppInterfaceToRelease)->Release();
        (*ppInterfaceToRelease) = NULL;
    }
}

void Direct2DHandler::DiscardDeviceResources()
{
    SafeRelease(&m_renderTarget);
    SafeRelease(&m_lightSlateGrayBrush);
    SafeRelease(&m_linearGradientBrush);
}