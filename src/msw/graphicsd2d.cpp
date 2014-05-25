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

#include "wx/private/graphics.h"

extern WXDLLIMPEXP_DATA_CORE(wxGraphicsPen) wxNullGraphicsPen;

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

template <class T> bool IsAcquired(T *ppT)
{
    return ppT != NULL;
}

D2D1_CAP_STYLE ConvertPenCap(wxPenCap cap)
{
    switch (cap)
    {
    case wxCAP_ROUND:
        return D2D1_CAP_STYLE_ROUND;

    case wxCAP_PROJECTING:
        return D2D1_CAP_STYLE_SQUARE;

    case wxCAP_BUTT:
        return D2D1_CAP_STYLE_FLAT;

    case wxCAP_INVALID: 
        return D2D1_CAP_STYLE_FLAT;
    }

    return D2D1_CAP_STYLE_FLAT;
}

D2D1_LINE_JOIN ConvertPenJoin(wxPenJoin join)
{
    switch (join)
    {
    case wxJOIN_BEVEL:
        return D2D1_LINE_JOIN_BEVEL;

    case wxJOIN_MITER:
        return D2D1_LINE_JOIN_MITER;

    case wxJOIN_ROUND:
        return D2D1_LINE_JOIN_ROUND;

    case wxJOIN_INVALID:
        return D2D1_LINE_JOIN_MITER;
    }

    return D2D1_LINE_JOIN_MITER;
}

D2D1_DASH_STYLE ConvertPenStyle(wxPenStyle dashStyle)
{
    switch (dashStyle)
    {
    case wxPENSTYLE_SOLID:
        return D2D1_DASH_STYLE_SOLID;
    case wxPENSTYLE_DOT:
        return D2D1_DASH_STYLE_DOT;
    case wxPENSTYLE_LONG_DASH:
        return D2D1_DASH_STYLE_DASH;
    case wxPENSTYLE_SHORT_DASH:
        return D2D1_DASH_STYLE_DASH;
    case wxPENSTYLE_DOT_DASH:
        return D2D1_DASH_STYLE_DASH_DOT;
    case wxPENSTYLE_USER_DASH:
        return D2D1_DASH_STYLE_CUSTOM;

    // NB: These styles cannot be converted to a D2D1_DASH_STYLE
    // and must be handled separately.
    case wxPENSTYLE_TRANSPARENT:
        wxFALLTHROUGH;
    case wxPENSTYLE_INVALID:
        wxFALLTHROUGH;
    case wxPENSTYLE_STIPPLE_MASK_OPAQUE:
        wxFALLTHROUGH;
    case wxPENSTYLE_STIPPLE_MASK:
        wxFALLTHROUGH;
    case wxPENSTYLE_STIPPLE:
        wxFALLTHROUGH;
    case wxPENSTYLE_BDIAGONAL_HATCH: 
        wxFALLTHROUGH;
    case wxPENSTYLE_CROSSDIAG_HATCH:
        wxFALLTHROUGH;
    case wxPENSTYLE_FDIAGONAL_HATCH:
        wxFALLTHROUGH;
    case wxPENSTYLE_CROSS_HATCH:
        wxFALLTHROUGH;
    case wxPENSTYLE_HORIZONTAL_HATCH:
        wxFALLTHROUGH;
    case wxPENSTYLE_VERTICAL_HATCH:
        return D2D1_DASH_STYLE_SOLID;
    }

    return D2D1_DASH_STYLE_SOLID;
}

D2D1_COLOR_F ConvertColour(wxColour colour)
{
    return D2D1::ColorF(
        colour.Red() / 255.0f, 
        colour.Green() / 255.0f, 
        colour.Blue() / 255.0f, 
        colour.Alpha() / 255.0f);
}

D2D1_ANTIALIAS_MODE ConvertAntialiasMode(wxAntialiasMode antialiasMode)
{
    switch (antialiasMode)
    {
    case wxANTIALIAS_NONE:
        return D2D1_ANTIALIAS_MODE_ALIASED;
    case wxANTIALIAS_DEFAULT:
        return D2D1_ANTIALIAS_MODE_PER_PRIMITIVE;
    }

    return D2D1_ANTIALIAS_MODE_ALIASED;;
}

// Interface used by objects holding Direct2D device-dependent resources.
class DeviceDependentResourceHolder
{
public:
    virtual void AcquireDeviceDependentResources(ID2D1RenderTarget* renderTarget) = 0;
    virtual void ReleaseDeviceDependentResources() = 0;
};

//-----------------------------------------------------------------------------
// wxD2DPathData declaration
//-----------------------------------------------------------------------------

class wxD2DPathData : public wxGraphicsPathData
{
public :

    // ID2D1PathGeometry objects are device-independent resources created
    // from a ID2D1Factory. This means we can safely create the resource outside
    // (the wxD2DRenderer handles this) and store it here since it never gets 
    // thrown away by the GPU.
    wxD2DPathData(wxGraphicsRenderer* renderer, ID2D1PathGeometry* d2dPathGeometry);

    ~wxD2DPathData();

    wxGraphicsObjectRefData* Clone() const wxOVERRIDE;

    // begins a new subpath at (x,y)
    void MoveToPoint(wxDouble x, wxDouble y) wxOVERRIDE;

    // adds a straight line from the current point to (x,y)
    void AddLineToPoint(wxDouble x, wxDouble y) wxOVERRIDE;

    // adds a cubic Bezier curve from the current point, using two control points and an end point
    void AddCurveToPoint(wxDouble cx1, wxDouble cy1, wxDouble cx2, wxDouble cy2, wxDouble x, wxDouble y) wxOVERRIDE;

    // adds an arc of a circle centering at (x,y) with radius (r) from startAngle to endAngle
    void AddArc(wxDouble x, wxDouble y, wxDouble r, wxDouble startAngle, wxDouble endAngle, bool clockwise) wxOVERRIDE;

    // gets the last point of the current path, (0,0) if not yet set
    void GetCurrentPoint(wxDouble* x, wxDouble* y) const wxOVERRIDE;

    // adds another path
    void AddPath(const wxGraphicsPathData* path) wxOVERRIDE;

    // closes the current sub-path
    void CloseSubpath() wxOVERRIDE;

private :
    D2D1_POINT_2F m_cursorPosition;

    ID2D1PathGeometry* m_pathGeometry;
};

//-----------------------------------------------------------------------------
// wxD2DPathData implementation
//-----------------------------------------------------------------------------

wxD2DPathData::wxD2DPathData(wxGraphicsRenderer* renderer, ID2D1PathGeometry* d2dPathGeometry) : wxGraphicsPathData(renderer)
{
    wxFAIL_MSG("not implemented");
}

wxD2DPathData::~wxD2DPathData()
{
    wxFAIL_MSG("not implemented");
}

wxD2DPathData::wxGraphicsObjectRefData* wxD2DPathData::Clone() const
{
    wxFAIL_MSG("not implemented");
    return NULL;
}

void wxD2DPathData::MoveToPoint(wxDouble x, wxDouble y)
{
    wxFAIL_MSG("not implemented");
}

// adds a straight line from the current point to (x,y)
void wxD2DPathData::AddLineToPoint(wxDouble x, wxDouble y)
{
    wxFAIL_MSG("not implemented");
}

// adds a cubic Bezier curve from the current point, using two control points and an end point
void wxD2DPathData::AddCurveToPoint(wxDouble cx1, wxDouble cy1, wxDouble cx2, wxDouble cy2, wxDouble x, wxDouble y)
{
    wxFAIL_MSG("not implemented");
}

// adds an arc of a circle centering at (x,y) with radius (r) from startAngle to endAngle
void wxD2DPathData::AddArc(wxDouble x, wxDouble y, wxDouble r, wxDouble startAngle, wxDouble endAngle, bool clockwise)
{
    wxFAIL_MSG("not implemented");
}

// gets the last point of the current path, (0,0) if not yet set
void wxD2DPathData::GetCurrentPoint(wxDouble* x, wxDouble* y) const
{
    wxFAIL_MSG("not implemented");
}

// adds another path
void wxD2DPathData::AddPath(const wxGraphicsPathData* path)
{
    wxFAIL_MSG("not implemented");
}

// closes the current sub-path
void wxD2DPathData::CloseSubpath()
{
    wxFAIL_MSG("not implemented");
}

//-----------------------------------------------------------------------------
// wxD2DPenData declaration
//-----------------------------------------------------------------------------

class wxD2DPenData : public wxGraphicsObjectRefData, public DeviceDependentResourceHolder
{
public:
    wxD2DPenData(wxGraphicsRenderer* renderer, ID2D1Factory* direct2dFactory, const wxPen& pen);
    ~wxD2DPenData();

    void CreateStrokeStyle(ID2D1Factory* const direct2dfactory);

    ID2D1Brush* GetBrush();

    FLOAT GetWidth();

    ID2D1StrokeStyle* GetStrokeStyle();

    void AcquireDeviceDependentResources(ID2D1RenderTarget* renderTarget) wxOVERRIDE;

    void ReleaseDeviceDependentResources() wxOVERRIDE;

private:
    // We store the source pen for later when we need to recreate the
    // device-independent resources.
    const wxPen m_sourcePen;

    // A stroke style is a device-independent resource.
    // Describes the caps, miter limit, line join, and dash information.
    ID2D1StrokeStyle* m_strokeStyle;

    // A brush is a device-dependent resource.
    // Drawing outlines with Direct2D requires a brush for the color or stipple.
    ID2D1SolidColorBrush* m_solidColorStrokeBrush;

    FLOAT m_width;
};

//-----------------------------------------------------------------------------
// wxD2DPenData implementation
//-----------------------------------------------------------------------------

wxD2DPenData::wxD2DPenData(
    wxGraphicsRenderer* renderer, 
    ID2D1Factory* direct2dFactory, 
    const wxPen& pen)
    : wxGraphicsObjectRefData(renderer), m_sourcePen(pen), m_width(pen.GetWidth()), 
    m_solidColorStrokeBrush(NULL), m_strokeStyle(NULL)
{
    CreateStrokeStyle(direct2dFactory);
}

wxD2DPenData::~wxD2DPenData()
{
    SafeRelease(&m_strokeStyle);
    ReleaseDeviceDependentResources();
}

void wxD2DPenData::CreateStrokeStyle(ID2D1Factory* const direct2dfactory)
{
    D2D1_CAP_STYLE capStyle = ConvertPenCap(m_sourcePen.GetCap());
    D2D1_LINE_JOIN lineJoin = ConvertPenJoin(m_sourcePen.GetJoin());
    D2D1_DASH_STYLE dashStyle = ConvertPenStyle(m_sourcePen.GetStyle());

    direct2dfactory->CreateStrokeStyle(
        D2D1::StrokeStyleProperties(
            capStyle, capStyle, capStyle,
            lineJoin, 0, dashStyle, 0.0f),
        NULL,
        0,
        &m_strokeStyle);

    // TODO: Handle user-defined dashes
}

void wxD2DPenData::AcquireDeviceDependentResources(ID2D1RenderTarget* renderTarget)
{
    // Create the solid color stroke brush
    if (m_sourcePen.GetStyle() != wxPENSTYLE_STIPPLE && !IsAcquired(m_solidColorStrokeBrush))
    {
        renderTarget->CreateSolidColorBrush(
            ConvertColour(m_sourcePen.GetColour()),
            &m_solidColorStrokeBrush);
    }

    // TODO: Handle stipple pens
}

void wxD2DPenData::ReleaseDeviceDependentResources()
{
    SafeRelease(&m_solidColorStrokeBrush);
}

ID2D1Brush* wxD2DPenData::GetBrush()
{
    return m_solidColorStrokeBrush;
}

FLOAT wxD2DPenData::GetWidth()
{
    return m_width;
}

ID2D1StrokeStyle* wxD2DPenData::GetStrokeStyle()
{
    return m_strokeStyle;
}

//-----------------------------------------------------------------------------
// wxD2DBrushData declaration
//-----------------------------------------------------------------------------

class wxD2DBrushData : public wxGraphicsObjectRefData, public DeviceDependentResourceHolder
{
public:
    enum wxD2DBrushType 
    {
        wxD2DBRUSHTYPE_SOLID,
        wxD2DBRUSHTYPE_LINEAR_GRADIENT,
        wxD2DBRUSHTYPE_RADIAL_GRADIENT,
        wxD2DBRUSHTYPE_BITMAP,
        wxD2DBRUSHTYPE_UNSPECIFIED
    };

    struct LinearGradientBrushInfo {
        const wxDouble x1;
        const wxDouble y1;
        const wxDouble x2;
        const wxDouble y2;
        const wxGraphicsGradientStops stops;

        LinearGradientBrushInfo(
            wxDouble& x1, wxDouble& y1,
            wxDouble& x2, wxDouble& y2,
            const wxGraphicsGradientStops& stops)
            : x1(x1), y1(y1), x2(x2), y2(y2), stops(stops)
        {}
    };

    struct RadialGradientBrushInfo {
        const wxDouble x1;
        const wxDouble y1;
        const wxDouble x2;
        const wxDouble y2;
        const wxDouble radius;
        const wxGraphicsGradientStops stops;

        RadialGradientBrushInfo(
            wxDouble x1, wxDouble y1,
            wxDouble x2, wxDouble y2,
            wxDouble radius,
            const wxGraphicsGradientStops& stops)
            : x1(x1), y1(y1), x2(x2), y2(y2), radius(radius), stops(stops)
        {}
    };

    wxD2DBrushData(wxGraphicsRenderer* renderer, const wxBrush &brush);
    wxD2DBrushData(wxGraphicsRenderer* renderer);

    ~wxD2DBrushData();

    void CreateLinearGradientBrush(
        wxDouble x1, wxDouble y1,
        wxDouble x2, wxDouble y2,
        const wxGraphicsGradientStops& stops);

    void CreateRadialGradientBrush(
        wxDouble xo, wxDouble yo,
        wxDouble xc, wxDouble yc,
        wxDouble radius,
        const wxGraphicsGradientStops& stops);

    void EnsureInitialized();

    ID2D1Brush* GetBrush() const;

    void AcquireDeviceDependentResources(ID2D1RenderTarget* renderTarget) wxOVERRIDE;

    void ReleaseDeviceDependentResources() wxOVERRIDE;

private:
    void CreateSolidColorBrush();

    void CreateBitmapBrush();

private:
    // We store the source brush for later when we need to recreate the
    // device-independent resources.
    const wxBrush m_sourceBrush;

    // We store the information required to create a linear gradient brush for
    // later when we need to recreate the device-dependent resources.
    LinearGradientBrushInfo* m_linearGradientBrushInfo;

    // We store the information required to create a radial gradient brush for
    // later when we need to recreate the device-dependent resources.
    RadialGradientBrushInfo* m_radialGradientBrushInfo;

    // Used to identify which brush resource represents this brush.
    wxD2DBrushType m_brushType;

    // A ID2D1SolidColorBrush is a device-dependent resource.
    ID2D1SolidColorBrush* m_solidColorBrush;

    // A ID2D1LinearGradientBrush is a device-dependent resource.
    ID2D1LinearGradientBrush* m_linearGradientBrush;

    // A ID2D1RadialGradientBrush is a device-dependent resource.
    ID2D1RadialGradientBrush* m_radialGradientBrush;

    // A ID2D1BitmapBrush is a device-dependent resource.
    ID2D1BitmapBrush* m_bitmapBrush;
};

//-----------------------------------------------------------------------------
// wxD2DBrushData implementation
//-----------------------------------------------------------------------------

wxD2DBrushData::wxD2DBrushData(wxGraphicsRenderer* renderer, const wxBrush &brush) 
    : wxGraphicsObjectRefData(renderer), m_sourceBrush(brush),
    m_linearGradientBrushInfo(NULL), m_radialGradientBrushInfo(NULL), m_brushType(wxD2DBRUSHTYPE_UNSPECIFIED),
    m_solidColorBrush(NULL), m_linearGradientBrush(NULL), m_radialGradientBrush(NULL), m_bitmapBrush(NULL)
{
    if ( brush.GetStyle() == wxBRUSHSTYLE_SOLID)
    {
        CreateSolidColorBrush();
    } 
    else if (brush.IsHatch())
    {
        wxFAIL_MSG("hatch brushes are not implemented");
    }
    else
    {
        CreateBitmapBrush();
    }
}

wxD2DBrushData::wxD2DBrushData(wxGraphicsRenderer* renderer)
    : wxGraphicsObjectRefData(renderer),
    m_linearGradientBrushInfo(NULL), m_radialGradientBrushInfo(NULL), m_brushType(wxD2DBRUSHTYPE_UNSPECIFIED),
    m_solidColorBrush(NULL), m_linearGradientBrush(NULL), m_radialGradientBrush(NULL), m_bitmapBrush(NULL)
{
    wxFAIL_MSG("not implemented");
}

wxD2DBrushData::~wxD2DBrushData()
{
    delete m_linearGradientBrushInfo;
    delete m_radialGradientBrushInfo;

    ReleaseDeviceDependentResources();
}

void wxD2DBrushData::CreateSolidColorBrush()
{
    m_brushType = wxD2DBRUSHTYPE_SOLID;
    wxFAIL_MSG("not implemented");
}

void wxD2DBrushData::CreateBitmapBrush()
{
    m_brushType = wxD2DBRUSHTYPE_BITMAP;
    wxFAIL_MSG("not implemented");
}

void wxD2DBrushData::CreateLinearGradientBrush(
    wxDouble x1, wxDouble y1,
    wxDouble x2, wxDouble y2,
    const wxGraphicsGradientStops& stops)
{
    m_brushType = wxD2DBRUSHTYPE_LINEAR_GRADIENT;
    m_linearGradientBrushInfo = new LinearGradientBrushInfo(x1, y1, x2, y2, stops);

    wxFAIL_MSG("not implemented");
}

void wxD2DBrushData::CreateRadialGradientBrush(
    wxDouble xo, wxDouble yo,
    wxDouble xc, wxDouble yc,
    wxDouble radius,
    const wxGraphicsGradientStops& stops)
{
    m_brushType = wxD2DBRUSHTYPE_RADIAL_GRADIENT;
    m_radialGradientBrushInfo = new RadialGradientBrushInfo(xo, yo, xc, yc, radius, stops);

    wxFAIL_MSG("not implemented");
}

void wxD2DBrushData::AcquireDeviceDependentResources(ID2D1RenderTarget* renderTarget)
{
    if (m_brushType == wxD2DBRUSHTYPE_SOLID && !IsAcquired(m_solidColorBrush)) 
    {
        renderTarget->CreateSolidColorBrush(ConvertColour(m_sourceBrush.GetColour()), &m_solidColorBrush);
    } 
    else 
    {
        wxFAIL_MSG("not implemented");
    }
}

void wxD2DBrushData::ReleaseDeviceDependentResources()
{
    SafeRelease(&m_solidColorBrush);
    SafeRelease(&m_linearGradientBrush);
    SafeRelease(&m_radialGradientBrush);
    SafeRelease(&m_bitmapBrush);
}

ID2D1Brush* wxD2DBrushData::GetBrush() const
{
    switch (m_brushType)
    {
    case wxD2DBrushData::wxD2DBRUSHTYPE_SOLID:
        return m_solidColorBrush;
    case wxD2DBrushData::wxD2DBRUSHTYPE_LINEAR_GRADIENT:
        return m_linearGradientBrush;
    case wxD2DBrushData::wxD2DBRUSHTYPE_RADIAL_GRADIENT:
        return m_radialGradientBrush;
    case wxD2DBrushData::wxD2DBRUSHTYPE_BITMAP:
        return m_bitmapBrush;
    }

    return m_solidColorBrush;
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

    void SetPen(const wxGraphicsPen& pen) wxOVERRIDE;

private:
    void DoDrawText(const wxString& str, wxDouble x, wxDouble y) wxOVERRIDE;

    void EnsureInitialized();
    HRESULT CreateRenderTarget();

    void ReleaseDeviceDependentResources();

private:
    HWND m_hwnd;
    ID2D1Factory* m_direct2dFactory;
    ID2D1HwndRenderTarget* m_renderTarget;

    wxVector<DeviceDependentResourceHolder*> m_deviceDependentResourceHolders;

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

void wxD2DContext::SetPen(const wxGraphicsPen& pen)
{
    wxGraphicsContext::SetPen(pen);

    if ( &m_pen != &wxNullGraphicsPen )
    {
        wxD2DPenData* penData = static_cast<wxD2DPenData*>(m_pen.GetGraphicsData());
        penData->AcquireDeviceDependentResources(this->m_renderTarget);
        m_deviceDependentResourceHolders.push_back(penData);
    }
}

void wxD2DContext::ReleaseDeviceDependentResources()
{
    for ( unsigned int i = 0; i < m_deviceDependentResourceHolders.size(); ++i )
    {
        DeviceDependentResourceHolder* resourceHolder = 
            m_deviceDependentResourceHolders[i];
        resourceHolder->ReleaseDeviceDependentResources();
    }
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
    if ( !pen.IsOk() || pen.GetStyle() == wxPENSTYLE_TRANSPARENT )
    {
        return wxNullGraphicsPen;
    }
    else
    {
        wxGraphicsPen p;
        p.SetRefData(new wxD2DPenData(this, m_direct2dFactory, pen));
        return p;
    }
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