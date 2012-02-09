#include <wx/wx.h>
#include <wx/wfstream.h>
#include <wx/datstrm.h>
#include <wx/tokenzr.h>
#include <math.h>

#include "../lk_absyn.h"
#include "../lk_env.h"
#include "../lk_eval.h"
#include "../lk_parse.h"
#include "../lk_lex.h"
#include "../lk_stdlib.h"

enum { ID_SOLVE = 8145 };

class LKSolve : public wxFrame
{
private:
	wxTextCtrl *m_input;
	wxTextCtrl *m_output;

public:
	LKSolve( )
		: wxFrame( NULL, wxID_ANY, "LKSolve", wxDefaultPosition, wxSize(800,650) )
	{
		wxBoxSizer *tools = new wxBoxSizer( wxHORIZONTAL );
		tools->Add( new wxButton(this, ID_SOLVE, "Solve" ) );
		tools->AddStretchSpacer();
		

		m_input = new wxTextCtrl( this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
		m_input->SetFont( wxFont(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Consolas") );
		m_output = new wxTextCtrl( this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
		m_output->SetFont( wxFont(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Consolas") );
		m_output->SetForegroundColour( *wxBLUE );

		wxBoxSizer *sz = new wxBoxSizer( wxVERTICAL );
		sz->Add( tools, 0, wxALL|wxEXPAND );
		sz->Add( m_input,  1, wxALL|wxEXPAND );
		sz->Add( m_output, 1, wxALL|wxEXPAND );
		SetSizer( sz );

		m_input->SetFocus();

		wxFFileInputStream file( "solver.txt" );
		if (file.IsOk())
			m_input->SetValue( wxDataInputStream(file).ReadString() );
	}

	void OnSolve( wxCommandEvent & )
	{
		m_output->Clear();
		wxArrayString eqns = wxStringTokenize(m_input->GetValue(), "\n");
		for (size_t i=0;i<eqns.Count();i++)
		{
			m_output->AppendText("\n");
			lk::input_string p( eqns[i] );
			lk::parser parser(p);
			lk::node_t *lhs = parser.additive();
			if ( !parser.match( lk::lexer::OP_ASSIGN) )
			{
				if (lhs) delete lhs;
				m_output->AppendText( wxString::Format("equation %d does not contain an assignment [%s]\n", i+1, (const char*)eqns[i].c_str()) );
				return;
			}


			lk::node_t *rhs = parser.additive();

			if ( rhs == 0 || lhs == 0
				|| parser.error_count() > 0
				|| parser.token() != lk::lexer::END )
			{
				if (lhs) delete lhs;
				if (rhs) delete rhs;
				m_output->AppendText( wxString::Format("equation %d does not parse correctly [%s]\n", i+1, (const char*)eqns[i].c_str()) );
				for ( int j=0;j<parser.error_count();j++ )
					m_output->AppendText("\t" + parser.error(j) + "\n");

				return;
			}

			lk::expr_t *expr = new lk::expr_t( i+1, lk::expr_t::MINUS,  lhs, rhs );

			wxString pretty;
			lk::pretty_print( pretty, expr, 0);
			m_output->AppendText( wxString::Format("\nEquation %d:\n", i+1) + pretty );
			delete expr;
		}
	}

	void OnCloseFrame( wxCloseEvent & )
	{
		wxFFileOutputStream file( "solver.txt" );
		if (file.IsOk())
			wxDataOutputStream(file).WriteString( m_input->GetValue() );

		Destroy();

	}

	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( LKSolve, wxFrame )
	EVT_BUTTON( ID_SOLVE, LKSolve::OnSolve )
	EVT_CLOSE( LKSolve::OnCloseFrame )
END_EVENT_TABLE()

void new_solver()
{
	LKSolve *l = new LKSolve;
	l->Show();
}