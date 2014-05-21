/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/graphicsd2d.cpp
// Purpose:     Implementation of Direct2D Render Context
// Author:      Pana Alexandru <astronothing@gmail.com>
// Created:     2014-05-20
// Copyright:   (c) 2014 wxWidgets development team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// add individual includes here
#endif

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

protected :

private :
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxD2DRenderer)
} ;

//-----------------------------------------------------------------------------
// wxD2DRenderer implementation
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxGDIPlusRenderer,wxGraphicsRenderer)

static wxGDIPlusRenderer gs_D2DRenderer;

wxGraphicsRenderer* wxGraphicsRenderer::GetDefaultRenderer()
{
    return &gs_D2DRenderer;
}

wxD2DRenderer::wxD2DRenderer()
{
    wxFAIL_MSG("not implemented");
}

wxD2DRenderer::~wxD2DRenderer()
{
    wxFAIL_MSG("not implemented");
}

wxGraphicsContext* wxD2DRenderer::CreateContext(const wxWindowDC& dc)
{
    wxFAIL_MSG("not implemented");
}

wxGraphicsContext* wxD2DRenderer::CreateContext(const wxMemoryDC& dc)
{
    wxFAIL_MSG("not implemented");
}

#if wxUSE_PRINTING_ARCHITECTURE
wxGraphicsContext* wxD2DRenderer::CreateContext(const wxPrinterDC& dc)
{
    wxFAIL_MSG("not implemented");
}
#endif

#if wxUSE_ENH_METAFILE
wxGraphicsContext* wxD2DRenderer::CreateContext(const wxEnhMetaFileDC& dc)
{
    wxFAIL_MSG("not implemented");
}
#endif

wxGraphicsContext* wxD2DRenderer::CreateContextFromNativeContext(void* context)
{
    wxFAIL_MSG("not implemented");
}

wxGraphicsContext* wxD2DRenderer::CreateContextFromNativeWindow(void* window)
{
    wxFAIL_MSG("not implemented");
}

wxGraphicsContext* wxD2DRenderer::CreateContext(wxWindow* window)
{
    wxFAIL_MSG("not implemented");
}

#if wxUSE_IMAGE
wxGraphicsContext* wxD2DRenderer::CreateContextFromImage(wxImage& image)
{
    wxFAIL_MSG("not implemented");
}
#endif // wxUSE_IMAGE

wxGraphicsContext* wxD2DRenderer::CreateMeasuringContext()
{
    wxFAIL_MSG("not implemented");
}

wxGraphicsPath wxD2DRenderer::CreatePath()
{
    wxFAIL_MSG("not implemented");
}

wxGraphicsMatrix wxD2DRenderer::CreateMatrix(
    wxDouble a = 1.0, wxDouble b = 0.0, wxDouble c = 0.0, wxDouble d = 1.0,
    wxDouble tx = 0.0, wxDouble ty = 0.0)
{
    wxFAIL_MSG("not implemented");
}

wxGraphicsPen wxD2DRenderer::CreatePen(const wxPen& pen)
{
    wxFAIL_MSG("not implemented");
}

wxGraphicsBrush wxD2DRenderer::CreateBrush(const wxBrush& brush)
{
    wxFAIL_MSG("not implemented");
}


wxGraphicsBrush wxD2DRenderer::CreateLinearGradientBrush(
    wxDouble x1, wxDouble y1,
    wxDouble x2, wxDouble y2,
    const wxGraphicsGradientStops& stops)
{
    wxFAIL_MSG("not implemented");
}

wxGraphicsBrush wxD2DRenderer::CreateRadialGradientBrush(
    wxDouble xo, wxDouble yo,
    wxDouble xc, wxDouble yc,
    wxDouble radius,
    const wxGraphicsGradientStops& stops)
{
    wxFAIL_MSG("not implemented");
}

// create a native bitmap representation
wxGraphicsBitmap wxD2DRenderer::CreateBitmap(const wxBitmap& bitmap)
{
    wxFAIL_MSG("not implemented");
}

#if wxUSE_IMAGE
wxGraphicsBitmap wxD2DRenderer::CreateBitmapFromImage(const wxImage& image)
{
    wxFAIL_MSG("not implemented");
}

wxImage wxD2DRenderer::CreateImageFromBitmap(const wxGraphicsBitmap& bmp)
{
    wxFAIL_MSG("not implemented");
}
#endif

wxGraphicsFont wxD2DRenderer::CreateFont(const wxFont& font, const wxColour& col)
{
    wxFAIL_MSG("not implemented");
}

wxGraphicsFont wxD2DRenderer::CreateFont(
    double size, const wxString& facename, 
    int flags = wxFONTFLAG_DEFAULT, 
    const wxColour& col = *wxBLACK)
{
    wxFAIL_MSG("not implemented");
}

// create a graphics bitmap from a native bitmap
wxGraphicsBitmap wxD2DRenderer::CreateBitmapFromNativeBitmap(void* bitmap)
{
    wxFAIL_MSG("not implemented");
}

// create a sub-image from a native image representation
wxGraphicsBitmap wxD2DRenderer::CreateSubBitmap(const wxGraphicsBitmap& bitmap, wxDouble x, wxDouble y, wxDouble w, wxDouble h)
{
    wxFAIL_MSG("not implemented");
}

wxString wxD2DRenderer::GetName() const
{
    wxFAIL_MSG("not implemented");
}

void wxD2DRenderer::GetVersion(int* major, int* minor, int* micro)
{
    wxFAIL_MSG("not implemented");
}
