#ifndef __lk_parse_h
#define __lk_parse_h

#include <vector>
#include "lk_lex.h"
#include "lk_absyn.h"

namespace lk
{
	class parser
	{
	public:
		parser( input_base &input);
				
		node_t *script();
		node_t *block();
		node_t *statement();
		node_t *test();
		node_t *loop();
		node_t *define();
		node_t *assignment();		
		node_t *ternary();
		node_t *logicalor();
		node_t *logicaland();
		node_t *equality();
		node_t *relational();
		node_t *additive();
		node_t *multiplicative();
		node_t *exponential();
		node_t *unary();
		node_t *postfix();
		node_t *primary();		
		
		int line() { return lex.line(); }
		int error_count() { return m_errorList.size(); }
		const char *error(int idx);
		
		int token();
		bool token(int t);
		
	private:
		list_t *ternarylist( int septok, int endtok );
		list_t *identifierlist( int septok, int endtok );
	
		void skip();
		bool match(int t);
		bool match( const char *s );
		void error( const char *fmt, ... );
		
		lexer lex;				
		int m_tokType;		
		std::string m_lexError;
		bool m_haltFlag;
		std::vector<std::string> m_errorList;
	};
};

#endif
