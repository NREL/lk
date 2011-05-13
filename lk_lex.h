#ifndef __lk_lex_h
#define __lk_lex_h

#include <string>
#include <cstdio>

/* 

For proper compilation:

Define _WIN32 if on Windows
Define _DEBUG if compile with debugging

*/

namespace lk {

	class input_base
	{
	public:
		virtual ~input_base() { }
		virtual char operator*() = 0;
		virtual char peek() = 0;
		virtual char operator++(int) = 0;
	};
	
	class input_stream : public input_base
	{
	private:
		FILE *m_fp;
		char m_ch;
	public:
		input_stream(FILE *fp);
		virtual ~input_stream();
		bool is_ok();
		
		virtual char operator*();
		virtual char operator++(int);
		virtual char peek();
	};
		
	class input_string : public input_base
	{
	private:
		char *m_buf;
		char *m_p;
	public:
		input_string( const std::string &in );
		virtual ~input_string();
		virtual char operator*();
		virtual char operator++(int);
		virtual char peek();
	};
	
			
	class lexer
	{
	public:
		static const char *tokstr(int t);

		enum {
			INVALID,
			END,
			IDENTIFIER,
			NUMBER,
			LITERAL,
			
			// one character tokens
			SEP_SEMI,
			SEP_COLON,
			SEP_COMMA,
			SEP_LPAREN,
			SEP_RPAREN,
			SEP_LCURLY,
			SEP_RCURLY,
			SEP_LBRACK,
			SEP_RBRACK,

			OP_PLUS,
			OP_MINUS,
			OP_MULT,
			OP_DIV,
			OP_EXP,
			OP_DOT,
			OP_QMARK,
			OP_POUND,
			OP_TILDE,
			OP_PERCENT,
			OP_AT,
			OP_LOGIAND,
			OP_LOGIOR,
			OP_BITAND,
			OP_BITOR,
			OP_BANG,
			OP_ASSIGN,
			OP_REF,
			OP_PP,
			OP_MM,
			OP_LT,
			OP_GT,
			OP_EQ,
			OP_NE,
			OP_LE,
			OP_GE

		};

		lexer( input_base &input );

		int next();
		
		std::string text();
		double value();

		int line();
		std::string error();

	private:
		void whitespace();
		void comments();

		std::string m_error;
		int m_line;
		std::string m_buf;
		double m_val;
		
		input_base &p;
	};
};

#endif
