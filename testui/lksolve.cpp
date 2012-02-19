#include <wx/wx.h>
#include <wx/wfstream.h>
#include <wx/datstrm.h>
#include <wx/tokenzr.h>
#include <wx/busyinfo.h>
#include <wx/splitter.h>

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

#include "jacobian.h"
#include "newton.h"
#include "search.h"
#include "lu.h"

enum { ID_SOLVE = 8145, ID_DEMO };

double bessj0(double x)
{
	double ax,z;
	double xx,y,ans,ans1,ans2;

	if ((ax=fabs(x)) < 8.0) {
		y=x*x;
		ans1=57568490574.0+y*(-13362590354.0+y*(651619640.7
			+y*(-11214424.18+y*(77392.33017+y*(-184.9052456)))));
		ans2=57568490411.0+y*(1029532985.0+y*(9494680.718
			+y*(59272.64853+y*(267.8532712+y*1.0))));
		ans=ans1/ans2;
	} else {
		z=8.0/ax;
		y=z*z;
		xx=ax-0.785398164;
		ans1=1.0+y*(-0.1098628627e-2+y*(0.2734510407e-4
			+y*(-0.2073370639e-5+y*0.2093887211e-6)));
		ans2 = -0.1562499995e-1+y*(0.1430488765e-3
			+y*(-0.6911147651e-5+y*(0.7621095161e-6
			-y*0.934945152e-7)));
		ans=sqrt(0.636619772/ax)*(cos(xx)*ans1-z*sin(xx)*ans2);
	}
	return ans;
}

double bessj1(double x)
{
	double ax,z;
	double xx,y,ans,ans1,ans2;

	if ((ax=fabs(x)) < 8.0) {
		y=x*x;
		ans1=x*(72362614232.0+y*(-7895059235.0+y*(242396853.1
			+y*(-2972611.439+y*(15704.48260+y*(-30.16036606))))));
		ans2=144725228442.0+y*(2300535178.0+y*(18583304.74
			+y*(99447.43394+y*(376.9991397+y*1.0))));
		ans=ans1/ans2;
	} else {
		z=8.0/ax;
		y=z*z;
		xx=ax-2.356194491;
		ans1=1.0+y*(0.183105e-2+y*(-0.3516396496e-4
			+y*(0.2457520174e-5+y*(-0.240337019e-6))));
		ans2=0.04687499995+y*(-0.2002690873e-3
			+y*(0.8449199096e-5+y*(-0.88228987e-6
			+y*0.105787412e-6)));
		ans=sqrt(0.636619772/ax)*(cos(xx)*ans1-z*sin(xx)*ans2);
		if (x < 0.0) ans = -ans;
	}
	return ans;
}

double bessy0(double x)
{
	double bessj0(double x);
	double z;
	double xx,y,ans,ans1,ans2;

	if (x < 8.0) {
		y=x*x;
		ans1 = -2957821389.0+y*(7062834065.0+y*(-512359803.6
			+y*(10879881.29+y*(-86327.92757+y*228.4622733))));
		ans2=40076544269.0+y*(745249964.8+y*(7189466.438
			+y*(47447.26470+y*(226.1030244+y*1.0))));
		ans=(ans1/ans2)+0.636619772*bessj0(x)*log(x);
	} else {
		z=8.0/x;
		y=z*z;
		xx=x-0.785398164;
		ans1=1.0+y*(-0.1098628627e-2+y*(0.2734510407e-4
			+y*(-0.2073370639e-5+y*0.2093887211e-6)));
		ans2 = -0.1562499995e-1+y*(0.1430488765e-3
			+y*(-0.6911147651e-5+y*(0.7621095161e-6
			+y*(-0.934945152e-7))));
		ans=sqrt(0.636619772/x)*(sin(xx)*ans1+z*cos(xx)*ans2);
	}
	return ans;
}

double bessy1(double x)
{
	double bessj1(double x);
	double z;
	double xx,y,ans,ans1,ans2;

	if (x < 8.0) {
		y=x*x;
		ans1=x*(-0.4900604943e13+y*(0.1275274390e13
			+y*(-0.5153438139e11+y*(0.7349264551e9
			+y*(-0.4237922726e7+y*0.8511937935e4)))));
		ans2=0.2499580570e14+y*(0.4244419664e12
			+y*(0.3733650367e10+y*(0.2245904002e8
			+y*(0.1020426050e6+y*(0.3549632885e3+y)))));
		ans=(ans1/ans2)+0.636619772*(bessj1(x)*log(x)-1.0/x);
	} else {
		z=8.0/x;
		y=z*z;
		xx=x-2.356194491;
		ans1=1.0+y*(0.183105e-2+y*(-0.3516396496e-4
			+y*(0.2457520174e-5+y*(-0.240337019e-6))));
		ans2=0.04687499995+y*(-0.2002690873e-3
			+y*(0.8449199096e-5+y*(-0.88228987e-6
			+y*0.105787412e-6)));
		ans=sqrt(0.636619772/x)*(sin(xx)*ans1+z*cos(xx)*ans2);
	}
	return ans;
}


double bessi0(double x)
{
	double ax,ans;
	double y;

	if ((ax=fabs(x)) < 3.75) {
		y=x/3.75;
		y*=y;
		ans=1.0+y*(3.5156229+y*(3.0899424+y*(1.2067492
			+y*(0.2659732+y*(0.360768e-1+y*0.45813e-2)))));
	} else {
		y=3.75/ax;
		ans=(exp(ax)/sqrt(ax))*(0.39894228+y*(0.1328592e-1
			+y*(0.225319e-2+y*(-0.157565e-2+y*(0.916281e-2
			+y*(-0.2057706e-1+y*(0.2635537e-1+y*(-0.1647633e-1
			+y*0.392377e-2))))))));
	}
	return ans;
}

double bessk0(double x)
{
	double bessi0(double x);
	double y,ans;

	if (x <= 2.0) {
		y=x*x/4.0;
		ans=(-log(x/2.0)*bessi0(x))+(-0.57721566+y*(0.42278420
			+y*(0.23069756+y*(0.3488590e-1+y*(0.262698e-2
			+y*(0.10750e-3+y*0.74e-5))))));
	} else {
		y=2.0/x;
		ans=(exp(-x)/sqrt(x))*(1.25331414+y*(-0.7832358e-1
			+y*(0.2189568e-1+y*(-0.1062446e-1+y*(0.587872e-2
			+y*(-0.251540e-2+y*0.53208e-3))))));
	}
	return ans;
}

double bessi1(double x)
{
	double ax,ans;
	double y;

	if ((ax=fabs(x)) < 3.75) {
		y=x/3.75;
		y*=y;
		ans=ax*(0.5+y*(0.87890594+y*(0.51498869+y*(0.15084934
			+y*(0.2658733e-1+y*(0.301532e-2+y*0.32411e-3))))));
	} else {
		y=3.75/ax;
		ans=0.2282967e-1+y*(-0.2895312e-1+y*(0.1787654e-1
			-y*0.420059e-2));
		ans=0.39894228+y*(-0.3988024e-1+y*(-0.362018e-2
			+y*(0.163801e-2+y*(-0.1031555e-1+y*ans))));
		ans *= (exp(ax)/sqrt(ax));
	}
	return x < 0.0 ? -ans : ans;
}

double bessk1(double x)
{
	double bessi1(double x);
	double y,ans;

	if (x <= 2.0) {
		y=x*x/4.0;
		ans=(log(x/2.0)*bessi1(x))+(1.0/x)*(1.0+y*(0.15443144
			+y*(-0.67278579+y*(-0.18156897+y*(-0.1919402e-1
			+y*(-0.110404e-2+y*(-0.4686e-4)))))));
	} else {
		y=2.0/x;
		ans=(exp(-x)/sqrt(x))*(1.25331414+y*(0.23498619
			+y*(-0.3655620e-1+y*(0.1504268e-1+y*(-0.780353e-2
			+y*(0.325614e-2+y*(-0.68245e-3)))))));
	}
	return ans;
}



namespace lk {
	typedef unordered_map< lk_string, double, lk_string_hash, lk_string_equal > valtab_t;
		
	bool nsolver_callback( int iter, double *x, double *resid, const int n, void *data)
	{
		wxTextCtrl *t = (wxTextCtrl*)data;
		if (!t) return true;

		wxString text;
		text.Printf("\niter=%d\n", iter);
		for (int i=0;i<n;i++)
		{
			text += wxString::Format("x[%d]=%.15lf     resid[%d]=%.15lf\n", i, x[i], i, resid[i] );
		}
		t->AppendText( text );
		return true;
	}

	class eqnsolve
	{
	private:
		
		class funccall_t : public node_t
		{
		public:
			lk_string name;
			std::vector< lk::node_t* > args;
			funccall_t(int line, const lk_string &n) : node_t(line), name(n) {  }
			virtual ~funccall_t() { for (size_t i=0;i<args.size();i++) delete args[i]; }
		};

		bool m_parsed;
		valtab_t m_values;
		bool m_haltFlag;
		lexer lex;				
		int m_tokType;	
		std::vector<lk_string> m_errorList;
		std::vector< lk::node_t* > m_eqnList;
		std::vector< lk_string > m_varList;
		
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
			m_parsed = false;
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
#ifdef _MSC_VER
#define myisnan _isnan
#else
#define myisnan isnan
#endif

		int solve( int max_iter, double ftol, double appfac, wxTextCtrl *log)
		{
			if (!parse()) return -1;

			m_varList.clear();
			for ( size_t i=0;i<m_eqnList.size();i++)
			find_names( m_eqnList[i], m_varList );
			
			int n = m_varList.size();

			if ( n != (int)m_eqnList.size() )
			{
				error("#equations(%d) != #variables(%d)", m_eqnList.size(), n);
				return -2;
			}

			// set all initial conditions NaN variables to 1
			for ( int i=0;i<n;i++ )
			{
				if ( myisnan( get(m_varList[i]) ) )
					set( m_varList[i], 1 );
			}

			double *x = new double[n];
			double *resid = new double[n];

			for (int i=0;i<n;i++)
				x[i] = resid[i] = 0.0;

			for (int i=0;i<n;i++)
				x[i] = m_values[ m_varList[i] ]; // initial values
			
			bool check = false;
			int niter = newton<double, eqnsolve>( x, resid, n, check, *this, 
					max_iter, ftol, ftol, appfac,
					nsolver_callback, log);

			for( int i=0; i < n; i++)
				m_values[ m_varList[i] ] = x[i];

			delete [] x;
			delete [] resid;

			if (check) niter = -999;
			return niter;
		}

		lk_string variables()
		{
			std::stringstream oss;
			oss.setf(std::ios::fixed);
			oss.precision(15);
			for ( valtab_t::iterator it = m_values.begin();
				it != m_values.end();
				++it )
				oss << (*it).first  << '=' << (*it).second << std::endl;

			return lk_string( oss.str() );
		}
		
		
		void operator() ( const double *x, double *f, int n )
		{
			int i;
			for( i=0; i < n; i++)
				m_values[ m_varList[i] ] = x[i];

			// solve each eqn to get f[i];
			for( i=0;i < n;i++)
				f[i] = eval_eqn( m_eqnList[i], m_values );
		}
	private:

	
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
			else if ( funccall_t *f = dynamic_cast<funccall_t*>( n ))
			{

				if (f->name == "sin" && f->args.size() == 1 )
					return ::sin( eval_eqn( f->args[0], t ) );			
				else if (f->name == "cos" && f->args.size() == 1 )
					return ::cos( eval_eqn( f->args[0], t ) );			
				else if (f->name == "tan" && f->args.size() == 1 )
					return ::tan( eval_eqn( f->args[0], t ) );
				else if (f->name == "besj0" && f->args.size() == 1 )
					return ::bessj0( eval_eqn(f->args[0], t) );
				else if (f->name == "besj1" && f->args.size() == 1 )
					return ::bessj1( eval_eqn(f->args[0], t) );
				else if (f->name == "besy0" && f->args.size() == 1 )
					return ::bessy0( eval_eqn(f->args[0], t) );
				else if (f->name == "besy1" && f->args.size() == 1 )
					return ::bessy1( eval_eqn(f->args[0], t) );
				else if (f->name == "besi0" && f->args.size() == 1 )
					return ::bessi0( eval_eqn(f->args[0], t) );
				else if (f->name == "besi1" && f->args.size() == 1 )
					return ::bessi1( eval_eqn(f->args[0], t) );
				else if (f->name == "besk0" && f->args.size() == 1 )
					return ::bessk0( eval_eqn(f->args[0], t) );
				else if (f->name == "besk1" && f->args.size() == 1 )
					return ::bessk1( eval_eqn(f->args[0], t) );
			}

			return std::numeric_limits<double>::quiet_NaN();

		}
		
		void find_names( node_t *n, std::vector<lk_string> &vars )
		{
			if (!n) return;
			if ( lk::expr_t *e = dynamic_cast<lk::expr_t*>(n) )
			{
				find_names( e->left, vars );
				find_names( e->right, vars );
			}
			else if ( funccall_t *f = dynamic_cast<funccall_t*>(n) )
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
				

		bool parse()
		{
			if (m_parsed) return true;
			m_parsed = true;
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
			case lk::lexer::SEP_LPAREN:
				skip();
				n = additive();
				match(lk::lexer::SEP_RPAREN);
				return n;
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
						funccall_t *f =  new funccall_t( line(), name);
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

static const char *eqn_default=
"h=500;\n"
"D=0.1;\n"
"r=D/2;\n"
"k=20;\n"
"Biot=(h*r/2)/k;\n"
"lambda*besj1(lambda)-Biot*besj0(lambda)=0;\n"
"C=1/lambda*besj1(lambda)/(1/2*(besj0(lambda)^2 + besj1(lambda)^2));\n";

class LKSolve : public wxFrame
{
private:
	wxTextCtrl *m_input;
	wxTextCtrl *m_output;
	wxTextCtrl *m_maxIter;
	wxTextCtrl *m_fTol;
	wxTextCtrl *m_appFac;
	wxCheckBox *m_showIters;

public:
	LKSolve( )
		: wxFrame( NULL, wxID_ANY, "frees", wxDefaultPosition, wxSize(800,600) )
	{
		wxBoxSizer *tools = new wxBoxSizer( wxHORIZONTAL );
		tools->Add( new wxButton(this, ID_SOLVE, "Solve" ) );
		tools->Add( new wxStaticText(this, wxID_ANY, "   MaxIter:") );
		tools->Add( m_maxIter = new wxTextCtrl(this, wxID_ANY, "10000") );
		tools->Add( new wxStaticText(this, wxID_ANY, "   fTol:") );
		tools->Add( m_fTol = new wxTextCtrl(this, wxID_ANY, "1e-11") );
		tools->Add( new wxStaticText(this, wxID_ANY, "   Appfac:") );
		tools->Add( m_appFac = new wxTextCtrl(this, wxID_ANY, "0.2") );
		tools->Add( m_showIters = new wxCheckBox(this, wxID_ANY, "Show intermediates") );
		tools->Add( new wxButton(this, ID_DEMO, "Demo" ) );
		m_showIters->SetValue( false );

		tools->AddStretchSpacer();
	
		wxSplitterWindow *split = new wxSplitterWindow( this );

		m_input = new wxTextCtrl( split, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
		m_input->SetFont( wxFont(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Consolas") );
		m_output = new wxTextCtrl( split, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
		m_output->SetFont( wxFont(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Consolas") );
		m_output->SetForegroundColour( *wxBLUE );
		
		m_input->SetValue( eqn_default );

		split->SplitHorizontally( m_input, m_output );

		wxBoxSizer *sz = new wxBoxSizer( wxVERTICAL );
		sz->Add( tools, 0, wxALL|wxEXPAND );
		sz->Add( split,  1, wxALL|wxEXPAND );
		SetSizer( sz );

		m_input->SetFocus();

		wxFFileInputStream file( "solver.txt" );
		if (file.IsOk())
			m_input->SetValue( wxDataInputStream(file).ReadString() );

		wxAcceleratorEntry entries[7];
		entries[0].Set( wxACCEL_NORMAL, WXK_F5, ID_SOLVE );
		/*entries[1].Set( wxACCEL_CTRL,   's',  wxID_SAVE );
		entries[2].Set( wxACCEL_CTRL,   'o',  wxID_OPEN );
		entries[3].Set( wxACCEL_NORMAL, WXK_F2, wxID_PREFERENCES );
		entries[4].Set( wxACCEL_NORMAL, WXK_F3, ID_SHOW_STATS );
		entries[5].Set( wxACCEL_NORMAL, WXK_F4, ID_ADD_VARIABLE );
		entries[6].Set( wxACCEL_NORMAL, WXK_F5, ID_START );*/
		SetAcceleratorTable( wxAcceleratorTable(1,entries) );

	}

	void OnDemo( wxCommandEvent & )
	{
		m_input->SetValue(eqn_default);
	}

	void OnSolve( wxCommandEvent & )
	{
		wxBusyInfo inf("Solving, please wait");
		wxYield();

		lk::input_string p( m_input->GetValue() );
		lk::eqnsolve ee( p );

		m_output->Clear();

		wxStopWatch sw;
		int code = ee.solve( atoi( m_maxIter->GetValue().c_str() ),
			atof( m_fTol->GetValue().c_str() ),
			atof( m_appFac->GetValue().c_str() ),
			m_showIters->GetValue() ? m_output : 0 );
		if ( code < 0 )
		{
			m_output->AppendText( ee.errortext() );
		}
		else
		{
			m_output->AppendText(wxString::Format("solved ok (%d iter, %.3lf sec)\n\n", code, sw.Time()/1000.0));
			m_output->AppendText(ee.variables());
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
	EVT_BUTTON( ID_DEMO, LKSolve::OnDemo )
	EVT_CLOSE( LKSolve::OnCloseFrame )
END_EVENT_TABLE()

void new_solver()
{
	wxLog::EnableLogging( false );
	LKSolve *l = new LKSolve;
	l->Show();
}