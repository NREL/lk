#include <wx/wx.h>
#include <wx/wfstream.h>
#include <wx/datstrm.h>
#include <wx/tokenzr.h>
#include <math.h>

#include <sstream>

#include <algorithm>
#include <limits>
#include <numeric>

#include "../lk_absyn.h"
#include "../lk_env.h"
#include "../lk_eval.h"
#include "../lk_parse.h"
#include "../lk_lex.h"
#include "../lk_stdlib.h"

#include "gamma.h"
#include "jacobian.h"
#include "newton.h"
#include "search.h"
#include "lu.h"
#include "6parmod.h"

enum { ID_SOLVE = 8145 };

#define NMAX 10

namespace lk {
	typedef unordered_map< lk_string, double, lk_string_hash, lk_string_equal > valtab_t;
	
	class LKEXPORT funccall_t : public node_t
	{
	public:
		lk_string name;
		std::vector< lk::node_t* > args;
		funccall_t(int line, const lk_string &n) : node_t(line), name(n) {  }
		virtual ~funccall_t() { for (size_t i=0;i<args.size();i++) delete args[i]; }
	};

	
	double eval_eqn( lk::node_t *n, valtab_t &t )
	{
		if ( !n ) return std::numeric_limits<double>::quiet_NaN();
		if ( lk::expr_t *e = dynamic_cast<lk::expr_t*>( n ) )
		{
			double a = eval_eqn( e->left, t );
			double b = eval_eqn( e->right, t );
			switch( e->oper )
			{
			case lk::expr_t::PLUS:
				return a + b;
			case lk::expr_t::MINUS:
				return a - b;
			case lk::expr_t::MULT:
				return a * b;
			case lk::expr_t::DIV:
				{
					if ( b == 0 ) return std::numeric_limits<double>::quiet_NaN();
					else return a / b;
				}
			case lk::expr_t::EXP:
				return pow( a, b );
			case lk::expr_t::NEG:
				return -a;
			}

		}
		else if ( lk::iden_t *i = dynamic_cast<lk::iden_t*>( n ))
		{
			return t[i->name];
		}
		else if ( lk::constant_t *c = dynamic_cast<lk::constant_t*>( n ))
		{
			return c->value;
		}
		else if ( lk::funccall_t *f = dynamic_cast<lk::funccall_t*>( n ))
		{

			if (f->name == "sin" && f->args.size() == 1 )
				return ::sin( eval_eqn( f->args[0], t ) );			
			else if (f->name == "cos" && f->args.size() == 1 )
				return ::cos( eval_eqn( f->args[0], t ) );			
			else if (f->name == "tan" && f->args.size() == 1 )
				return ::tan( eval_eqn( f->args[0], t ) );
		}

		return std::numeric_limits<double>::quiet_NaN();

	}

	bool nsolver_callback( int iter, double x[], double resid[], const int n, void *data)
	{
		wxTextCtrl *t = (wxTextCtrl*)data;
		wxString text;
		text.Printf("\niter=%d\n", iter);
		for (int i=0;i<n;i++)
		{
			text += wxString::Format("x[%d]=%lg     resid[%d]=%lg\n", i, x[i], i, resid[i] );
		}
		t->AppendText( text );
		return true;
	}

	class nsolver
	{
	private:
		std::vector< lk::node_t* > &m_eqns;
		std::vector< lk_string > &m_vars;
		valtab_t &m_tab;
	public:

		nsolver( std::vector< lk::node_t* > &e, std::vector< lk_string > &v, valtab_t &t )
			: m_eqns(e), m_vars(v), m_tab(t)
		{
		}
		
		void operator() ( const double x[NMAX], double f[NMAX] )
		{
			size_t i;
			for( i=0; i < m_vars.size()  && i < NMAX; i++)
				m_tab[ m_vars[i] ] = x[i];

			// solve each eqn to get f[i];
			for ( size_t i=0;i< m_eqns.size() && i < NMAX;i++)
				f[i] = eval_eqn( m_eqns[i], m_tab );
		}

		bool exec( int max_iter, double tol )
		{
			double x[NMAX], resid[NMAX];
			for (int i=0;i<NMAX;i++)
				x[i] = resid[i] = 0.0;

			for (size_t i=0;i<m_vars.size() && i < NMAX;i++)
				x[i] = m_tab[ m_vars[i] ]; // initial values


			wxFrame *frame = new wxFrame(0, 0, "Log" );
			wxTextCtrl *log = new wxTextCtrl(frame, 0, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
			frame->Show();

			bool check = false;
			int niter = newton<double, nsolver, NMAX>( x, resid, check, *this, 
					max_iter, tol, tol, 0.7,
					nsolver_callback, log);

			if ( niter < 0 || check ) return false;
			
			for( size_t i=0; i < m_vars.size()  && i < NMAX; i++)
				m_tab[ m_vars[i] ] = x[i];

			return true;
		}
	};

	class eqnsolve
	{
	public:
		valtab_t m_values;
		bool m_haltFlag;
		lexer lex;				
		int m_tokType;	
		std::vector<lk_string> m_errorList;
		std::vector< lk::node_t* > m_eqnList;
		
		int line() { return lex.line(); }
		int error_count() { return m_errorList.size(); }
		lk_string error(int idx);
				
		void skip() { m_tokType = lex.next(); }

		void error( const char *fmt, ... )
		{
			char buf[512];
			va_list list;
			va_start( list, fmt );	
	
			#if defined(_WIN32)&&defined(_MSC_VER)
				_vsnprintf( buf, 510, fmt, list );
			#else
				vsnprintf( buf, 510, fmt, list );
			#endif
	
			m_errorList.push_back(lk_string(buf));
			va_end( list );
		}

		bool match( const char *s )
		{	
			if ( lex.text() != s )
			{
				error("%d: expected '%s'", line(), s);
				return false;
			}	
			skip();
			return true;
		}

		bool match(int t)
		{
			if ( m_tokType != t)
			{
				error("%d: expected '%s'", line(), lk::lexer::tokstr(t));
				return false;
			}	
			skip();
			return true;
		}
		
		int token()
		{
			return m_tokType;
		}

		bool token(int t)
		{
			return (m_tokType==t);
		}

	public:
		eqnsolve( input_base &input )
			: lex( input )
		{
			m_haltFlag = false;
			m_tokType = lex.next();

		}

		void set( const lk_string &name, double val )
		{
			m_values[name] = val;
		}

		double get( const lk_string &name )
		{
			valtab_t::iterator it = m_values.find( name );
			if ( it != m_values.end() )
				return (*it).second;
			else
				return std::numeric_limits<double>::quiet_NaN();
		}

		std::vector< lk_string > errors() { return m_errorList; }

		lk_string errortext()
		{
			lk_string t;
			for (size_t i=0;i<m_errorList.size();i++)
				t += m_errorList[i] + "\n";
			return t;
		}

		bool parse()
		{
			m_errorList.clear();
			for (size_t i=0;i<m_eqnList.size();i++)
				delete m_eqnList[i];
			m_eqnList.clear();

			while ( token() != lk::lexer::INVALID
				&& token() != lk::lexer::END
				&& m_errorList.size() == 0 )
			{

				node_t *lhs = additive();
				if ( !lhs )
				{
					error("%d: could not parse equation lhs", line());
					break;
				}

				match( lk::lexer::OP_ASSIGN );
				node_t *rhs = additive();
				if ( !rhs )
				{
					error("%d: could not parse equation rhs", line());
					delete lhs;
					break;
				}

				lk::expr_t *expr = new lk::expr_t(0, lk::expr_t::MINUS,  lhs, rhs );
				m_eqnList.push_back( expr );

				match( lk::lexer::SEP_SEMI );
			}

			if (m_errorList.size() > 0)
			{
				for ( size_t i=0;i<m_eqnList.size();i++)
					delete m_eqnList[i];
				m_eqnList.clear();
				return false;
			}

			return true;
		}

		bool solve( std::vector< lk_string > &varlist )
		{
			 varlist.clear();
			 for ( size_t i=0;i<m_eqnList.size();i++)
				find_names( m_eqnList[i], varlist );

			 if ( varlist.size() != m_eqnList.size() )
			 {
				 error("#equations != #variables");
				 return false;
			 }

			 // set all initial conditions NaN variables to 1.0
			 for ( size_t i=0;i<varlist.size();i++ )
			 {
				 if ( _isnan( get(varlist[i]) ) )
					 set( varlist[i], 1.0 );
			 }

			 for ( size_t i=0;i<m_eqnList.size();i++ )
			 {
				std::stringstream oss;
				oss << "F" << i << "_0";
				set(oss.str(), eval_eqn( m_eqnList[i], m_values ));
			 }

			 nsolver xx( m_eqnList, varlist, m_values );
			 return xx.exec( 300, 1e-9 );
		}

		lk_string variables()
		{
			std::stringstream oss;
			for ( valtab_t::iterator it = m_values.begin();
				it != m_values.end();
				++it )
				oss << (*it).first  << '=' << (*it).second << std::endl;

			return lk_string( oss.str() );
		}

		void find_names( node_t *n, std::vector<lk_string> &vars )
		{
			if (!n) return;
			if ( lk::expr_t *e = dynamic_cast<lk::expr_t*>(n) )
			{
				find_names( e->left, vars );
				find_names( e->right, vars );
			}
			else if ( lk::funccall_t *f = dynamic_cast<lk::funccall_t*>(n) )
			{
				for ( size_t i=0;i<f->args.size();i++)
					find_names( f->args[i], vars );
			}
			else if ( lk::iden_t *i = dynamic_cast<lk::iden_t*>(n) )
			{
				if ( std::find( vars.begin(), vars.end(), i->name ) == vars.end() )
					vars.push_back( i->name );
			}
		}

	private:
		

		lk::node_t *additive()
		{
			if (m_haltFlag) return 0;
	
			node_t *n = multiplicative();
	
			while ( token( lk::lexer::OP_PLUS )
				|| token( lk::lexer::OP_MINUS ) )
			{
				int oper = token(lk::lexer::OP_PLUS) ? expr_t::PLUS : expr_t::MINUS;
				skip();
		
				node_t *left = n;
				node_t *right = multiplicative();
		
				n = new lk::expr_t( line(), oper, left, right );		
			}
	
			return n;
	
		}		

		lk::node_t *multiplicative()
		{
			if (m_haltFlag) return 0;
	
			node_t *n = exponential();
	
			while ( token( lk::lexer::OP_MULT )
				|| token( lk::lexer::OP_DIV ) )
			{
				int oper = token(lk::lexer::OP_MULT) ? expr_t::MULT : expr_t::DIV ;
				skip();
		
				node_t *left = n;
				node_t *right = exponential();
		
				n = new lk::expr_t( line(), oper, left, right );		
			}
	
			return n;
		}

		lk::node_t *exponential()
		{
			if (m_haltFlag) return 0;
	
			node_t *n = unary();
	
			if ( token(lk::lexer::OP_EXP) )
			{
				skip();
				n = new expr_t( line(), expr_t::EXP, n, exponential() );
			}
	
			return n;
		}

		lk::node_t *unary()
		{
			if (m_haltFlag) return 0;

			switch( token() )
			{
			case lk::lexer::OP_MINUS:
				skip();
				return new lk::expr_t( line(), expr_t::NEG, unary(), 0 );
			default:
				return primary();
			}
		}

		lk::node_t *primary()
		{
			if (m_haltFlag) return 0;
	
			node_t *n = 0;
			switch( token() )
			{
			case lk::lexer::NUMBER:
				n = new lk::constant_t( line(), lex.value() );
				skip();
				return n;
			case lk::lexer::IDENTIFIER:
				{
					lk_string name = lex.text();
					skip();
					if ( token( lk::lexer::SEP_LPAREN ) )
					{
						funccall_t *f =  new lk::funccall_t( line(), name);
						skip();

						while( token() != lk::lexer::INVALID
							&& token() != lk::lexer::END
							&& token() != lk::lexer::SEP_RPAREN )
						{
							f->args.push_back( additive() );
							if ( token() != lk::lexer::SEP_RPAREN )
								match( lk::lexer::SEP_COMMA );
						}

						match( lk::lexer::SEP_RPAREN );
						return f;
					}
					else						
						return new lk::iden_t( line(), name, false, false );
				}
				break;
			default:
				error("expected identifier, function call, or constant value");
				m_haltFlag = true;
				break;
			}

			return 0;
		}
	};
};

class LKSolve : public wxFrame
{
private:
	wxTextCtrl *m_input;
	wxTextCtrl *m_output;

public:
	LKSolve( )
		: wxFrame( NULL, wxID_ANY, "LKSolve", wxDefaultPosition, wxSize(800,500) )
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
		lk::input_string p( m_input->GetValue() );
		lk::eqnsolve ee( p );

		m_output->Clear();
		if (!ee.parse())
		{
			m_output->AppendText( ee.errortext() );
			return;
		}

		m_output->AppendText("parsed ok\n");
		
		std::vector<lk_string> vars;
		if (!ee.solve( vars ))
			m_output->AppendText( ee.errortext() );

		m_output->AppendText("solved ok\n");

		m_output->AppendText(ee.variables());

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