#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/html/htmlwin.h>
#include <wx/tokenzr.h>
#include <wx/stc/stc.h>
#include <wx/splitter.h>

#include <lk_absyn.h>
#include <lk_env.h>
#include <lk_eval.h>
#include <lk_lex.h>
#include <lk_parse.h>
#include <lk_stdlib.h>

class OutputWindow;
static OutputWindow *__g_outputWindow = 0;
static bool __g_scriptRunning = false;


class OutputWindow : public wxFrame
{
private:
	wxTextCtrl *m_text;
public:
	OutputWindow() : wxFrame( 0, wxID_ANY, "Output Window", wxDefaultPosition, wxSize(900, 200) )
	{
#ifdef __WXMSW__
		SetIcon( wxICON( appicon ) );
#endif
		m_text = new wxTextCtrl( this,  wxID_ANY, "Ready\n", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
	}
	virtual ~OutputWindow() { __g_outputWindow = 0; }
	void CloseEventHandler( wxCloseEvent & ) { Hide(); }
	void Append( const wxString &text ) { m_text->AppendText( text ); }
	void Clear() { m_text->Clear(); }
	DECLARE_EVENT_TABLE()
};
BEGIN_EVENT_TABLE(OutputWindow, wxFrame)
	EVT_CLOSE( OutputWindow::CloseEventHandler )
END_EVENT_TABLE()

void ShowOutputWindow()
{
	if (__g_outputWindow == 0)
		__g_outputWindow = new OutputWindow();

	if ( !__g_outputWindow->IsShown())
	{
		__g_outputWindow->Show();
		__g_outputWindow->Raise();
	}
}

void HideOutputWindow()
{
	if (__g_outputWindow) __g_outputWindow->Hide();
}

void Output( const wxString &text )
{
	if (__g_outputWindow) __g_outputWindow->Append( text );
}


void Output( const char *fmt, ... )
{
	static char buf[2048];
	va_list ap;
	va_start(ap, fmt);
#if defined(_MSC_VER)||defined(_WIN32)
	_vsnprintf(buf, 2046, fmt, ap);
#else
	vsnprintf(buf, 2046, fmt, ap);
#endif
	va_end(ap);
	Output( wxString(buf) );	
}

void ClearOutput()
{
	if (__g_outputWindow) __g_outputWindow->Clear();
}

void DestroyOutputWindow()
{
	if (__g_outputWindow) __g_outputWindow->Destroy();
}


void fcall_out( lk::invoke_t &cxt )
{
	LK_DOC("out", "Output data to the console.", "(...):none");
	
	for (size_t i=0;i<cxt.arg_count();i++)
		Output( cxt.arg(i).as_string() );
}

void fcall_outln( lk::invoke_t &cxt )
{
	LK_DOC("outln", "Output data to the console with a newline.", "(...):none");
	
	for (size_t i=0;i<cxt.arg_count();i++)
		Output( cxt.arg(i).as_string()  + "\n" ); 
}

void fcall_in(  lk::invoke_t &cxt )
{
	LK_DOC("in", "Input text from the user.", "(none):string");
	cxt.result().assign( wxGetTextFromUser("Standard Input:") );	
}


lk::fcall_t* my_funcs()
{
	static const lk::fcall_t vec[] = {
		fcall_in,
		fcall_out,
		fcall_outln,
		0 };
		
	return (lk::fcall_t*)vec;
}

enum { ID_CODEEDITOR = wxID_HIGHEST+1, ID_RUN, ID_COMPILE, ID_PARSETIMER, ID_OUTPUT_WINDOW };

static int __ndoc = 0;

//forward
bool eval_callback( int, void* );

class EditorWindow : public wxFrame
{
private:
	static int __s_numEditorWindows;
	lk::env_t *m_env;
	wxTextCtrl *m_asmOutput;
	wxStyledTextCtrl *m_editor;
	wxStaticText *m_statusLabel;
	wxString m_fileName;
	wxButton *m_stopButton;
	wxGauge *m_progressBar;
	wxTextCtrl *m_txtProgress;
	wxTextCtrl *m_compileErrors;
	bool m_stopScriptFlag;
	wxTimer m_timer;
public:
	EditorWindow()
		: wxFrame( 0, wxID_ANY, wxString::Format("untitled %d",++__ndoc), wxDefaultPosition, wxSize(700,700) ),
		m_timer( this, ID_PARSETIMER )
	{
		__s_numEditorWindows++;
		m_stopScriptFlag = false;
#ifdef __WXMSW__
		SetIcon( wxICON( appicon ) );
#endif

		
		SetBackgroundColour( *wxWHITE );
		
		m_progressBar = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_SMOOTH );
		m_txtProgress = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(270, 21), wxTE_READONLY );
		m_txtProgress->SetBackgroundColour(*wxBLACK);
		m_txtProgress->SetForegroundColour(*wxGREEN);

		wxBoxSizer *sztools = new wxBoxSizer( wxHORIZONTAL );
		sztools->Add( m_stopButton = new wxButton( this, wxID_STOP, "Stop" ), 0, wxALL|wxEXPAND, 2 );
		sztools->Add( new wxButton( this, ID_RUN, "Run" ), 0, wxALL|wxEXPAND, 2  );
		sztools->Add( new wxButton( this, ID_COMPILE, "Compile" ), 0, wxALL|wxEXPAND, 2 );
		sztools->Add( m_progressBar, 0, wxALL|wxEXPAND, 2  );
		sztools->Add( m_txtProgress, 0, wxALL|wxEXPAND, 2  );
		sztools->Add( m_statusLabel = new wxStaticText(this,wxID_ANY,wxEmptyString), 0, wxALL|wxALIGN_CENTER_VERTICAL, 2 );

		m_stopButton->SetForegroundColour( *wxRED );

		m_stopButton->Hide();
		m_progressBar->Hide();
		m_txtProgress->Hide();
			
		wxMenu *file_menu = new wxMenu;
		file_menu->Append( wxID_NEW, "New\tCtrl-N" );
		file_menu->AppendSeparator();
		file_menu->Append( wxID_OPEN, "Open\tCtrl-O");
		file_menu->Append( wxID_SAVE, "Save\tCtrl-S" );
		file_menu->Append( wxID_SAVEAS, "Save as...");
		file_menu->AppendSeparator();
		file_menu->Append( wxID_CLOSE, "Close\tCtrl-W");
		file_menu->AppendSeparator();
		file_menu->Append( wxID_EXIT, "Exit" );

		wxMenu *edit_menu = new wxMenu;
		edit_menu->Append( wxID_UNDO, "Undo\tCtrl-Z" );
		edit_menu->Append( wxID_REDO, "Redo\tCtrl-Y" );
		edit_menu->AppendSeparator();
		edit_menu->Append( wxID_CUT, "Cut\tCtrl-X" );
		edit_menu->Append( wxID_COPY, "Copy\tCtrl-C" );
		edit_menu->Append( wxID_PASTE, "Paste\tCtrl-V" );
		edit_menu->AppendSeparator();
		edit_menu->Append( wxID_SELECTALL, "Select all\tCtrl-A" );
		edit_menu->AppendSeparator();
		edit_menu->Append( wxID_FIND, "Find...\tCtrl-F" );
		edit_menu->Append( wxID_FORWARD, "Find next\tCtrl-G" );

		wxMenu *act_menu = new wxMenu;
		act_menu->Append( ID_RUN, "Run\tF5" );
		act_menu->Append( ID_COMPILE, "Compile\tF7" );
		act_menu->AppendSeparator();
		act_menu->Append( ID_OUTPUT_WINDOW, "Show output window\tF6" );
		
		wxMenu *help_menu = new wxMenu;
		help_menu->Append( wxID_HELP, "Script reference\tF1");

		wxMenuBar *menu_bar = new wxMenuBar;
		menu_bar->Append( file_menu, "File" );
		menu_bar->Append( edit_menu, "Edit" );
		menu_bar->Append( act_menu, "Actions" );
		menu_bar->Append( help_menu, "Help" );
		
		SetMenuBar( menu_bar );
		
		m_env = new lk::env_t;
		
		m_env->register_funcs( my_funcs() );
		m_env->register_funcs( lk::stdlib_basic() );
		m_env->register_funcs( lk::stdlib_string() );
		m_env->register_funcs( lk::stdlib_math() );
		m_env->register_funcs( lk::stdlib_wxui() );

		/*
		StringMap tips;
		std::vector<lk_string> list = m_env->list_funcs();
		wxString funclist;
		for (size_t i=0;i<list.size();i++)
		{
			lk::doc_t d;
			if (lk::doc_t::info( m_env->lookup_func( list[i] ), d ))
			{
				wxString data = ::LimitColumnWidth( d.func_name + d.sig1 + "\n\n" + d.desc1, 90 );
				if (d.has_2) data += ::LimitColumnWidth( "\n\n" + d.func_name + d.sig2 + "\n\n" + d.desc2, 90 );
				if (d.has_3) data += ::LimitColumnWidth( "\n\n" + d.func_name + d.sig3 + "\n\n" + d.desc3, 90 );

				tips[ d.func_name ] = data;			
				funclist += d.func_name + " ";
			}
		}
		*/
		
		wxSplitterWindow *splitter = new wxSplitterWindow( this, wxID_ANY );

		m_asmOutput = new wxTextCtrl( splitter, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxTE_DONTWRAP );
		m_asmOutput->SetFont( wxFont( 11, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Consolas" ) );
		m_asmOutput->SetForegroundColour( *wxBLUE );

		m_editor = new wxStyledTextCtrl(splitter, ID_CODEEDITOR );

		m_compileErrors = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
		
		/*
		m_editor->ApplyLKStyling();
		m_editor->EnableCallTips(true);
		m_editor->SetCallTipData('(',')', false, tips);
		m_editor->StyleSetForeground( wxSTC_C_WORD2, wxColour(0,128,192) );
		m_editor->SetKeyWords( 1, funclist );
		*/

		wxBoxSizer *szmain = new wxBoxSizer( wxVERTICAL );
		szmain->Add( sztools, 0, wxALL|wxEXPAND, 0 );
		szmain->Add( new wxStaticLine( this ), 0, wxALL|wxEXPAND, 0 );
		szmain->Add( splitter, 6, wxALL|wxEXPAND, 0 );
		szmain->Add( m_compileErrors, 1, wxALL|wxEXPAND, 0 );
		SetSizer( szmain );

		splitter->SplitVertically( m_asmOutput, m_editor, 250 );
		
		m_editor->SetFocus();
	}

	virtual ~EditorWindow()
	{
		delete m_env;
	}

	wxString GetFileName() { return m_fileName; }

	void OnCommand( wxCommandEvent &evt )
	{
		switch( evt.GetId() )
		{
		case wxID_NEW:
			(new EditorWindow())->Show();
			break;
		case wxID_OPEN:
			{
				wxFileDialog dlg(this, "Open", wxEmptyString, wxEmptyString,
									  "LK Script Files (*.lk)|*.lk",
									  wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR);

				if (dlg.ShowModal() == wxID_OK)
				{
					wxWindowList wl = ::wxTopLevelWindows;
					for (size_t i=0;i<wl.size();i++)
					{
						if (EditorWindow *ew = dynamic_cast<EditorWindow*>( wl[i] ))
						{
							if ( dlg.GetPath() == ew->GetFileName() )
							{
								ew->Raise();
								return;
							}
						}
					}

					EditorWindow *target = this;
					if (m_editor->GetModify() || !m_fileName.IsEmpty())
					{
						target = new EditorWindow;
						target->Show();
					}
					
					if (!target->Load( dlg.GetPath() ))
						wxMessageBox("Could not load file:\n\n" + dlg.GetPath());
				}
			}
			break;
		case wxID_SAVE:
			Save();
			break;
		case wxID_SAVEAS:
			SaveAs();
			break;
		case wxID_CLOSE:
			Close();
			break;
		case wxID_UNDO: m_editor->Undo(); break;
		case wxID_REDO: m_editor->Redo(); break;
		case wxID_CUT: m_editor->Cut(); break;
		case wxID_COPY: m_editor->Copy(); break;
		case wxID_PASTE: m_editor->Paste(); break;
		case wxID_SELECTALL: m_editor->SelectAll(); break;
		/*
		case wxID_FIND: m_editor->ShowFindDialog(); break;
		case wxID_FORWARD: m_editor->FindNext(); break;
		*/
		case wxID_HELP:
			{
				wxFrame *frm = new wxFrame( this, wxID_ANY, "Scripting Reference", wxDefaultPosition, wxSize(800, 700) );
				frm->SetIcon(wxIcon("appicon"));
				wxHtmlWindow *html = new wxHtmlWindow( frm, wxID_ANY, wxDefaultPosition, wxDefaultSize );

				wxString data;
				data += lk::html_doc( "Input/Output Functions", my_funcs() );
				data += lk::html_doc( "Basic Operations", lk::stdlib_basic() );
				data += lk::html_doc( "String Functions", lk::stdlib_string() );
				data += lk::html_doc( "Math Functions", lk::stdlib_math() );
				data += lk::html_doc( "User Interface Functions", lk::stdlib_wxui() );		
				html->SetPage( data );
				frm->Show();
			}
			break;
		case wxID_EXIT:
			{
				wxWindowList wl = ::wxTopLevelWindows;
				for (size_t i=0;i<wl.size();i++)
					if ( dynamic_cast<EditorWindow*>( wl[i] ) != 0 
						&& wl[i] != this )
						wl[i]->Close();

				Close();
			}
			break;
		case ID_RUN:
			Exec();
			break;
		case ID_COMPILE:
			Compile();
			break;
		case ID_OUTPUT_WINDOW:
			ShowOutputWindow();
			break;
		case wxID_STOP:
			m_stopScriptFlag = true;
			break;
		}
	}

	bool IsStopFlagSet() { return m_stopScriptFlag; }
	
	
	bool Save()
	{
		if ( m_fileName.IsEmpty() )
			return SaveAs();
		else
			return Write( m_fileName );
	}

	bool SaveAs()
	{
		wxFileDialog dlg( this, "Save as...",
			wxPathOnly(m_fileName),
			wxFileNameFromPath(m_fileName),
			"LK Script Files (*.lk)|*.lk", wxFD_SAVE|wxFD_OVERWRITE_PROMPT );
		if (dlg.ShowModal() == wxID_OK)
			return Write( dlg.GetPath() );
		else
			return false;
	}

	
	bool CloseDoc()
	{
		if (__g_scriptRunning)
		{
			if (wxYES==wxMessageBox("A script is running. Cancel it?", "Query", wxYES_NO))
				m_stopScriptFlag = true;

			return false;
		}

		if (m_editor->GetModify())
		{
			Raise();
			wxString id = m_fileName.IsEmpty() ? this->GetTitle() : m_fileName;
			int result = wxMessageBox("Script modified. Save it?\n\n" + id, "Query", wxYES_NO|wxCANCEL);
			if ( result == wxCANCEL 
				|| (result == wxYES && !Save()) )
				return false;
		}

		m_editor->SetText("");
		m_editor->EmptyUndoBuffer();
		m_editor->SetSavePoint();
		m_fileName = "";
		SetTitle( wxString::Format("untitled %d",++__ndoc) );
		return true;
	}

	bool Write( const wxString &file )
	{
		if ( ((wxStyledTextCtrl*)m_editor)->SaveFile( file ) )
		{
			m_fileName = file;
			SetTitle( wxFileNameFromPath(m_fileName) );
			m_statusLabel->SetLabel( m_fileName );
			return true;
		}
		else return false;
	}

	bool Load( const wxString &file )
	{
		FILE *fp = fopen(file.c_str(), "r");
		if ( fp )
		{
			wxString str;
			char buf[1024];
			while(fgets( buf, 1023, fp ) != 0)
				str += buf;

			fclose(fp);
			m_editor->SetText(str);
			m_editor->EmptyUndoBuffer();
			m_editor->SetSavePoint();
			m_fileName = file;
			SetTitle( wxFileNameFromPath(m_fileName) );
			m_statusLabel->SetLabel( m_fileName );
			return true;
		}
		else return false;
	}
	
	void Compile()
	{
		wxString script = m_editor->GetText();
		m_compileErrors->Clear();
		
	}


	void Exec()
	{
		if (__g_scriptRunning)
		{
			wxMessageBox("A script is already running.");
			return;
		}
		
		m_env->clear_objs();
		m_env->clear_vars();

		__g_scriptRunning = true;
		m_stopScriptFlag = false;

		m_stopButton->Show();
		m_progressBar->Show();
		m_txtProgress->Show();

		Layout();

		wxString script = m_editor->GetText();
		if (!m_fileName.IsEmpty())
		{
			wxString fn = m_fileName + "~";
			FILE *fp = fopen( (const char*)fn.c_str(), "w" );
			if (fp)
			{
				for (size_t i=0;i<script.Len();i++)
					fputc( (char)script[i], fp );
				fclose(fp);
			}
		}
		
		ShowOutputWindow();
		ClearOutput();
		Output("Start: " + wxNow()  + "\n");

		lk::input_string p( script );
		lk::parser parse( p );
	
		lk::node_t *tree = parse.script();
				
		wxYield();

		if ( parse.error_count() != 0 
			|| parse.token() != lk::lexer::END)
		{
			Output("parsing did not reach end of input\n");
		}
		else
		{
			m_env->clear_vars();
			m_env->clear_objs();

			lk::vardata_t result;
			unsigned int ctl_id = lk::CTL_NONE;
			wxStopWatch sw;
			std::vector<lk_string> errors;
			if ( lk::eval( tree, m_env, errors, result, 0, ctl_id, eval_callback, this ) )
			{
				long time = sw.Time();
				Output("elapsed time: %ld msec\n", time);

				/*
				lk_string key;
				lk::vardata_t *value;
				bool has_more = env.first( key, value );
				while( has_more )
				{
					applog("env{%s}=%s\n", key, value->as_string().c_str());
					has_more = env.next( key, value );
				}
				*/

			}
			else
			{
				Output("eval fail\n");
				for (size_t i=0;i<errors.size();i++)
					Output( wxString(errors[i].c_str()) + "\n");

			}
		}
			
		int i=0;
		while ( i < parse.error_count() )
			Output( parse.error(i++) );

		if (tree) delete tree;
		
		m_stopButton->Hide();
		m_progressBar->Hide();
		m_txtProgress->Hide();
		
		Layout();
		
		m_env->clear_objs();
		m_env->clear_vars();

		__g_scriptRunning = false;
		m_stopScriptFlag = false;

	}

	void Progress(const wxString &text, float percent)
	{
		m_txtProgress->SetValue(text);
		m_txtProgress->Update();
		m_progressBar->SetValue( (int)percent );
		m_progressBar->Update();
	}


	void CloseEventHandler( wxCloseEvent &evt )
	{	
		if ( !CloseDoc() )
		{
			evt.Veto();
			return;
		}

		Destroy();
		if (--__s_numEditorWindows == 0)
		{
			DestroyOutputWindow();
			/*
			wxWindowList wl = ::wxTopLevelWindows;
			for (size_t i=0;i<wl.size();i++)
				if (dynamic_cast<PlotWin*>( wl[i] ) != 0)
					wl[i]->Close();
					*/
		}
	}

	void OnParseTimer( wxTimerEvent & )
	{
		Compile();
	}

	void OnEditorModified( wxStyledTextEvent &evt )
	{
		if ( evt.GetModificationType() & wxSTC_MOD_INSERTTEXT 
			|| evt.GetModificationType() & wxSTC_MOD_DELETETEXT )
		{
			m_timer.Stop();
			m_timer.Start( 500, true );
			evt.Skip();
		}
	}



	DECLARE_EVENT_TABLE()
};
int EditorWindow::__s_numEditorWindows = 0;

BEGIN_EVENT_TABLE( EditorWindow, wxFrame )
	EVT_MENU( wxID_NEW, EditorWindow::OnCommand )
	EVT_MENU( wxID_OPEN, EditorWindow::OnCommand )
	EVT_MENU( wxID_SAVE, EditorWindow::OnCommand )
	EVT_MENU( wxID_SAVEAS, EditorWindow::OnCommand )
	EVT_MENU( wxID_HELP, EditorWindow::OnCommand )
	EVT_MENU( wxID_CLOSE, EditorWindow::OnCommand )
	EVT_MENU( wxID_EXIT, EditorWindow::OnCommand )

	EVT_MENU( wxID_UNDO, EditorWindow::OnCommand )
	EVT_MENU( wxID_REDO, EditorWindow::OnCommand )
	EVT_MENU( wxID_CUT, EditorWindow::OnCommand )
	EVT_MENU( wxID_COPY, EditorWindow::OnCommand )
	EVT_MENU( wxID_PASTE, EditorWindow::OnCommand )
	EVT_MENU( wxID_SELECTALL, EditorWindow::OnCommand )
	EVT_MENU( wxID_FIND, EditorWindow::OnCommand )
	EVT_MENU( wxID_FORWARD, EditorWindow::OnCommand )

	
	EVT_MENU( ID_RUN, EditorWindow::OnCommand )
	EVT_MENU( ID_COMPILE, EditorWindow::OnCommand )
	EVT_MENU( ID_OUTPUT_WINDOW, EditorWindow::OnCommand )

	EVT_BUTTON( wxID_STOP, EditorWindow::OnCommand )
	EVT_BUTTON( ID_RUN, EditorWindow::OnCommand )
	EVT_BUTTON( ID_COMPILE, EditorWindow::OnCommand )
	EVT_BUTTON( wxID_HELP, EditorWindow::OnCommand )

	EVT_STC_MODIFIED( ID_CODEEDITOR, EditorWindow::OnEditorModified )
	EVT_TIMER( ID_PARSETIMER, EditorWindow::OnParseTimer )

	EVT_CLOSE( EditorWindow::CloseEventHandler )
END_EVENT_TABLE()


class REToolApp : public wxApp
{
public:
	virtual bool OnInit()
	{
		::wxInitAllImageHandlers();
		(new EditorWindow())->Show();
		return true;
	}
};

IMPLEMENT_APP( REToolApp );

bool eval_callback( int, void *cbdata )
{
	wxGetApp().Yield(true);
	return !((EditorWindow*)cbdata)->IsStopFlagSet();
}
