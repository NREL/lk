#ifndef __lk_codegen_h
#define __lk_codegen_h

#include <vector>
#include "lk_lex.h"
#include "lk_absyn.h"
#include "lk_parse.h"

namespace lk
{
	
	class codegen
	{
	public:
		codegen( input_base &input, const lk_string &name = "" );

		void set_importer( importer *imploc );
		void set_importer( importer *imploc, const std::vector< lk_string > &imported_names );
				
		bool script();
		bool block();
		bool statement();
		bool test();
		bool enumerate();
		bool loop();
		bool define();
		bool assignment();		
		bool ternary();
		bool logicalor();
		bool logicaland();
		bool equality();
		bool relational();
		bool additive();
		bool multiplicative();
		bool exponential();
		bool unary();
		bool postfix();
		bool primary();		
		
		int line() { return lex.line(); }
		int error_count() { return m_errorList.size(); }
		lk_string error(int idx, int *line = 0);

		int token();
		bool token(int t);
		
		void skip();
		bool match(int t);
		bool match( const char *s );

		lk_string label();
		void emit( const lk_string &instruction );
		void comment( const lk_string &cmt );
		void label( const lk_string &name );

		lk_string assembly();

		
	private:
		bool ternarylist( int septok, int endtok );
		bool identifierlist( int septok, int endtok );
	
		void error( const char *fmt, ... );
		
		lexer lex;				
		int m_tokType;
		int m_lastLine;
		lk_string m_lexError;
		bool m_haltFlag;
		struct errinfo { int line; lk_string text; };
		std::vector<errinfo> m_errorList;
		importer *m_importer;
		std::vector< lk_string > m_importNameList;
		lk_string m_name;

		lk_string m_assembly;
		int m_labelCounter;
		int m_lastCodeLine;
	};
};

#endif
