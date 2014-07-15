/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/graphicsd2d.cpp
// Purpose:     Implementation of Direct2D Render Context
// Author:      Pana Alexandru <astronothing@gmail.com>
// Created:     2014-05-20
// Copyright:   (c) 2014 wxWidgets development team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

// Minimum supported client    Windows 8 and Platform Update for Windows 7 [desktop apps | Windows Store apps]
// Minimum supported server    Windows Server 2012 and Platform Update for Windows Server 2008 R2 [desktop apps | Windows Store apps]
// Minimum supported phone     Windows Phone 8.1 [Windows Phone Silverlight 8.1 and Windows Runtime apps]
#define D2D1_BLEND_SUPPORTED 1

// Minimum supported client    Windows 8 and Platform Update for Windows 7 [desktop apps | Windows Store apps]
// Minimum supported server    Windows Server 2012 and Platform Update for Windows Server 2008 R2 [desktop apps | Windows Store apps]
// Minimum supported phone     Windows Phone 8.1 [Windows Phone Silverlight 8.1 and Windows Runtime apps]
#define D2D1_INTERPOLATION_MODE_SUPPORTED 1

#include <algorithm>
#include <cmath>

// Ensure no previous defines interfere with the Direct2D API headers
#undef GetHwnd

#include <d2d1.h>

#include <d2d1effectauthor.h>

#include <wincodec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if wxUSE_GRAPHICS_DIRECT2D

#ifndef WX_PRECOMP
// add individual includes here
#endif

#include "wx/graphics.h"
#include "wx/dc.h"
#include "wx/private/graphics.h"
#include "wx/stack.h"

static IWICImagingFactory* gs_WICImagingFactory = NULL;

IWICImagingFactory* WICImagingFactory() {
    if (gs_WICImagingFactory == NULL) {
        CoCreateInstance(
            CLSID_WICImagingFactory,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IWICImagingFactory,
            (LPVOID*)&gs_WICImagingFactory);
    }
    return gs_WICImagingFactory;
}

extern WXDLLIMPEXP_DATA_CORE(wxGraphicsPen) wxNullGraphicsPen;
extern WXDLLIMPEXP_DATA_CORE(wxGraphicsBrush) wxNullGraphicsBrush;

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

#if D2D1_BLEND_SUPPORTED
bool CompositionModeSupported(wxCompositionMode compositionMode)
{
    if (compositionMode == wxCOMPOSITION_DEST || compositionMode == wxCOMPOSITION_CLEAR || compositionMode == wxCOMPOSITION_INVALID)
    {
        return false;
    }

    return true;
}

D2D1_COMPOSITE_MODE ConvertCompositionMode(wxCompositionMode compositionMode)
{
    switch (compositionMode)
    {
    case wxCOMPOSITION_SOURCE:
        return D2D1_COMPOSITE_MODE_SOURCE_COPY;
    case wxCOMPOSITION_OVER:
        return D2D1_COMPOSITE_MODE_SOURCE_OVER;
    case wxCOMPOSITION_IN:
        return D2D1_COMPOSITE_MODE_SOURCE_IN;
    case wxCOMPOSITION_OUT:
        return D2D1_COMPOSITE_MODE_SOURCE_OUT;
    case wxCOMPOSITION_ATOP:
        return D2D1_COMPOSITE_MODE_SOURCE_ATOP;
    case wxCOMPOSITION_DEST_OVER:
        return D2D1_COMPOSITE_MODE_DESTINATION_OVER;
    case wxCOMPOSITION_DEST_IN:
        return D2D1_COMPOSITE_MODE_DESTINATION_IN;
    case wxCOMPOSITION_DEST_OUT:
        return D2D1_COMPOSITE_MODE_DESTINATION_OUT;
    case wxCOMPOSITION_DEST_ATOP:
        return D2D1_COMPOSITE_MODE_DESTINATION_ATOP;
    case wxCOMPOSITION_XOR:
        return D2D1_COMPOSITE_MODE_XOR;
    case wxCOMPOSITION_ADD:
        return D2D1_COMPOSITE_MODE_PLUS;

    // unsupported composition modes
    case wxCOMPOSITION_DEST:
        wxFALLTHROUGH;
    case wxCOMPOSITION_CLEAR:
        wxFALLTHROUGH;
    case wxCOMPOSITION_INVALID:
        return D2D1_COMPOSITE_MODE_SOURCE_COPY;
    }

    return D2D1_COMPOSITE_MODE_SOURCE_COPY;
}
#endif // D2D1_BLEND_SUPPORTED

#if D2D1_INTERPOLATION_MODE_SUPPORTED
D2D1_INTERPOLATION_MODE ConvertInterpolationQuality(wxInterpolationQuality interpolationQuality)
{
    switch (interpolationQuality)
    {
    case wxINTERPOLATION_DEFAULT:
        wxFALLTHROUGH;
    case wxINTERPOLATION_NONE:
        wxFALLTHROUGH;
    case wxINTERPOLATION_FAST:
        return D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
    case wxINTERPOLATION_GOOD:
        return D2D1_INTERPOLATION_MODE_LINEAR;
    case wxINTERPOLATION_BEST:
        return D2D1_INTERPOLATION_MODE_CUBIC;
    }

    return D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
}
#endif // D2D1_INTERPOLATION_MODE_SUPPORTED

D2D1_BITMAP_INTERPOLATION_MODE ConvertBitmapInterpolationMode(wxInterpolationQuality interpolationQuality)
{
    switch (interpolationQuality)
    {
    case wxINTERPOLATION_DEFAULT:
        wxFALLTHROUGH;
    case wxINTERPOLATION_NONE:
        wxFALLTHROUGH;
    case wxINTERPOLATION_FAST:
        wxFALLTHROUGH;
    case wxINTERPOLATION_GOOD:
        return D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
    case wxINTERPOLATION_BEST:
        return D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;
    }

    return D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
}

class wxD2DOffsetHelper
{
public:
    wxD2DOffsetHelper(wxGraphicsContext* g) : m_context(g)
    {
        if (m_context->ShouldOffset())
        {
            m_context->Translate(0.5, 0.5);
        }
    }

    ~wxD2DOffsetHelper()
    {
        if (m_context->ShouldOffset())
        {
            m_context->Translate(-0.5, -0.5);
        }
    }

private:
    wxGraphicsContext* m_context;
};

bool operator==(const D2D1::Matrix3x2F& lhs, const D2D1::Matrix3x2F& rhs)
{
    return 
        lhs._11 == rhs._11 && lhs._12 == rhs._12 &&
        lhs._21 == rhs._21 && lhs._22 == rhs._22 &&
        lhs._31 == rhs._31 && lhs._32 == rhs._32;
}

// Interface used by objects holding Direct2D device-dependent resources.
class DeviceDependentResourceHolder
{
public:
    virtual void AcquireDeviceDependentResources(ID2D1RenderTarget* renderTarget) = 0;
    virtual void ReleaseDeviceDependentResources() = 0;
};

//-----------------------------------------------------------------------------
// wxD2DMatrixData declaration
//-----------------------------------------------------------------------------

class wxD2DMatrixData : public wxGraphicsMatrixData
{
public:
    wxD2DMatrixData(wxGraphicsRenderer* renderer);
    wxD2DMatrixData(wxGraphicsRenderer* renderer, const D2D1::Matrix3x2F& matrix);

    void Concat(const wxGraphicsMatrixData* t) wxOVERRIDE;

    void Set(wxDouble a = 1.0, wxDouble b = 0.0, wxDouble c = 0.0, wxDouble d = 1.0,
        wxDouble tx = 0.0, wxDouble ty = 0.0) wxOVERRIDE;

    void Get(wxDouble* a = NULL, wxDouble* b = NULL,  wxDouble* c = NULL,
        wxDouble* d = NULL, wxDouble* tx = NULL, wxDouble* ty = NULL) const wxOVERRIDE;

    void Invert() wxOVERRIDE;

    bool IsEqual(const wxGraphicsMatrixData* t) const wxOVERRIDE;

    bool IsIdentity() const wxOVERRIDE;

    void Translate(wxDouble dx, wxDouble dy) wxOVERRIDE;

    void Scale(wxDouble xScale, wxDouble yScale) wxOVERRIDE;

    void Rotate(wxDouble angle) wxOVERRIDE;

    void TransformPoint(wxDouble* x, wxDouble* y) const wxOVERRIDE;

    void TransformDistance(wxDouble* dx, wxDouble* dy) const wxOVERRIDE;

    void* GetNativeMatrix() const wxOVERRIDE;

    D2D1::Matrix3x2F GetMatrix3x2F() const;

private:
    D2D1::Matrix3x2F m_matrix;
};

//-----------------------------------------------------------------------------
// wxD2DMatrixData implementation
//-----------------------------------------------------------------------------

wxD2DMatrixData::wxD2DMatrixData(wxGraphicsRenderer* renderer) : wxGraphicsMatrixData(renderer)
{
    m_matrix = D2D1::Matrix3x2F::Identity();
}

wxD2DMatrixData::wxD2DMatrixData(wxGraphicsRenderer* renderer, const D2D1::Matrix3x2F& matrix) : 
    wxGraphicsMatrixData(renderer), m_matrix(matrix)
{
}

void wxD2DMatrixData::Concat(const wxGraphicsMatrixData* t)
{
    m_matrix.SetProduct(m_matrix, static_cast<const wxD2DMatrixData*>(t)->m_matrix);
}

void wxD2DMatrixData::Set(wxDouble a, wxDouble b, wxDouble c, wxDouble d, wxDouble tx, wxDouble ty)
{
    m_matrix._11 = a;
    m_matrix._12 = b;
    m_matrix._21 = c;
    m_matrix._22 = d;
    m_matrix._31 = tx;
    m_matrix._32 = ty;
}

void wxD2DMatrixData::Get(wxDouble* a, wxDouble* b,  wxDouble* c, wxDouble* d, wxDouble* tx, wxDouble* ty) const
{
    *a = m_matrix._11;
    *b = m_matrix._12;
    *c = m_matrix._21;
    *d = m_matrix._22;
    *tx = m_matrix._31;
    *ty = m_matrix._32;
}

void wxD2DMatrixData::Invert()
{
    m_matrix.Invert();
}

bool wxD2DMatrixData::IsEqual(const wxGraphicsMatrixData* t) const
{
    return m_matrix == static_cast<const wxD2DMatrixData*>(t)->m_matrix;
}

bool wxD2DMatrixData::IsIdentity() const
{
    return m_matrix.IsIdentity();
}

void wxD2DMatrixData::Translate(wxDouble dx, wxDouble dy)
{
    m_matrix.SetProduct(D2D1::Matrix3x2F::Translation(dx, dy), m_matrix);
}

void wxD2DMatrixData::Scale(wxDouble xScale, wxDouble yScale)
{
    m_matrix.SetProduct(D2D1::Matrix3x2F::Scale(xScale, yScale), m_matrix);
}

void wxD2DMatrixData::Rotate(wxDouble angle)
{
    m_matrix.SetProduct(D2D1::Matrix3x2F::Rotation(wxRadToDeg(angle)), m_matrix);
}

void wxD2DMatrixData::TransformPoint(wxDouble* x, wxDouble* y) const
{
    D2D1_POINT_2F result = m_matrix.TransformPoint(D2D1::Point2F(*x, *y));
    *x = result.x;
    *y = result.y;
}

void wxD2DMatrixData::TransformDistance(wxDouble* dx, wxDouble* dy) const
{
    D2D1::Matrix3x2F noTranslationMatrix = m_matrix;
    noTranslationMatrix._31 = 0;
    noTranslationMatrix._32 = 0;
    D2D1_POINT_2F result = m_matrix.TransformPoint(D2D1::Point2F(*dx, *dy));
    *dx = result.x;
    *dy = result.y;
}

void* wxD2DMatrixData::GetNativeMatrix() const
{
    return (void*)&m_matrix;
}

D2D1::Matrix3x2F wxD2DMatrixData::GetMatrix3x2F() const
{
    return m_matrix;
}

const wxD2DMatrixData* GetD2DMatrixData(const wxGraphicsMatrix& matrix)
{
    return static_cast<const wxD2DMatrixData*>(matrix.GetMatrixData());
}

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
    wxD2DPathData(wxGraphicsRenderer* renderer, ID2D1Factory* d2dFactory);

    ~wxD2DPathData();

    ID2D1PathGeometry* GetPathGeometry();

    // This closes the geometry sink, ensuring all the figures are stored inside
    // the ID2D1PathGeometry. Calling this method is required before any draw operation
    // involving a path.
    void Flush();

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

    // returns the native path
    void* GetNativePath() const wxOVERRIDE;

    // give the native path returned by GetNativePath() back (there might be some deallocations necessary)
    void UnGetNativePath(void* WXUNUSED(p)) const wxOVERRIDE {};

    // transforms each point of this path by the matrix
    void Transform(const wxGraphicsMatrixData* matrix) wxOVERRIDE;

    // gets the bounding box enclosing all points (possibly including control points)
    void GetBox(wxDouble* x, wxDouble* y, wxDouble* w, wxDouble *h) const wxOVERRIDE;

    bool Contains(wxDouble x, wxDouble y, wxPolygonFillMode fillStyle = wxODDEVEN_RULE) const wxOVERRIDE;

    // appends an ellipsis as a new closed subpath fitting the passed rectangle
    void AddCircle(wxDouble x, wxDouble y, wxDouble r) wxOVERRIDE;

    // appends an ellipse
    void AddEllipse(wxDouble x, wxDouble y, wxDouble w, wxDouble h) wxOVERRIDE;

private:
    void EnsureGeometryOpen();

    void EnsureSinkOpen();

    void EnsureFigureOpen(wxDouble x = 0, wxDouble y = 0);

private :
    ID2D1PathGeometry* m_pathGeometry;

    ID2D1GeometrySink* m_geometrySink;

    ID2D1Factory* m_direct2dfactory;

    D2D1_POINT_2F m_currentPoint;

    D2D1_MATRIX_3X2_F m_transformMatrix;

    bool m_figureOpened;

    bool m_geometryWritable;
};

//-----------------------------------------------------------------------------
// wxD2DPathData implementation
//-----------------------------------------------------------------------------

wxD2DPathData::wxD2DPathData(wxGraphicsRenderer* renderer, ID2D1Factory* d2dFactory) : 
    wxGraphicsPathData(renderer), m_geometrySink(NULL), m_direct2dfactory(d2dFactory),
    m_pathGeometry(NULL), m_figureOpened(false), m_geometryWritable(false),
    m_transformMatrix(D2D1::Matrix3x2F::Identity())
{
    m_direct2dfactory->CreatePathGeometry(&m_pathGeometry);
}

wxD2DPathData::~wxD2DPathData()
{
    Flush();
    SafeRelease(&m_geometrySink);
    SafeRelease(&m_pathGeometry);
}

ID2D1PathGeometry* wxD2DPathData::GetPathGeometry()
{
    return m_pathGeometry;
}

wxD2DPathData::wxGraphicsObjectRefData* wxD2DPathData::Clone() const
{
    wxFAIL_MSG("not implemented");
    return NULL;
}

void wxD2DPathData::Flush()
{
    if (m_geometrySink != NULL)
    {
        if (m_figureOpened)
        {
            m_geometrySink->EndFigure(D2D1_FIGURE_END_OPEN);
        }

        m_figureOpened = false;
        m_geometrySink->Close();
        SafeRelease(&m_geometrySink);

        m_geometryWritable = false;
    }
}

void wxD2DPathData::EnsureGeometryOpen()
{
    if (!m_geometryWritable) {
        ID2D1PathGeometry* newPathGeometry;
        m_direct2dfactory->CreatePathGeometry(&newPathGeometry);

        newPathGeometry->Open(&m_geometrySink);

        if (m_pathGeometry != NULL)
        {
            m_pathGeometry->Stream(m_geometrySink);
            SafeRelease(&m_pathGeometry);
        }

        m_pathGeometry = newPathGeometry;
        m_geometryWritable = true;
    }
}

void wxD2DPathData::EnsureSinkOpen()
{
    EnsureGeometryOpen();

    if (m_geometrySink == NULL)
    {
        m_pathGeometry->Open(&m_geometrySink);
    }
}

void wxD2DPathData::EnsureFigureOpen(wxDouble x, wxDouble y)
{
    EnsureSinkOpen();

    if (!m_figureOpened)
    {
        m_geometrySink->BeginFigure(D2D1::Point2F(x, y), D2D1_FIGURE_BEGIN_FILLED);
        m_figureOpened = true;
        m_currentPoint = D2D1::Point2F(x, y);
    }
}

void wxD2DPathData::MoveToPoint(wxDouble x, wxDouble y)
{
    if (m_figureOpened)
    {
        CloseSubpath();
    }

    EnsureFigureOpen(x, y);

    m_currentPoint = D2D1::Point2F(x, y);
}

// adds a straight line from the current point to (x,y)
void wxD2DPathData::AddLineToPoint(wxDouble x, wxDouble y)
{
    EnsureFigureOpen();
    m_geometrySink->AddLine(D2D1::Point2F(x, y));

    m_currentPoint = D2D1::Point2F(x, y);
}

// adds a cubic Bezier curve from the current point, using two control points and an end point
void wxD2DPathData::AddCurveToPoint(wxDouble cx1, wxDouble cy1, wxDouble cx2, wxDouble cy2, wxDouble x, wxDouble y)
{
    EnsureFigureOpen();
    D2D1_BEZIER_SEGMENT bezierSegment = {
        {cx1, cy1},
        {cx2, cy2},
        {x, y}};
    m_geometrySink->AddBezier(bezierSegment);

    m_currentPoint = D2D1::Point2F(x, y);
}

// adds an arc of a circle centering at (x,y) with radius (r) from startAngle to endAngle
void wxD2DPathData::AddArc(wxDouble x, wxDouble y, wxDouble r, wxDouble startAngle, wxDouble endAngle, bool clockwise)
{
    static wxDouble PI = std::atan(1) * 4;
    wxPoint2DDouble center = wxPoint2DDouble(x, y);
    wxPoint2DDouble start = wxPoint2DDouble(std::cos(startAngle) * r, std::sin(startAngle) * r);
    wxPoint2DDouble end = wxPoint2DDouble(std::cos(endAngle) * r, std::sin(endAngle) * r);

    if (m_figureOpened) 
    {
        AddLineToPoint(start.m_x + x, start.m_y + y);
    }
    else
    {
        MoveToPoint(start.m_x + x, start.m_y + y);
    }

    double angle = (end.GetVectorAngle() - start.GetVectorAngle());

    if (!clockwise)
    {
        angle = 360 - angle;
    }

    while (abs(angle) > 360)
    {
        angle -= (angle / abs(angle)) * 360;
    }

    if (angle == 360) 
    {
        AddCircle(center.m_x, center.m_y, start.GetVectorLength());
        return;
    }

    D2D1_SWEEP_DIRECTION sweepDirection = clockwise ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE;

    D2D1_ARC_SIZE arcSize = angle > 180 ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL;

    D2D1_ARC_SEGMENT arcSegment = {
        {end.m_x + x, end.m_y + y},     // end point
        {r, r},                         // size
        0,                              // rotation
        sweepDirection,                 // sweep direction
        arcSize                         // arc size
    };

    m_geometrySink->AddArc(arcSegment);

    m_currentPoint = D2D1::Point2F(end.m_x + x, end.m_y + y);
}

// appends an ellipsis as a new closed subpath fitting the passed rectangle
void wxD2DPathData::AddCircle(wxDouble x, wxDouble y, wxDouble r) 
{
    AddEllipse(x - r, y - r, r * 2, r * 2);
}

// appends an ellipse
void wxD2DPathData::AddEllipse(wxDouble x, wxDouble y, wxDouble w, wxDouble h)
{
    Flush();

    ID2D1EllipseGeometry* ellipseGeometry;
    ID2D1PathGeometry* newPathGeometry;

    D2D1_ELLIPSE ellipse = { {x + w / 2, y + h / 2}, w / 2, h / 2 };

    m_direct2dfactory->CreateEllipseGeometry(ellipse, &ellipseGeometry);

    m_direct2dfactory->CreatePathGeometry(&newPathGeometry);

    newPathGeometry->Open(&m_geometrySink);
    m_geometryWritable = true;

    ellipseGeometry->CombineWithGeometry(m_pathGeometry, D2D1_COMBINE_MODE_UNION, NULL, m_geometrySink);

    SafeRelease(&m_pathGeometry);
    SafeRelease(&ellipseGeometry);

    m_pathGeometry = newPathGeometry;
}

// gets the last point of the current path, (0,0) if not yet set
void wxD2DPathData::GetCurrentPoint(wxDouble* x, wxDouble* y) const
{
    D2D1_POINT_2F transformedPoint = D2D1::Matrix3x2F::ReinterpretBaseType(&m_transformMatrix)->TransformPoint(m_currentPoint);

    *x = transformedPoint.x;
    *y = transformedPoint.y;
}

// adds another path
void wxD2DPathData::AddPath(const wxGraphicsPathData* path)
{
    const wxD2DPathData* d2dPath = static_cast<const wxD2DPathData*>(path);

    EnsureFigureOpen();

    d2dPath->m_pathGeometry->Stream(m_geometrySink);
}

// closes the current sub-path
void wxD2DPathData::CloseSubpath()
{
    if (m_figureOpened) 
    {
        m_geometrySink->EndFigure(D2D1_FIGURE_END_CLOSED);
        m_figureOpened = false;
    }
}

void* wxD2DPathData::GetNativePath() const
{
    ID2D1TransformedGeometry* transformedGeometry;
    m_direct2dfactory->CreateTransformedGeometry(m_pathGeometry, m_transformMatrix, &transformedGeometry);
    return transformedGeometry;
}

void wxD2DPathData::Transform(const wxGraphicsMatrixData* matrix)
{
    m_transformMatrix = *((D2D1_MATRIX_3X2_F*)(matrix->GetNativeMatrix()));
}

void wxD2DPathData::GetBox(wxDouble* x, wxDouble* y, wxDouble* w, wxDouble *h) const
{
    D2D1_RECT_F bounds;
    m_pathGeometry->GetBounds(D2D1::Matrix3x2F::Identity(), &bounds);
    *x = bounds.left;
    *y = bounds.top;
    *w = bounds.right - bounds.left;
    *h = bounds.bottom - bounds.top;
}

bool wxD2DPathData::Contains(wxDouble x, wxDouble y, wxPolygonFillMode fillStyle) const
{
    BOOL result;
    m_pathGeometry->FillContainsPoint(D2D1::Point2F(x, y), D2D1::Matrix3x2F::Identity(), &result);
    return result != 0;
}

wxD2DPathData* GetD2DPathData(const wxGraphicsPath& path)
{
    return static_cast<wxD2DPathData*>(path.GetGraphicsData());
}

//-----------------------------------------------------------------------------
// wxD2DBitmapData declaration
//-----------------------------------------------------------------------------

class wxD2DBitmapData : public wxGraphicsBitmapData, public DeviceDependentResourceHolder
{
public:

    // Structure containing both the source bitmap and the native bitmap.
    // This is required by the GetNativeBitmap method and the constructor
    // from a native bitmap pointer. The reason is: for device-dependent
    // resources we must make sure to keep the source object in case we
    // need to re-acquire the resource later.
    typedef struct  
    {
        wxBitmap m_sourceBitmap;
        ID2D1Bitmap* m_nativeBitmap;
    } PseudoNativeBitmap;

    wxD2DBitmapData(wxGraphicsRenderer* renderer, const wxBitmap& bitmap);
    wxD2DBitmapData(wxGraphicsRenderer* renderer, const void* pseudoNativeBitmap);
    ~wxD2DBitmapData();

    // returns the native representation
    void* GetNativeBitmap() const wxOVERRIDE;

    void AcquireDeviceDependentResources(ID2D1RenderTarget* renderTarget) wxOVERRIDE;

    void ReleaseDeviceDependentResources() wxOVERRIDE;

    ID2D1Bitmap* GetD2DBitmap() { return m_impl.m_nativeBitmap; }

private:
    wxBitmap& SourceBitmap() { return m_impl.m_sourceBitmap; }

    ID2D1Bitmap*& NativeBitmap() { return m_impl.m_nativeBitmap; }

private:
    PseudoNativeBitmap m_impl;
};

// RAII class hosting a WIC bitmap lock used for writing pixel data
class BitmapPixelWriteLock
{
public:
    BitmapPixelWriteLock(IWICBitmap* bitmap) : m_pixelLock(NULL)
    {
        // Retrieve the size of the bitmap
        UINT w, h;
        bitmap->GetSize(&w, &h);
        WICRect lockSize = {0, 0, w, h};

        // Obtain a bitmap lock for exclusive write.
        bitmap->Lock(&lockSize, WICBitmapLockWrite, &m_pixelLock);
    }

    ~BitmapPixelWriteLock()
    {
        m_pixelLock->Release();
    }

private:
    IWICBitmapLock *m_pixelLock;
};

//-----------------------------------------------------------------------------
// wxD2DBitmapData implementation
//-----------------------------------------------------------------------------

wxD2DBitmapData::wxD2DBitmapData(wxGraphicsRenderer* renderer, const wxBitmap& bitmap) : 
    wxGraphicsBitmapData(renderer)
{
    m_impl.m_sourceBitmap = bitmap;
}

wxD2DBitmapData::wxD2DBitmapData(wxGraphicsRenderer* renderer, const void* pseudoNativeBitmap) : 
    wxGraphicsBitmapData(renderer)
{
    wxASSERT_MSG(pseudoNativeBitmap == NULL, "Cannot create a bitmap data from a null native bitmap");
    m_impl = *static_cast<const PseudoNativeBitmap*>(pseudoNativeBitmap);
}

wxD2DBitmapData::~wxD2DBitmapData()
{
    ReleaseDeviceDependentResources();
}

void* wxD2DBitmapData::GetNativeBitmap() const 
{
    return (void*)&m_impl;
}

IWICBitmapSource* CreateWICBitmap(const WXHBITMAP sourceBitmap, bool hasAlpha = false)
{
    HRESULT hr;

    IWICBitmap* wicBitmap;
    WICImagingFactory()->CreateBitmapFromHBITMAP(sourceBitmap, NULL, WICBitmapUseAlpha, &wicBitmap);

    IWICFormatConverter* converter;
    hr = WICImagingFactory()->CreateFormatConverter(&converter);

    WICPixelFormatGUID pixelFormat = hasAlpha ? GUID_WICPixelFormat32bppPBGRA : GUID_WICPixelFormat32bppBGR;

    hr = converter->Initialize(
        wicBitmap,
        pixelFormat,
        WICBitmapDitherTypeNone,NULL,0.f, 
        WICBitmapPaletteTypeMedianCut);

    wicBitmap->Release();

    return converter;
}

IWICBitmapSource* CreateWICBitmap(const wxBitmap& sourceBitmap, bool hasAlpha = false)
{
    return CreateWICBitmap(sourceBitmap.GetHBITMAP());
}

bool HasAlpha(const wxBitmap& bitmap)
{
    return bitmap.HasAlpha() || bitmap.GetMask();
}

void wxD2DBitmapData::AcquireDeviceDependentResources(ID2D1RenderTarget* renderTarget)
{
    HRESULT hr;

    IWICBitmapSource* wicBitmap = CreateWICBitmap(SourceBitmap(), HasAlpha(SourceBitmap()));

    hr = renderTarget->CreateBitmapFromWicBitmap(wicBitmap, 0, &NativeBitmap());

    wicBitmap->Release();

    return;
}

void wxD2DBitmapData::ReleaseDeviceDependentResources()
{
    SafeRelease(&NativeBitmap());
}

wxD2DBitmapData* GetD2DBitmapData(const wxGraphicsBitmap& bitmap)
{
    return static_cast<wxD2DBitmapData*>(bitmap.GetRefData());
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
    // device-dependent resources.
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
    if (m_width <= 0) {
        m_width = 1;
    }

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

    int dashCount = 0;
    FLOAT* dashes = NULL;

    if (dashStyle == D2D1_DASH_STYLE_CUSTOM)
    {
        dashCount = m_sourcePen.GetDashCount();
        dashes = new FLOAT[dashCount];

        for (int i = 0; i < dashCount; ++i)
        {
            dashes[i] = m_sourcePen.GetDash()[i];
        }

    }

    direct2dfactory->CreateStrokeStyle(
        D2D1::StrokeStyleProperties(capStyle, capStyle, capStyle, lineJoin, 0, dashStyle, 0.0f),
        dashes, dashCount, 
        &m_strokeStyle);
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

wxD2DPenData* GetD2DPenData(const wxGraphicsPen& pen)
{
    return static_cast<wxD2DPenData*>(pen.GetGraphicsData());
}

// Helper class used to create and safely release a ID2D1GradientStopCollection from wxGraphicsGradientStops
class D2DGradientStopsHelper
{
public:
    D2DGradientStopsHelper(const wxGraphicsGradientStops& gradientStops, ID2D1RenderTarget* renderTarget) 
        : m_gradientStopCollection(NULL)
    {
        int stopCount = gradientStops.GetCount();

        m_gradientStopArray = new D2D1_GRADIENT_STOP[stopCount];

        for (int i = 0; i < stopCount; ++i)
        {
            m_gradientStopArray[i].color = ConvertColour(gradientStops.Item(i).GetColour());
            m_gradientStopArray[i].position = gradientStops.Item(i).GetPosition();
        }

        renderTarget->CreateGradientStopCollection(m_gradientStopArray, stopCount, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &m_gradientStopCollection);
    }

    ~D2DGradientStopsHelper()
    {
        delete[] m_gradientStopArray;
        SafeRelease(&m_gradientStopCollection);
    }

    ID2D1GradientStopCollection* GetGradientStopCollection()
    {
        return m_gradientStopCollection;
    }

private:
    D2D1_GRADIENT_STOP* m_gradientStopArray;
    ID2D1GradientStopCollection* m_gradientStopCollection;
};

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

    ID2D1Brush* GetBrush() const;

    void AcquireDeviceDependentResources(ID2D1RenderTarget* renderTarget) wxOVERRIDE;

    void ReleaseDeviceDependentResources() wxOVERRIDE;

private:
    void CreateSolidColorBrush();

    void CreateBitmapBrush();

    void AcquireSolidColorBrush(ID2D1RenderTarget* renderTarget);

    void AcquireLinearGradientBrush(ID2D1RenderTarget* renderTarget);

    void AcquireRadialGradientBrush(ID2D1RenderTarget* renderTarget);

    void AcquireBitmapBrush(ID2D1RenderTarget* renderTarget);

private:
    // We store the source brush for later when we need to recreate the
    // device-dependent resources.
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
}

void wxD2DBrushData::CreateRadialGradientBrush(
    wxDouble xo, wxDouble yo,
    wxDouble xc, wxDouble yc,
    wxDouble radius,
    const wxGraphicsGradientStops& stops)
{
    m_brushType = wxD2DBRUSHTYPE_RADIAL_GRADIENT;
    m_radialGradientBrushInfo = new RadialGradientBrushInfo(xo, yo, xc, yc, radius, stops);
}

void wxD2DBrushData::AcquireSolidColorBrush(ID2D1RenderTarget* renderTarget)
{
    if (!IsAcquired(m_solidColorBrush))
    {
        renderTarget->CreateSolidColorBrush(ConvertColour(m_sourceBrush.GetColour()), &m_solidColorBrush);
    }
}

void wxD2DBrushData::AcquireLinearGradientBrush(ID2D1RenderTarget* renderTarget)
{
    if (!IsAcquired(m_linearGradientBrush))
    {

        D2DGradientStopsHelper helper(m_linearGradientBrushInfo->stops, renderTarget);

        renderTarget->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(m_linearGradientBrushInfo->x1, m_linearGradientBrushInfo->y1),
                D2D1::Point2F(m_linearGradientBrushInfo->x2, m_linearGradientBrushInfo->y2)),
            helper.GetGradientStopCollection(),
            &m_linearGradientBrush);
    }
}

void wxD2DBrushData::AcquireRadialGradientBrush(ID2D1RenderTarget* renderTarget)
{
    if (!IsAcquired(m_radialGradientBrush))
    {
        D2DGradientStopsHelper helper(m_radialGradientBrushInfo->stops, renderTarget);

        int xo = m_radialGradientBrushInfo->x1 - m_radialGradientBrushInfo->x2;
        int yo = m_radialGradientBrushInfo->y1 - m_radialGradientBrushInfo->y2;

        renderTarget->CreateRadialGradientBrush(
            D2D1::RadialGradientBrushProperties(
                D2D1::Point2F(m_radialGradientBrushInfo->x2, m_radialGradientBrushInfo->y2),
                D2D1::Point2F(xo, yo),
                m_radialGradientBrushInfo->radius, m_radialGradientBrushInfo->radius),
            helper.GetGradientStopCollection(),
            &m_radialGradientBrush);
    }
}

void wxD2DBrushData::AcquireBitmapBrush(ID2D1RenderTarget* renderTarget)
{
    wxFAIL_MSG("not implemented");
}

void wxD2DBrushData::AcquireDeviceDependentResources(ID2D1RenderTarget* renderTarget)
{
    switch (m_brushType)
    {
    case wxD2DBrushData::wxD2DBRUSHTYPE_SOLID:
        AcquireSolidColorBrush(renderTarget);
        break;
    case wxD2DBrushData::wxD2DBRUSHTYPE_LINEAR_GRADIENT:
        AcquireLinearGradientBrush(renderTarget);
        break;
    case wxD2DBrushData::wxD2DBRUSHTYPE_RADIAL_GRADIENT:
        AcquireRadialGradientBrush(renderTarget);
        break;
    case wxD2DBrushData::wxD2DBRUSHTYPE_BITMAP:
        AcquireBitmapBrush(renderTarget);
        break;
    case wxD2DBrushData::wxD2DBRUSHTYPE_UNSPECIFIED:
        wxFAIL_MSG("cannot acquire resources for an uninitialized brush");
        break;
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

wxD2DBrushData* GetD2DBrushData(const wxGraphicsBrush& brush)
{
    return static_cast<wxD2DBrushData*>(brush.GetGraphicsData());
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

    // The native context used by wxD2DContext is a Direct2D render target.
    void* GetNativeContext() wxOVERRIDE;

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

    void StrokePath(const wxGraphicsPath& p) wxOVERRIDE;

    void FillPath(const wxGraphicsPath& p , wxPolygonFillMode fillStyle = wxODDEVEN_RULE) wxOVERRIDE;

    void DrawRectangle(wxDouble x, wxDouble y, wxDouble w, wxDouble h) wxOVERRIDE; 

    void DrawRoundedRectangle(wxDouble x, wxDouble y, wxDouble w, wxDouble h, wxDouble radius) wxOVERRIDE;

    void DrawEllipse(wxDouble x, wxDouble y, wxDouble w, wxDouble h) wxOVERRIDE;

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
    enum RenderTargetType
    {
        RTT_HWND,
        RTT_DC
    };

private:
    void DoDrawText(const wxString& str, wxDouble x, wxDouble y) wxOVERRIDE;

    void EnsureInitialized();
    HRESULT CreateRenderTarget();

    void AdjustRenderTargetSize();

    void ReleaseDeviceDependentResources();

    ID2D1RenderTarget* GetRenderTarget() const;

private:
    RenderTargetType m_renderTargetType;

    HWND m_hwnd;

    ID2D1Factory* m_direct2dFactory;

    ID2D1HwndRenderTarget* m_hwndRenderTarget;

    ID2D1DCRenderTarget* m_dcRenderTarget;

    // A ID2D1DrawingStateBlock represents the drawing state of a render target: 
    // the antialiasing mode, transform, tags, and text-rendering options.
    // The context owns these pointers and is responsible for releasing them.
    wxStack<ID2D1DrawingStateBlock*> m_stateStack;

    // The context does not own these pointers and is not responsible for
    // releasing them.
    wxVector<DeviceDependentResourceHolder*> m_deviceDependentResourceHolders;

    bool m_isClipped;

private:
    wxDECLARE_NO_COPY_CLASS(wxD2DContext);
};

//-----------------------------------------------------------------------------
// wxD2DContext implementation
//-----------------------------------------------------------------------------

wxD2DContext::wxD2DContext(wxGraphicsRenderer* renderer, ID2D1Factory* direct2dFactory, HWND hwnd) : wxGraphicsContext(renderer),
    m_renderTargetType(RTT_HWND), m_hwnd(hwnd),m_hwndRenderTarget(NULL), m_dcRenderTarget(NULL), 
    m_direct2dFactory(direct2dFactory)
{
    m_enableOffset = true;
}

wxD2DContext::~wxD2DContext()
{
    HRESULT result = GetRenderTarget()->EndDraw();

    // Release the objects saved on the state stack
    while(!m_stateStack.empty())
    {
        SafeRelease(&m_stateStack.top());
        m_stateStack.pop();
    }

    SafeRelease(&m_hwndRenderTarget);
    SafeRelease(&m_dcRenderTarget);
}

ID2D1RenderTarget* wxD2DContext::GetRenderTarget() const
{
    switch (m_renderTargetType)
    {
    case wxD2DContext::RTT_HWND:
        return m_hwndRenderTarget;
    case wxD2DContext::RTT_DC:
        return m_dcRenderTarget;
    }

    return NULL;
}

void wxD2DContext::Clip(const wxRegion& region)
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::Clip(wxDouble x, wxDouble y, wxDouble w, wxDouble h)
{
    ResetClip();

    GetRenderTarget()->PushAxisAlignedClip(
        D2D1::RectF(x, y, x + w, y + h),
        D2D1_ANTIALIAS_MODE_ALIASED);

    m_isClipped = true;
}

void wxD2DContext::ResetClip()
{
    if (m_isClipped)
    {
        GetRenderTarget()->PopAxisAlignedClip();
        m_isClipped = false;
    }
}

void* wxD2DContext::GetNativeContext()
{
    return GetRenderTarget();
}

void wxD2DContext::StrokePath(const wxGraphicsPath& p)
{
    wxD2DOffsetHelper helper(this);

    EnsureInitialized();
    AdjustRenderTargetSize();

    wxD2DPathData* pathData = GetD2DPathData(p);
    pathData->Flush();

    if (!m_pen.IsNull()) 
    {
        wxD2DPenData* penData = GetD2DPenData(m_pen);
        penData->AcquireDeviceDependentResources(GetRenderTarget());
        GetRenderTarget()->DrawGeometry((ID2D1Geometry*)pathData->GetNativePath(), penData->GetBrush(), penData->GetWidth(), penData->GetStrokeStyle());
    }
}

void wxD2DContext::FillPath(const wxGraphicsPath& p , wxPolygonFillMode WXUNUSED(fillStyle))
{
    EnsureInitialized();
    AdjustRenderTargetSize();

    wxD2DPathData* pathData = GetD2DPathData(p);
    pathData->Flush();

    if (!m_brush.IsNull()) 
    {
        wxD2DBrushData* brushData = GetD2DBrushData(m_brush);
        brushData->AcquireDeviceDependentResources(GetRenderTarget());
        GetRenderTarget()->FillGeometry((ID2D1Geometry*)pathData->GetNativePath(), brushData->GetBrush());
    }
}

bool wxD2DContext::SetAntialiasMode(wxAntialiasMode antialias)
{
    if (m_antialias == antialias)
    {
        return true;
    }

    GetRenderTarget()->SetAntialiasMode(ConvertAntialiasMode(antialias));

    m_antialias = antialias;
    return true;
}

bool wxD2DContext::SetInterpolationQuality(wxInterpolationQuality interpolation)
{
#if D2D1_INTERPOLATION_MODE_SUPPORTED
    m_interpolation = interpolation;
    return true;
#else
    return false;
#endif // D2D1_INTERPOLATION_MODE_SUPPORTED
}

bool wxD2DContext::SetCompositionMode(wxCompositionMode op)
{
#if D2D1_BLEND_SUPPORTED
    if (CompositionModeSupported(op))
    {
        m_composition = op;
        return true;
    }
    else 
    {
        return false;
    }
#else
    return false;
#endif // D2D1_BLEND_SUPPORTED
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
    wxGraphicsMatrix translationMatrix = CreateMatrix();
    translationMatrix.Translate(dx, dy);
    ConcatTransform(translationMatrix);
}

void wxD2DContext::Scale(wxDouble xScale, wxDouble yScale)
{
    wxGraphicsMatrix scaleMatrix = CreateMatrix();
    scaleMatrix.Scale(xScale, yScale);
    ConcatTransform(scaleMatrix);
}

void wxD2DContext::Rotate(wxDouble angle)
{
    wxGraphicsMatrix rotationMatrix = CreateMatrix();
    rotationMatrix.Rotate(angle);
    ConcatTransform(rotationMatrix);
}

void wxD2DContext::ConcatTransform(const wxGraphicsMatrix& matrix)
{
    D2D1::Matrix3x2F localMatrix = GetD2DMatrixData(GetTransform())->GetMatrix3x2F();
    D2D1::Matrix3x2F concatMatrix = GetD2DMatrixData(matrix)->GetMatrix3x2F();

    D2D1::Matrix3x2F resultMatrix;
    resultMatrix.SetProduct(concatMatrix, localMatrix);

    wxGraphicsMatrix resultTransform;
    resultTransform.SetRefData(new wxD2DMatrixData(GetRenderer(), resultMatrix));

    SetTransform(resultTransform);
}

void wxD2DContext::SetTransform(const wxGraphicsMatrix& matrix)
{
    EnsureInitialized();

    GetRenderTarget()->SetTransform(GetD2DMatrixData(matrix)->GetMatrix3x2F());
}

wxGraphicsMatrix wxD2DContext::GetTransform() const
{
    D2D1::Matrix3x2F transformMatrix;

    if (GetRenderTarget() != NULL) 
    {
        GetRenderTarget()->GetTransform(&transformMatrix);
    }
    else
    {
        transformMatrix = D2D1::Matrix3x2F::Identity();
    }

    wxD2DMatrixData* matrixData = new wxD2DMatrixData(GetRenderer(), transformMatrix);

    wxGraphicsMatrix matrix;
    matrix.SetRefData(matrixData);

    return matrix;
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
    ID2D1Factory* GetD2DFactory(wxGraphicsRenderer* renderer);

    ID2D1DrawingStateBlock* drawStateBlock;
    GetD2DFactory(GetRenderer())->CreateDrawingStateBlock(&drawStateBlock);
    GetRenderTarget()->SaveDrawingState(drawStateBlock);

    m_stateStack.push(drawStateBlock);
}

void wxD2DContext::PopState()
{
    wxCHECK_RET(!m_stateStack.empty(), wxT("No state to pop"));

    ID2D1DrawingStateBlock* drawStateBlock = m_stateStack.top();
    m_stateStack.pop();

    GetRenderTarget()->RestoreDrawingState(drawStateBlock);
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
    if (!m_enableOffset)
    {
        return false;
    }

    int penWidth = 0;
    if (!m_pen.IsNull())
    {
        penWidth = GetD2DPenData(m_pen)->GetWidth();
        penWidth = std::max(penWidth, 1);
    }

    return (penWidth % 2) == 1;
}

void wxD2DContext::DoDrawText(const wxString& str, wxDouble x, wxDouble y)
{
    wxFAIL_MSG("not implemented");
}

void wxD2DContext::EnsureInitialized()
{
    if (GetRenderTarget() == NULL) 
    {
        HRESULT result;

        result = CreateRenderTarget();

        if (FAILED(result))
        {
            wxFAIL_MSG("Could not create Direct2D render target");
        }

        GetRenderTarget()->SetTransform(D2D1::Matrix3x2F::Identity());

        GetRenderTarget()->BeginDraw();
    }
}

HRESULT wxD2DContext::CreateRenderTarget()
{
    if (m_renderTargetType == RTT_HWND) 
    {
        RECT clientRect;
        GetClientRect(m_hwnd, &clientRect);

        D2D1_SIZE_U size = D2D1::SizeU(
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top);

        return m_direct2dFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &m_hwndRenderTarget);
    }

    return S_FALSE;
}

void wxD2DContext::SetPen(const wxGraphicsPen& pen)
{
    wxGraphicsContext::SetPen(pen);

    if ( !m_pen.IsNull() )
    {
        EnsureInitialized();

        wxD2DPenData* penData = GetD2DPenData(pen);
        penData->AcquireDeviceDependentResources(GetRenderTarget());
        m_deviceDependentResourceHolders.push_back(penData);
    }
}

void wxD2DContext::AdjustRenderTargetSize()
{
    if (m_hwndRenderTarget != NULL)
    {
        RECT clientRect;
        GetClientRect(m_hwnd, &clientRect);

        D2D1_SIZE_U hwndSize = D2D1::SizeU(
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top);

        D2D1_SIZE_U renderTargetSize = m_hwndRenderTarget->GetPixelSize();

        if (hwndSize.width != renderTargetSize.width || hwndSize.height != renderTargetSize.height)
        {
            m_hwndRenderTarget->Resize(hwndSize);
        }
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

void wxD2DContext::DrawRectangle(wxDouble x, wxDouble y, wxDouble w, wxDouble h)
{
    wxD2DOffsetHelper helper(this);

    EnsureInitialized();
    AdjustRenderTargetSize();

    D2D1_RECT_F rect = {x, y, x + w, y + h};


    if (!m_brush.IsNull())
    {
        wxD2DBrushData* brushData = GetD2DBrushData(m_brush);
        brushData->AcquireDeviceDependentResources(GetRenderTarget());
        GetRenderTarget()->FillRectangle(rect, brushData->GetBrush());
    }

    if (!m_pen.IsNull()) 
    {
        wxD2DPenData* penData = GetD2DPenData(m_pen);
        penData->AcquireDeviceDependentResources(GetRenderTarget());
        GetRenderTarget()->DrawRectangle(rect, penData->GetBrush(), penData->GetWidth(), penData->GetStrokeStyle());
    }
}

void wxD2DContext::DrawRoundedRectangle(wxDouble x, wxDouble y, wxDouble w, wxDouble h, wxDouble radius)
{
    wxD2DOffsetHelper helper(this);

    EnsureInitialized();
    AdjustRenderTargetSize();

    D2D1_RECT_F rect = {x, y, x + w, y + h};

    D2D1_ROUNDED_RECT roundedRect = {rect, radius, radius};

    if (!m_brush.IsNull())
    {
        wxD2DBrushData* brushData = GetD2DBrushData(m_brush);
        brushData->AcquireDeviceDependentResources(GetRenderTarget());
        GetRenderTarget()->FillRoundedRectangle(roundedRect, brushData->GetBrush());
    }

    if (!m_pen.IsNull())
    {
        wxD2DPenData* penData = GetD2DPenData(m_pen);
        penData->AcquireDeviceDependentResources(GetRenderTarget());
        GetRenderTarget()->DrawRoundedRectangle(roundedRect, penData->GetBrush(), penData->GetWidth(), penData->GetStrokeStyle());
    }
}

void wxD2DContext::DrawEllipse(wxDouble x, wxDouble y, wxDouble w, wxDouble h)
{
    wxD2DOffsetHelper helper(this);

    EnsureInitialized();
    AdjustRenderTargetSize();

    D2D1_ELLIPSE ellipse = {
        {(x + w / 2), (y + h / 2)}, // center point
        w / 2,                      // radius x
        h / 2                       // radius y
    };

    if (!m_brush.IsNull())
    {
        wxD2DBrushData* brushData = GetD2DBrushData(m_brush);
        brushData->AcquireDeviceDependentResources(GetRenderTarget());
        GetRenderTarget()->FillEllipse(ellipse, brushData->GetBrush());
    }

    if (!m_pen.IsNull())
    {
        wxD2DPenData* penData = GetD2DPenData(m_pen);
        penData->AcquireDeviceDependentResources(GetRenderTarget());
        GetRenderTarget()->DrawEllipse(ellipse, penData->GetBrush(), penData->GetWidth(), penData->GetStrokeStyle());
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

    ID2D1Factory* GetD2DFactory();

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
    return new wxD2DContext(this, m_direct2dFactory, (HWND)window);
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
    wxGraphicsPath p;
    p.SetRefData(new wxD2DPathData(this, m_direct2dFactory));

    return p;
}

wxGraphicsMatrix wxD2DRenderer::CreateMatrix(
    wxDouble a, wxDouble b, wxDouble c, wxDouble d,
    wxDouble tx, wxDouble ty)
{
    wxD2DMatrixData* matrixData = new wxD2DMatrixData(this);
    matrixData->Set(a, b, c, d, tx, ty);

    wxGraphicsMatrix matrix;
    matrix.SetRefData(matrixData);

    return matrix;
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
    if ( !brush.IsOk() || brush.GetStyle() == wxPENSTYLE_TRANSPARENT )
    {
        return wxNullGraphicsBrush;
    }
    else
    {
        wxGraphicsBrush b;
        b.SetRefData(new wxD2DBrushData(this, brush));
        return b;
    }
}


wxGraphicsBrush wxD2DRenderer::CreateLinearGradientBrush(
    wxDouble x1, wxDouble y1,
    wxDouble x2, wxDouble y2,
    const wxGraphicsGradientStops& stops)
{
    wxD2DBrushData* brushData = new wxD2DBrushData(this);
    brushData->CreateLinearGradientBrush(x1, y1, x2, y2, stops);

    wxGraphicsBrush brush;
    brush.SetRefData(brushData);

    return brush;
}

wxGraphicsBrush wxD2DRenderer::CreateRadialGradientBrush(
    wxDouble xo, wxDouble yo,
    wxDouble xc, wxDouble yc,
    wxDouble radius,
    const wxGraphicsGradientStops& stops)
{
    wxD2DBrushData* brushData = new wxD2DBrushData(this);
    brushData->CreateRadialGradientBrush(xo, yo, xc, yc, radius, stops);

    wxGraphicsBrush brush;
    brush.SetRefData(brushData);

    return brush;
}

// create a native bitmap representation
wxGraphicsBitmap wxD2DRenderer::CreateBitmap(const wxBitmap& bitmap)
{
    wxD2DBitmapData* bitmapData = new wxD2DBitmapData(this, bitmap);

    wxGraphicsBitmap graphicsBitmap;
    graphicsBitmap.SetRefData(bitmapData);

    return graphicsBitmap;
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

ID2D1Factory* wxD2DRenderer::GetD2DFactory()
{
    return m_direct2dFactory;
}

ID2D1Factory* GetD2DFactory(wxGraphicsRenderer* renderer)
{
    return static_cast<wxD2DRenderer*>(renderer)->GetD2DFactory();
}

#endif // wxUSE_GRAPHICS_DIRECT2D
