/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/graphicsd2d.cpp
// Purpose:     Implementation of Direct2D Render Context
// Author:      Pana Alexandru <astronothing@gmail.com>
// Created:     2014-05-20
// Copyright:   (c) 2014 wxWidgets development team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

// Ensure no previous defines interfere with the Direct2D API headers
#undef GetHwnd()

#include <d2d1.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "wx/dc.h"

#if wxUSE_GRAPHICS_DIRECT2D

#ifndef WX_PRECOMP
// add individual includes here
#endif

#include "wx/graphics.h"

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

//-----------------------------------------------------------------------------
// wxD2DContext declaration
//-----------------------------------------------------------------------------

class wxD2DContext : public wxGraphicsContext
{
public:
    wxD2DContext(wxGraphicsRenderer* renderer, ID2D1Factory* direct2dFactory, HWND hwnd);

    ~wxD2DContext() wxOVERRIDE;

    void Clip(const wxRegion &region) wxOVERRIDE;
    void Clip(wxDouble x, wxDouble y, wxDouble w, wxDouble h) wxOVERRIDE;
    void ResetClip() wxOVERRIDE;

    void* GetNativeContext() wxOVERRIDE;

    void StrokePath(const wxGraphicsPath& p) wxOVERRIDE;
    void FillPath(const wxGraphicsPath& p , wxPolygonFillMode fillStyle = wxODDEVEN_RULE) wxOVERRIDE;

    void DrawRectangle(wxDouble x, wxDouble y, wxDouble w, wxDouble h) wxOVERRIDE; 

    void StrokeLines(size_t n, const wxPoint2DDouble* points) wxOVERRIDE;
    void StrokeLines(size_t n, const wxPoint2DDouble* beginPoints, const wxPoint2DDouble* endPoints) wxOVERRIDE;

    void DrawLines(size_t n, const wxPoint2DDouble* points, wxPolygonFillMode fillStyle = wxODDEVEN_RULE) wxOVERRIDE;

    bool SetAntialiasMode(wxAntialiasMode antialias) wxOVERRIDE;

    bool SetInterpolationQuality(wxInterpolationQuality interpolation) wxOVERRIDE;

    bool SetCompositionMode(wxCompositionMode op) wxOVERRIDE;

    void BeginLayer(wxDouble opacity) wxOVERRIDE;

    void EndLayer() wxOVERRIDE;

    void Translate(wxDouble dx, wxDouble dy) wxOVERRIDE;
    void Scale(wxDouble xScale, wxDouble yScale) wxOVERRIDE;
    void Rotate(wxDouble angle) wxOVERRIDE;

    void ConcatTransform(const wxGraphicsMatrix& matrix) wxOVERRIDE;

    void SetTransform(const wxGraphicsMatrix& matrix) wxOVERRIDE;

    wxGraphicsMatrix GetTransform() const wxOVERRIDE;

    void DrawBitmap(const wxGraphicsBitmap& bmp, wxDouble x, wxDouble y, wxDouble w, wxDouble h) wxOVERRIDE;
    void DrawBitmap(const wxBitmap& bmp, wxDouble x, wxDouble y, wxDouble w, wxDouble h) wxOVERRIDE;

    void DrawIcon(const wxIcon& icon, wxDouble x, wxDouble y, wxDouble w, wxDouble h) wxOVERRIDE;

    void PushState() wxOVERRIDE;
    void PopState() wxOVERRIDE;

    void GetTextExtent(
        const wxString& str, 
        wxDouble* width, 
        wxDouble* height,
        wxDouble* descent,
        wxDouble* externalLeading) const wxOVERRIDE;

    void GetPartialTextExtents(const wxString& text, wxArrayDouble& widths) const wxOVERRIDE;
    bool ShouldOffset() const wxOVERRIDE;

private:
    void DoDrawText(const wxString& str, wxDouble x, wxDouble y) wxOVERRIDE;

    void EnsureInitialized();
    HRESULT CreateRenderTarget();

private:
    HWND m_hwnd;
    ID2D1Factory* m_direct2dFactory;
    ID2D1HwndRenderTarget* m_renderTarget;

private:
    wxDECLARE_NO_COPY_CLASS(wxD2DContext);
};

//-----------------------------------------------------------------------------
// wxD2DContext implementation
//-----------------------------------------------------------------------------

wxD2DContext::wxD2DContext(wxGraphicsRenderer* renderer, ID2D1Factory* direct2dFactory, HWND hwnd) : wxGraphicsContext(renderer)
{
    m_hwnd = hwnd;
    m_renderTarget = NULL;
    m_direct2dFactory = direct2dFactory;
}

wxD2DContext::~wxD2DContext()
{
    m_renderTarget->EndDraw();
    SafeRelease(&m_renderTarget);
}

void wxD2DContext::Clip(const wxRegion& region)
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::Clip(wxDouble x, wxDouble y, wxDouble w, wxDouble h)
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::ResetClip()
{
    wxFAIL_MSG("not implemented");
}

void* wxD2DContext::GetNativeContext()
{
    wxFAIL_MSG("not implemented");
    return NULL;
}

void wxD2DContext::StrokePath(const wxGraphicsPath& p)
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::FillPath(const wxGraphicsPath& p , wxPolygonFillMode fillStyle)
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::DrawRectangle(wxDouble x, wxDouble y, wxDouble w, wxDouble h)
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::StrokeLines(size_t n, const wxPoint2DDouble* points)
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::StrokeLines(size_t n, const wxPoint2DDouble* beginPoints, const wxPoint2DDouble* endPoints)
{
    wxGraphicsContext::StrokeLines(n, beginPoints, endPoints);
}

void wxD2DContext::DrawLines(size_t n, const wxPoint2DDouble* points, wxPolygonFillMode fillStyle)
{
    wxFAIL_MSG("not implemented");
}

bool wxD2DContext::SetAntialiasMode(wxAntialiasMode antialias)
{
    wxFAIL_MSG("not implemented");
    return false;
}

bool wxD2DContext::SetInterpolationQuality(wxInterpolationQuality interpolation)
{
    wxFAIL_MSG("not implemented");
    return false;
}

bool wxD2DContext::SetCompositionMode(wxCompositionMode op)
{
    wxFAIL_MSG("not implemented");
    return false;
}

void wxD2DContext::BeginLayer(wxDouble opacity)
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::EndLayer()
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::Translate(wxDouble dx, wxDouble dy)
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::Scale(wxDouble xScale, wxDouble yScale)
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::Rotate(wxDouble angle)
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::ConcatTransform(const wxGraphicsMatrix& matrix)
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::SetTransform(const wxGraphicsMatrix& matrix)
{
    wxFAIL_MSG("not implemented");
}

wxGraphicsMatrix wxD2DContext::GetTransform() const
{
    wxFAIL_MSG("not implemented");
    return wxGraphicsMatrix();
}

void wxD2DContext::DrawBitmap(const wxGraphicsBitmap& bmp, wxDouble x, wxDouble y, wxDouble w, wxDouble h)
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::DrawBitmap(const wxBitmap& bmp, wxDouble x, wxDouble y, wxDouble w, wxDouble h)
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::DrawIcon(const wxIcon& icon, wxDouble x, wxDouble y, wxDouble w, wxDouble h)
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::PushState()
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::PopState()
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::GetTextExtent(
    const wxString& str, 
    wxDouble* width, 
    wxDouble* height,
    wxDouble* descent,
    wxDouble* externalLeading) const
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::GetPartialTextExtents(const wxString& text, wxArrayDouble& widths) const
{
    wxFAIL_MSG("not implemented");
}

bool wxD2DContext::ShouldOffset() const
{
    wxFAIL_MSG("not implemented");
    return false;
}

void wxD2DContext::DoDrawText(const wxString& str, wxDouble x, wxDouble y)
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::EnsureInitialized()
{
    if ( !m_renderTarget ) 
    {
        HRESULT result;

        result = CreateRenderTarget();

        if (FAILED(result))
        {
            wxFAIL_MSG("Could not create Direct2D render target");
        }

        m_renderTarget->BeginDraw();
    }
}

HRESULT wxD2DContext::CreateRenderTarget()
{
    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);

    D2D1_SIZE_U size = D2D1::SizeU(
        clientRect.right = clientRect.left,
        clientRect.bottom - clientRect.top);

    return m_direct2dFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(m_hwnd, size),
        &m_renderTarget);
}

//-----------------------------------------------------------------------------
// wxD2DRenderer declaration
//-----------------------------------------------------------------------------

class wxD2DRenderer : public wxGraphicsRenderer
{
public :
    wxD2DRenderer();

    virtual ~wxD2DRenderer();

    wxGraphicsContext* CreateContext(const wxWindowDC& dc);

    wxGraphicsContext* CreateContext(const wxMemoryDC& dc) wxOVERRIDE;

#if wxUSE_PRINTING_ARCHITECTURE
    wxGraphicsContext* CreateContext(const wxPrinterDC& dc) wxOVERRIDE;
#endif

#if wxUSE_ENH_METAFILE
    wxGraphicsContext* CreateContext(const wxEnhMetaFileDC& dc) wxOVERRIDE;
#endif

    wxGraphicsContext* CreateContextFromNativeContext(void* context) wxOVERRIDE;

    wxGraphicsContext* CreateContextFromNativeWindow(void* window) wxOVERRIDE;

    wxGraphicsContext* CreateContext(wxWindow* window) wxOVERRIDE;

#if wxUSE_IMAGE
    wxGraphicsContext* CreateContextFromImage(wxImage& image) wxOVERRIDE;
#endif // wxUSE_IMAGE

    wxGraphicsContext* CreateMeasuringContext() wxOVERRIDE;

    wxGraphicsPath CreatePath() wxOVERRIDE;

    wxGraphicsMatrix CreateMatrix(
        wxDouble a = 1.0, wxDouble b = 0.0, wxDouble c = 0.0, wxDouble d = 1.0,
        wxDouble tx = 0.0, wxDouble ty = 0.0) wxOVERRIDE;

    wxGraphicsPen CreatePen(const wxPen& pen) wxOVERRIDE;

    wxGraphicsBrush CreateBrush(const wxBrush& brush) wxOVERRIDE;

    wxGraphicsBrush CreateLinearGradientBrush(
        wxDouble x1, wxDouble y1,
        wxDouble x2, wxDouble y2,
        const wxGraphicsGradientStops& stops) wxOVERRIDE;

    wxGraphicsBrush CreateRadialGradientBrush(
        wxDouble xo, wxDouble yo,
        wxDouble xc, wxDouble yc,
        wxDouble radius,
        const wxGraphicsGradientStops& stops) wxOVERRIDE;

    // create a native bitmap representation
    wxGraphicsBitmap CreateBitmap(const wxBitmap& bitmap) wxOVERRIDE;

#if wxUSE_IMAGE
    wxGraphicsBitmap CreateBitmapFromImage(const wxImage& image) wxOVERRIDE;
    wxImage CreateImageFromBitmap(const wxGraphicsBitmap& bmp) wxOVERRIDE;
#endif

    wxGraphicsFont CreateFont(const wxFont& font, const wxColour& col) wxOVERRIDE;

    wxGraphicsFont CreateFont(
        double size, const wxString& facename, 
        int flags = wxFONTFLAG_DEFAULT, 
        const wxColour& col = *wxBLACK) wxOVERRIDE;

    // create a graphics bitmap from a native bitmap
    wxGraphicsBitmap CreateBitmapFromNativeBitmap(void* bitmap) wxOVERRIDE;

    // create a sub-image from a native image representation
    wxGraphicsBitmap CreateSubBitmap(const wxGraphicsBitmap& bitmap, wxDouble x, wxDouble y, wxDouble w, wxDouble h) wxOVERRIDE;

    wxString GetName() const wxOVERRIDE;
    void GetVersion(int* major, int* minor, int* micro) const wxOVERRIDE;

private:
    ID2D1Factory* m_direct2dFactory;

private :
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxD2DRenderer)
};

//-----------------------------------------------------------------------------
// wxD2DRenderer implementation
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxD2DRenderer,wxGraphicsRenderer)

static wxD2DRenderer gs_D2DRenderer;

wxGraphicsRenderer* wxGraphicsRenderer::GetDirect2DRenderer()
{
    return &gs_D2DRenderer;
}

wxD2DRenderer::wxD2DRenderer()
{
    HRESULT result;
    result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_direct2dFactory);
    if (FAILED(result)) {
        wxFAIL_MSG("Could not create Direct2D Factory.");
    }
}

wxD2DRenderer::~wxD2DRenderer()
{
    SafeRelease(&m_direct2dFactory);
}

wxGraphicsContext* wxD2DRenderer::CreateContext(const wxWindowDC& dc)
{
    wxFAIL_MSG("not implemented");
    return NULL;
}

wxGraphicsContext* wxD2DRenderer::CreateContext(const wxMemoryDC& dc)
{
    wxFAIL_MSG("not implemented");
    return NULL;
}

#if wxUSE_PRINTING_ARCHITECTURE
wxGraphicsContext* wxD2DRenderer::CreateContext(const wxPrinterDC& dc)
{
    wxFAIL_MSG("not implemented");
    return NULL;
}
#endif

#if wxUSE_ENH_METAFILE
wxGraphicsContext* wxD2DRenderer::CreateContext(const wxEnhMetaFileDC& dc)
{
    wxFAIL_MSG("not implemented");
    return NULL;
}
#endif

wxGraphicsContext* wxD2DRenderer::CreateContextFromNativeContext(void* context)
{
    wxFAIL_MSG("not implemented");
    return NULL;
}

wxGraphicsContext* wxD2DRenderer::CreateContextFromNativeWindow(void* window)
{
    wxFAIL_MSG("not implemented");
    return NULL;
}

wxGraphicsContext* wxD2DRenderer::CreateContext(wxWindow* window)
{
    return new wxD2DContext(this, m_direct2dFactory, (HWND)window->GetHWND());
}

#if wxUSE_IMAGE
wxGraphicsContext* wxD2DRenderer::CreateContextFromImage(wxImage& image)
{
    wxFAIL_MSG("not implemented");
    return NULL;
}
#endif // wxUSE_IMAGE

wxGraphicsContext* wxD2DRenderer::CreateMeasuringContext()
{
    wxFAIL_MSG("not implemented");
    return NULL;
}

wxGraphicsPath wxD2DRenderer::CreatePath()
{
    wxFAIL_MSG("not implemented");
    return wxGraphicsPath();
}

wxGraphicsMatrix wxD2DRenderer::CreateMatrix(
    wxDouble a, wxDouble b, wxDouble c, wxDouble d,
    wxDouble tx, wxDouble ty)
{
    wxFAIL_MSG("not implemented");
    return wxGraphicsMatrix();
}

wxGraphicsPen wxD2DRenderer::CreatePen(const wxPen& pen)
{
    wxFAIL_MSG("not implemented");
    return wxGraphicsPen();
}

wxGraphicsBrush wxD2DRenderer::CreateBrush(const wxBrush& brush)
{
    wxFAIL_MSG("not implemented");
    return wxGraphicsBrush();
}


wxGraphicsBrush wxD2DRenderer::CreateLinearGradientBrush(
    wxDouble x1, wxDouble y1,
    wxDouble x2, wxDouble y2,
    const wxGraphicsGradientStops& stops)
{
    wxFAIL_MSG("not implemented");
    return wxGraphicsBrush();
}

wxGraphicsBrush wxD2DRenderer::CreateRadialGradientBrush(
    wxDouble xo, wxDouble yo,
    wxDouble xc, wxDouble yc,
    wxDouble radius,
    const wxGraphicsGradientStops& stops)
{
    wxFAIL_MSG("not implemented");
    return wxGraphicsBrush();
}

// create a native bitmap representation
wxGraphicsBitmap wxD2DRenderer::CreateBitmap(const wxBitmap& bitmap)
{
    wxFAIL_MSG("not implemented");
    return wxGraphicsBitmap();
}

#if wxUSE_IMAGE
wxGraphicsBitmap wxD2DRenderer::CreateBitmapFromImage(const wxImage& image)
{
    wxFAIL_MSG("not implemented");
    return wxGraphicsBitmap();
}

wxImage wxD2DRenderer::CreateImageFromBitmap(const wxGraphicsBitmap& bmp)
{
    wxFAIL_MSG("not implemented");
    return wxImage();
}
#endif

wxGraphicsFont wxD2DRenderer::CreateFont(const wxFont& font, const wxColour& col)
{
    wxFAIL_MSG("not implemented");
    return wxGraphicsFont();
}

wxGraphicsFont wxD2DRenderer::CreateFont(
    double size, const wxString& facename, 
    int flags, 
    const wxColour& col)
{
    wxFAIL_MSG("not implemented");
    return wxGraphicsFont();
}

// create a graphics bitmap from a native bitmap
wxGraphicsBitmap wxD2DRenderer::CreateBitmapFromNativeBitmap(void* bitmap)
{
    wxFAIL_MSG("not implemented");
    return wxGraphicsBitmap();
}

// create a sub-image from a native image representation
wxGraphicsBitmap wxD2DRenderer::CreateSubBitmap(const wxGraphicsBitmap& bitmap, wxDouble x, wxDouble y, wxDouble w, wxDouble h)
{
    wxFAIL_MSG("not implemented");
    return wxGraphicsBitmap();
}

wxString wxD2DRenderer::GetName() const
{
    return "direct2d";
}

void wxD2DRenderer::GetVersion(int* major, int* minor, int* micro) const
{
    if ( major )
        *major = wxPlatformInfo::Get().GetOSMajorVersion();
    if ( minor )
        *minor = wxPlatformInfo::Get().GetOSMinorVersion();
    if ( micro )
        *micro = 0;
}

#endif // wxUSE_GRAPHICS_DIRECT2D