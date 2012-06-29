#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wx/wx.h>

#include <wx/config.h>
#include <wx/scrolbar.h>
#include <wx/print.h>
#include <wx/textfile.h>
#include <wx/printdlg.h>
#include <wx/accel.h>
#include <wx/image.h>
#include <wx/fs_zip.h>
#include <wx/html/htmlwin.h>
#include <wx/snglinst.h>
#include <wx/progdlg.h>
#include <wx/busyinfo.h>
#include <wx/dir.h>
#include <wx/stdpaths.h>
#include <wx/generic/helpext.h>
#include <wx/clipbrd.h>
#include <wx/aui/aui.h>
#include <wx/splitter.h>

#include <cml/codeedit.h>
#include <cml/afdialogs.h>
#include <cml/util.h>
#include <cml/array.h>

#include "lkui.h"

#include "../lk_absyn.h"
#include "../lk_env.h"
#include "../lk_eval.h"
#include "../lk_parse.h"
#include "../lk_lex.h"
#include "../lk_stdlib.h"

/* exported application global variables */

int LK_major_ver = 0;
int LK_minor_ver = 1;
int LK_micro_ver = 0;

LKFrame *app_frame = NULL;
wxArrayString app_args;
wxConfig *app_config = NULL;

void applog(const char *fmt, ...)
{
	static char buf[1024];
	va_list arglist;

	if (app_frame)
	{
		va_start( arglist, fmt );
#ifdef __WXMSW__
		_vsnprintf(buf,1023,fmt,arglist);
#else
		vsnprintf(buf,1023,fmt,arglist);
#endif
		va_end( arglist );
		
		app_frame->Post( buf );
	}
}

void applog(const wxString &s)
{
	if (app_frame)
		app_frame->Post( s );
}

/* ************************************************************
   ************ LK Application (set up handlers/config) ******
   ************************************************************ */

IMPLEMENT_APP(LKApp)

wxString LKApp::GetInstanceName()
{
	return m_inst_name;
}

LKApp::LKApp()
{
	/* configure the comlib log function */
}

bool LKApp::OnInit()
{
	// segmentation faults handled in LKApp::OnFatalException
	// works on MSW to check during development
#ifdef __WXMSW__
	if (!wxIsDebuggerRunning())
		wxHandleFatalExceptions();
#endif

	// generate a unique instance name
	int counter = 1;
	do
	{
		m_inst_name = "lkui-" + wxGetUserId() + wxString::Format("-inst%d", counter++);
		m_inst_checker = new wxSingleInstanceChecker( m_inst_name );
		if (!m_inst_checker->IsAnotherRunning())
			counter = 0;
		else
			delete m_inst_checker;
	}
	while ( counter > 0 );

	// now we've found a unique instance name
	// and the app's instance checker object has been created
	// we will use the instance name as the temporary work directory
	SetAppName( "lkui" );
	
	// accumulate all the command line args
	for (int i=0;i<argc;i++)
		app_args.Add(argv[i]);

	// set the current working directory to locate .pdb on crash
	wxSetWorkingDirectory( wxPathOnly(app_args[0]) );

	app_config = new wxConfig( "lkui", "WXAPPS" );
	
	// clear out the directory if there is stuff in it
	applog("lkui %d.%d.%d (WX): Starting up...\n", LK_major_ver, LK_minor_ver, LK_micro_ver);
	applog("Framework by Aron Dobos\n");
	applog("Command line args: %d\n", app_args.Count());
	for (int k=0;k<(int)app_args.Count();k++)
		applog("argv[%d]: '%s'\n", k, app_args[k].c_str());

	applog("NOW:  %s\n", wxNow().c_str());
	applog("INST: %s\n", m_inst_name.c_str());

	/* needed for the html help viewer */
	wxImage::AddHandler(new wxBMPHandler);
    wxImage::AddHandler(new wxPNGHandler);
    wxImage::AddHandler(new wxJPEGHandler);
	wxImage::AddHandler(new wxXPMHandler);

    wxFileSystem::AddHandler(new wxZipFSHandler);

	app_frame = new LKFrame;
	SetTopWindow(app_frame);
	
	
extern void new_solver();
	new_solver();
	return true;
}


int LKApp::OnExit()
{	
	if (app_config)
		delete app_config;
	
	if (m_inst_checker)
		delete m_inst_checker;
			
	return 0;
}

#ifdef __WXMSW__
/* simple class to get a stack trace */
#include <wx/stackwalk.h>

class StackDump : public wxStackWalker
{
public:
    StackDump(FILE *_fp = NULL) { fp = _fp; }
    const wxString& GetStackTrace() const { return m_stackTrace; }
protected:
    virtual void OnStackFrame(const wxStackFrame& frame)
    {
		wxString line = wxString::Format("[%d] ", (int)frame.GetLevel());
        wxString name = frame.GetName();
        line += wxString::Format("%p %s", frame.GetAddress(), name.c_str());

        if ( frame.HasSourceLocation() )
            line += " (" + frame.GetFileName() + "):" << frame.GetLine();

		m_stackTrace += line + "\n";

		if (fp)
		{
			fprintf(fp, "%s\n", line.c_str());
			fflush(fp);
		}
    }
private:
	FILE *fp;
    wxString m_stackTrace;
};

#endif

void LKApp::OnFatalException()
{
	applog("SEGMENTATION FAULT, GENERATING STACK TRACE\n");	

#ifdef __WXMSW__
	StackDump dump(0);
	dump.WalkFromException();

	wxString msgtext = TruncateLines( dump.GetStackTrace(), 9, "..." );

	wxMessageBox("Our sincerest apologies!\n\n"
		"An internal segmentation fault occurred at the program location listed below.\n"
		"LK will attempt to send a complete crash report by email to the development team.\n\n"
		+ msgtext, "Fatal Error", wxICON_ERROR|wxOK);

	wxString body;
	
	body += "SYSTEM INFORMATION:\n\n";
	body += "User Name: " + wxGetUserName() + "\n";
	body += "Home Dir: " + wxGetHomeDir() + "\n";
	body += "Email Address: " + wxGetEmailAddress() + "\n";;
	body += "Full Host Name: " + wxGetFullHostName() + "\n";
	body += "OS: " + wxGetOsDescription() + "\n";
	body += "Little Endian?: " + wxString::Format("%s",wxIsPlatformLittleEndian()?"Yes":"No") + "\n";
	body += "64-bit?: " + wxString::Format("%s",wxIsPlatform64Bit()?"Yes":"No") + "\n";
	body += "Free Memory: " + wxString::Format("%lg kB",wxGetFreeMemory().ToDouble()/1024) + "\n";
	body += "\n";

	body += "\nCRASH TRACE [" + wxNow() + "]\n\n" + dump.GetStackTrace();
	
	::AFTextMessageDialog( body, "Crash Report" );
	
#else
	::AFTextMessageDialog( "Error in program.", "Crash Report" );
#endif
	
}

/* ************************************************************
   ************ LK  Parent window class  ******************
   ************************************************************ */

enum{  
		ID_OUTPUT, ID_EDITOR, ID_EXECUTE
};

BEGIN_EVENT_TABLE(LKFrame, wxFrame)

	EVT_MENU( ID_EXECUTE, LKFrame::OnDocumentCommand )
	EVT_CLOSE( LKFrame::OnCloseFrame )

	EVT_CHAR_HOOK( LKFrame::OnCharHook )

END_EVENT_TABLE()

LKFrame::LKFrame()
 : wxFrame(NULL, wxID_ANY, "lk editor (F1 to save)", wxDefaultPosition, wxSize(800,600))
{
	SetIcon( wxIcon("appicon") );

	
	wxSplitterWindow *split_win = new wxSplitterWindow( this, wxID_ANY,
		wxPoint(0,0), wxSize(800,700), wxSP_LIVE_UPDATE|wxBORDER_NONE );

	m_codeEdit = new CodeEdit( split_win, ID_EDITOR );
	m_codeEdit->ApplyLKStyling();
	
	m_txtOutput = new wxTextCtrl(split_win, ID_OUTPUT, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxTE_READONLY | wxTE_MULTILINE | wxHSCROLL | wxTE_DONTWRAP);
	m_txtOutput->SetFont( wxFont(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "courier") );
	m_txtOutput->SetForegroundColour( *wxBLUE );
		
	split_win->SplitHorizontally( m_codeEdit, m_txtOutput, -380 );
	split_win->SetSashGravity( 1 );

	/************ SHOW THE APPLICATION *************/
	
	this->Show();
	this->SetClientSize( 750, 600);	


	// restore window position

	bool b_maximize = false;
	int f_x,f_y,f_width,f_height;
	app_config->Read("FrameX", &f_x, -1);
	app_config->Read("FrameY", &f_y, -1);
	app_config->Read("FrameWidth", &f_width, -1);
	app_config->Read("FrameHeight", &f_height, -1);
	app_config->Read("FrameMaximized", &b_maximize, false);

	LoadCode();

	if (b_maximize)
	{
		this->Maximize();
	}
	else
	{
		if (f_width > 100 && f_height > 100)
			this->SetClientSize(f_width, f_height);

		if (f_x > 0 && f_y > 0)
			this->SetPosition(wxPoint(f_x,f_y));
	}

	
	wxAcceleratorEntry entries[7];
	entries[0].Set( wxACCEL_NORMAL, WXK_F5, ID_EXECUTE );
	/*entries[1].Set( wxACCEL_CTRL,   's',  wxID_SAVE );
	entries[2].Set( wxACCEL_CTRL,   'o',  wxID_OPEN );
	entries[3].Set( wxACCEL_NORMAL, WXK_F2, wxID_PREFERENCES );
	entries[4].Set( wxACCEL_NORMAL, WXK_F3, ID_SHOW_STATS );
	entries[5].Set( wxACCEL_NORMAL, WXK_F4, ID_ADD_VARIABLE );
	entries[6].Set( wxACCEL_NORMAL, WXK_F5, ID_START );*/
	SetAcceleratorTable( wxAcceleratorTable(1,entries) );
}

void LKFrame::OnCloseFrame( wxCloseEvent &evt )
{	
	/* save window position */
	bool b_maximize = this->IsMaximized();
	int f_x,f_y,f_width,f_height;

	this->GetPosition(&f_x,&f_y);
	this->GetClientSize(&f_width, &f_height);
	
	app_config->Write("FrameX", f_x);
	app_config->Write("FrameY", f_y);
	app_config->Write("FrameWidth", f_width);
	app_config->Write("FrameHeight", f_height);
	app_config->Write("FrameMaximized", b_maximize);

	Destroy();
}

void LKFrame::OnCharHook( wxKeyEvent &evt )
{
	if (evt.GetKeyCode() == WXK_ESCAPE)
		Close();
	else if (evt.GetKeyCode() == WXK_F1)
		SaveCode();
	else
		evt.Skip();
}


void LKFrame::LoadCode()
{
	m_codeEdit->SetText( ReadTextFile(wxPathOnly(app_args[0]) + "/lkCode.txt") );
}

void LKFrame::SaveCode()
{
	wxString tf( wxPathOnly(app_args[0]) + "/lkCode.txt" );
	FILE *fp = fopen((const char*)tf.c_str(), "w");
	if (fp)
	{
		fputs( (const char*)m_codeEdit->GetText().c_str(), fp );
		fclose(fp);
		m_txtOutput->AppendText("Wrote: " + tf + "\n");
	}
	else wxMessageBox("Could not write: " + tf);
}

void fcall_output( lk::invoke_t &cxt )
{
	LK_DOC("out", "Output data to the console.", "(...):none");
	for (size_t i=0;i<cxt.arg_count();i++)
		app_frame->Post( (const char*)cxt.arg(i).as_string().c_str());
}

void fcall_input(  lk::invoke_t &cxt )
{
	LK_DOC("in", "Input text from the user.", "(none):string");
	cxt.result().assign( std::string((const char*)wxGetTextFromUser("Standard Input:").c_str()) );	
}

void LKFrame::OnDocumentCommand(wxCommandEvent &evt)
{
	switch(evt.GetId())
	{
	case ID_EXECUTE:
		{
			m_txtOutput->Clear();
			
			SaveCode();
	
			m_txtOutput->AppendText( "Start: " + wxNow() + "\n" );

			lk::input_string p( (const char*) m_codeEdit->GetText().c_str() );
			lk::parser parse( p );
		
			lk::node_t *tree = parse.script();
		
			applog("parsed: %d nodes allocated, next: %s\n", lk::_node_alloc, lk::lexer::tokstr(parse.token()) );
		
		
			if ( parse.error_count() != 0 
				|| parse.token() != lk::lexer::END)
			{
				applog("parsing did not reach end of input\n");
			}
			else
			{			
				lk_string str;
				lk::pretty_print( str, tree, 0 );
				Post(str.c_str());
				Post("\n\n");
				

				lk::env_t env;
				env.register_func( fcall_input );
				env.register_func( fcall_output );

				env.register_funcs( lk::stdlib_basic() );
				env.register_funcs( lk::stdlib_string() );
				env.register_funcs( lk::stdlib_math() );
				env.register_funcs( lk::stdlib_wxui() );


				lk::vardata_t result;
				unsigned int ctl_id = lk::CTL_NONE;
				wxStopWatch sw;
				std::vector<lk_string> errors;
				if ( lk::eval( tree, &env, errors, result, 0, ctl_id, 0, 0 ) )
				{
					long time = sw.Time();
					applog("elapsed time: %ld msec\n", time);

					lk_string key;
					lk::vardata_t *value;
					bool has_more = env.first( key, value );
					while( has_more )
					{
						applog("env{%s}=%s\n", key, value->as_string().c_str());
						has_more = env.next( key, value );
					}

				}
				else
				{
					Post("eval fail\n");
					for (size_t i=0;i<errors.size();i++)
						Post( errors[i].c_str() );
				}
			}
			
			for (int i=0; i<parse.error_count(); i++ )
				Post( parse.error(i)  + "\n" );
		
			delete tree;
		
			if (lk::_node_alloc != 0)
				applog("memory leak! %d nodes remain\n", lk::_node_alloc);
		}
		break;
	}
}
