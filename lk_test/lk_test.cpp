#include <wx/wx.h>
#include <wx/frame.h>
#include <wx/stc/stc.h>
#include <wx/webview.h>

#include <wex/lkscript.h>

class MyApp : public wxApp
{
public:
	bool OnInit()
	{
		wxInitAllImageHandlers();

		wxFrame *frame1 = new wxFrame( 0, wxID_ANY, wxT("Editor"), wxDefaultPosition, wxSize(700,700) );
		new wxLKScriptCtrl( frame1, wxID_ANY );
		frame1->Show();
		
		return true;
	}
};

IMPLEMENT_APP( MyApp );
