#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <memory>

#include <d2d1.h>
#include <d2d1helper.h>

#include "d2dhandler.h"

class D2DFrame : public wxFrame
{
public:
    D2DFrame(const wxString& title) : wxFrame(NULL, wxID_ANY, title)
    {
        SetSize(wxSize(800, 600));
        handler = std::unique_ptr<Direct2DHandler>(new Direct2DHandler((HWND)GetHandle()));

        Bind(wxEVT_PAINT, &D2DFrame::OnPaint, this);
        Bind(wxEVT_SIZE, &D2DFrame::OnResize, this);
    }

    void OnPaint(const wxPaintEvent& evt)
    {
        UNREFERENCED_PARAMETER(evt);
        handler->OnRender();
    }

    void OnResize(const wxSizeEvent& evt)
    {
        handler->OnResize(evt.GetSize().GetWidth(), evt.GetSize().GetHeight());
    }

private:
    std::unique_ptr<Direct2DHandler> handler;
};

class MyApp : public wxApp
{
public:
    virtual bool OnInit() wxOVERRIDE
    {
        if ( !wxApp::OnInit() )
            return false;

        D2DFrame *frame = new D2DFrame("Direct2D Sandbox");
        frame->Show(true);

        return true;
    }
};


IMPLEMENT_APP(MyApp)