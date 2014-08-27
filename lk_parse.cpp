#include <cstdarg>
#include <cstdlib>
#include <cstring>

#include "lk_parse.h"

lk::parser::parser( input_base &input, const lk_string &name )
	: lex( input )
{
	m_importer = 0;
	m_haltFlag = false;
	m_lastLine = lex.line();
	m_tokType = lex.next();
	m_name = name;
}

void lk::parser::set_importer( importer *i )
{
	m_importer = i;
}

void lk::parser::set_importer( importer *i, const std::vector< lk_string > &nameList )
{
	m_importer = i;
	m_importNameList = nameList;
}

int lk::parser::token()
{
	return m_tokType;
}

bool lk::parser::token(int t)
{
	return (m_tokType==t);
}

lk_string lk::parser::error( int idx, int *line )
{
	if (idx >= 0 && idx < (int)m_errorList.size())
	{
		if ( line != 0 ) *line = m_errorList[idx].line;
		return m_errorList[idx].text;
	}
	return lk_string("");
}


bool lk::parser::match( const char *s )
{
	if ( m_tokType == lk::lexer::END )
	{
		error("reached end of input, but expected '%s'", s);
		return false;
	}
	
	if ( lex.text() != s )
	{
		error("expected '%s' but found '%s'", s, (const char*)lex.text().c_str());
		return false;
	}
	
	skip();
	return true;
}

bool lk::parser::match(int t)
{
	if (m_tokType == lk::lexer::END )
	{
		error("reached end of input, but expected token '%s'", 
			lk::lexer::tokstr(t));
		return false;
	}
	
	if ( m_tokType != t)
	{
		error("expected '%s' but found '%s' %s", 
			lk::lexer::tokstr(t), 
			lk::lexer::tokstr(m_tokType),
			(const char*)lex.text().c_str());
		return false;
	}
	
	skip();
	return true;
}

void lk::parser::skip()
{	
	m_lastLine = lex.line(); // update code line to last successfully accepted token
	m_tokType = lex.next();
	
	if (m_tokType == lk::lexer::INVALID)
		error("invalid sequence in input: %s", (const char*)lex.error().c_str());
}

void lk::parser::error( const char *fmt, ... )
{
	char buf[512];
	
	if ( !m_name.empty() )
		sprintf(buf, "[%s] %d: ", (const char*)m_name.c_str(), m_lastLine );
	else
		sprintf(buf, "%d: ", m_lastLine);
	
	char *p = buf + strlen(buf);

	va_list list;
	va_start( list, fmt );	
	
	#if defined(_WIN32)&&defined(_MSC_VER)
		_vsnprintf( p, 480, fmt, list );
	#else
		vsnprintf( p, 480, fmt, list );
	#endif
	
	errinfo e;
	e.text = buf;
	e.line = m_lastLine;
	m_errorList.push_back( e );

	va_end( list );
}

lk::node_t *lk::parser::script()
{
	list_t *head = 0, *tail = 0;
	node_t *stmt = 0;

	while ( !token(lk::lexer::END)
	   && !token(lk::lexer::INVALID)
		&& (stmt = statement()) )
	{
		list_t *link = new list_t( line(), stmt, 0 );

		if (!head) head = link;
		if (tail) tail->next = link;
		tail = link;
	}

	return head;
}


lk::node_t *lk::parser::block()
{
	if ( token( lk::lexer::SEP_LCURLY ) )
	{
		match(lk::lexer::SEP_LCURLY);
		
		node_t *n = statement();
		
		if (!token( lk::lexer::SEP_RCURLY ))
		{
			list_t *head = new list_t( line(), n, 0 );
			list_t *tail = head;
						
			while ( token() != lk::lexer::END
				&& token() != lk::lexer::INVALID
				&& token() != lk::lexer::SEP_RCURLY
				&& !m_haltFlag )
			{
				list_t *link = new list_t( line(), statement(), 0 );
				
				tail->next = link;				
				tail = link;
			}
			
			n = head;
			
		}
		
		if (!match( lk::lexer::SEP_RCURLY ))
		{
			m_haltFlag = true;
			if (n) delete n;
			return 0;
		}
			
		return n;
	}
	else
		return statement();		
}


lk::node_t *lk::parser::statement()
{
	node_t *stmt = 0;
	
	if (lex.text() == "function")
	{
		skip();
		// syntactic sugar for 'const my_function = define(...) {  };'
		// function my_function(...) {  }
		if ( token() != lexer::IDENTIFIER )
			error("function name missing");

		lk_string name = lex.text();
		skip();
		
		match( lk::lexer::SEP_LPAREN );
		node_t *a = identifierlist( lk::lexer::SEP_COMMA, lk::lexer::SEP_RPAREN );
		match( lk::lexer::SEP_RPAREN );
		node_t *b = block();
		
		return new expr_t( line(), expr_t::ASSIGN,
			new iden_t( line(), name, true, true, false ), 
			new expr_t( line(), expr_t::DEFINE,
				a, b ));
	}
	else if (lex.text() == "if")
	{
		return test();
	}
	else if (lex.text() == "while" || lex.text() == "for")
	{
		return loop();
	}
	else if (token(lk::lexer::SEP_LCURLY))
	{
		return block();
	}
	else if (lex.text() == "return")
	{
		skip();
		lk::node_t *rval = 0;
		if ( token() != lk::lexer::SEP_SEMI )
			rval = ternary();
		stmt = new expr_t( line(), expr_t::RETURN, rval, 0 );
	}
	else if (lex.text() == "exit")
	{
		stmt = new expr_t( line(), expr_t::EXIT, 0, 0 );
		skip();
	}
	else if (lex.text() == "break")
	{
		stmt = new expr_t( line(), expr_t::BREAK, 0, 0 );
		skip();
	}
	else if (lex.text() == "continue")
	{
		stmt = new expr_t( line(), expr_t::CONTINUE, 0, 0 );
		skip();
	}
	else if ( lex.text() == "import" )
	{
		skip();
		if ( token() != lk::lexer::LITERAL )
		{
			error("literal required after import statement");
			skip();
			if ( token() == lk::lexer::SEP_SEMI )
				skip();

			return statement();
		}

		lk_string path = lex.text();
		lk_string expanded_path = path;
		skip();

		bool import_found = false;
		lk_string src_text;
		if ( m_importer != 0 )
			import_found = m_importer->read_source( path, &expanded_path, &src_text );
		else
		{
			FILE *fp = fopen( (const char*)path.c_str(), "r" );
			if (fp)
			{
				import_found = true;
				char c;
				while ( (c=fgetc(fp))!=EOF )
					src_text += c;
				fclose(fp);
			}
		}

		for (size_t k=0;k<m_importNameList.size();k++)
		{
			if (m_importNameList[k] == expanded_path)
			{
				error("circular import of %s impossible", (const char*)path.c_str());
				m_haltFlag = true;
				return 0;
			}
		}

		if ( import_found )
		{
			m_importNameList.push_back( expanded_path );

			lk::input_string p( src_text );
			lk::parser parse( p, path );
			parse.set_importer( m_importer, m_importNameList );

			lk::node_t *tree = parse.script();

			if ( parse.error_count() != 0
				|| parse.token() != lk::lexer::END
				|| tree == 0 )
			{
				error("parse errors in import '%s':", (const char*)path.c_str());
				
				int i=0;
				while ( i < parse.error_count() )
					error( "\t%s", (const char*)parse.error(i++).c_str());
				
				if ( tree != 0 )
					delete tree;

				m_haltFlag = true;
				return 0;
			}
			else
				stmt = tree;
		}
		else
		{
			error("import '%s' could not be located", (const char*)path.c_str());
			return 0;
		}
	}
	else if (lex.text() == "enum")
	{
		stmt = enumerate();
	}
	else 	
		stmt = assignment();
	
	if ( stmt == 0 )
	{
		error("empty program statement encountered");
		return 0;
	}

	// require semicolon at end of a statement
	if ( !match(lk::lexer::SEP_SEMI) )
	{
		if ( stmt != 0 ) delete stmt;
		m_haltFlag = true;
		return 0;
	}

	return stmt;
}

lk::node_t *lk::parser::enumerate()
{
	match("enum");
	match( lk::lexer::SEP_LCURLY );

	double cur_value = 0;
	list_t *head=0, *tail=0;

	while ( !m_haltFlag && token(lk::lexer::IDENTIFIER) )
	{
		int line_num = line();
		lk_string name = lex.text();
		skip();

		if (token(lk::lexer::OP_ASSIGN))
		{
			skip();
			bool plus = false;
			if (token(lk::lexer::OP_PLUS))
			{
				plus = true;
				skip();
			}

			if (token(lk::lexer::NUMBER))
			{
				if (plus)
					cur_value += lex.value() - 1.0; // to adjust for +1 already added
				else if ( lex.value() > cur_value )
					cur_value = lex.value();
				else
					error("values in enumeration must increase");

				skip();
			}
			else
				error("enumerate statements can only contain numeric assignments");

		}

		list_t *link = new list_t( line(), 
			new expr_t( line(), expr_t::ASSIGN,
				new iden_t( line_num, name, true, true, false ),
				new constant_t( line(), cur_value ) ),
			0 );

		if (!head) head = link;
		if (tail) tail->next = link;
		tail = link;

		cur_value += 1.0;

		if ( token() != lk::lexer::SEP_RCURLY )
			if ( !match( lk::lexer::SEP_COMMA ) )
				m_haltFlag = true;
	}

	if ( !head ) error("enumeration must have one or more identifiers");

	match( lk::lexer::SEP_RCURLY );

	return head;
}

lk::node_t *lk::parser::test()
{
	match("if");
	match( lk::lexer::SEP_LPAREN );
	node_t *test = logicalor();
	match( lk::lexer::SEP_RPAREN );
	node_t *on_true = block();

	cond_t *c_top = new cond_t( line(), test, on_true, 0 );

	if ( lex.text() == "else" )
	{
		skip();
		c_top->on_false = block();
	}
	else if ( lex.text() == "elseif" )
	{
		cond_t *tail = c_top;

		while( lex.text() == "elseif" )
		{
			skip();
			match( lk::lexer::SEP_LPAREN );
			test = logicalor();
			match( lk::lexer::SEP_RPAREN );
			on_true = block();

			cond_t *link = new cond_t( line(), test, on_true, 0 );
			tail->on_false = link;
			tail = link;
		}

		if ( lex.text() == "else" )
		{
			skip();
			tail->on_false = block();
		}
	}

	return c_top;
}

lk::node_t *lk::parser::loop()
{
	iter_t *it = 0;

	if ( lex.text() == "while" )
	{
		it = new iter_t( line(), 0, 0, 0, 0 );
		skip();
		match( lk::lexer::SEP_LPAREN );
		it->test = logicalor();
		match( lk::lexer::SEP_RPAREN );
		it->block = block();
	}
	else if ( lex.text() == "for" )
	{
		it = new iter_t( line(), 0, 0, 0, 0 );
		skip();

		match( lk::lexer::SEP_LPAREN );
		
		if ( !token( lk::lexer::SEP_SEMI ) )
			it->init = assignment();
		
		match( lk::lexer::SEP_SEMI );

		if ( !token( lk::lexer::SEP_SEMI ) )
			it->test = logicalor();

		match( lk::lexer::SEP_SEMI );

		if ( !token( lk::lexer::SEP_RPAREN ) )
			it->adv = assignment();

		match( lk::lexer::SEP_RPAREN );

		it->block = block();
	}
	else
	{
		error("invalid looping construct");
		m_haltFlag = true;
	}

	return it;
}

lk::node_t *lk::parser::define()
{
	if (m_haltFlag) return 0;
	
	match("define");
	match( lk::lexer::SEP_LPAREN );
	node_t *a = identifierlist( lk::lexer::SEP_COMMA, lk::lexer::SEP_RPAREN );
	match( lk::lexer::SEP_RPAREN );
	node_t *b = block();
	
	return new expr_t( line(), expr_t::DEFINE, a, b );
}

lk::node_t *lk::parser::assignment()
{
	if (m_haltFlag) return 0;
	
	node_t *n = ternary();
	
	if ( token(lk::lexer::OP_ASSIGN) )
	{
		skip();		
		n = new expr_t(line(), expr_t::ASSIGN, n, assignment() );
	}
	else if ( token(lk::lexer::OP_PLUSEQ) )
	{
		skip();
		n = new expr_t(line(), expr_t::PLUSEQ, n, ternary() );
	}
	else if ( token(lk::lexer::OP_MINUSEQ) )
	{
		skip();
		n = new expr_t(line(), expr_t::MINUSEQ, n, ternary() );
	}
	else if ( token(lk::lexer::OP_MULTEQ) )
	{
		skip();
		n = new expr_t(line(), expr_t::MULTEQ, n, ternary() );
	}
	else if ( token(lk::lexer::OP_DIVEQ) )
	{
		skip();
		n = new expr_t(line(), expr_t::DIVEQ, n, ternary() );
	}
	else if ( token(lk::lexer::OP_MINUSAT) )
	{
		skip();
		n = new expr_t(line(), expr_t::MINUSAT, n, ternary() );
	}

	return n;
}

lk::node_t *lk::parser::ternary()
{
	if (m_haltFlag) return 0;
	
	node_t *test = logicalor();
	if ( token(lk::lexer::OP_QMARK) )
	{
		skip();
		node_t *rtrue = ternary();
		match( lk::lexer::SEP_COLON );
		node_t *rfalse = ternary();
		
		return new lk::cond_t(line(), test, rtrue, rfalse);
	}
	else
		return test;
}

lk::node_t *lk::parser::logicalor()
{
	if (m_haltFlag) return 0;
	
	node_t *n = logicaland();
	while ( token(lk::lexer::OP_LOGIOR) )
	{
		skip();		
		node_t *left = n;
		node_t *right = logicaland();
		n = new lk::expr_t( line(), expr_t::LOGIOR, left, right );
	}
	
	return n;
}

lk::node_t *lk::parser::logicaland()
{
	if (m_haltFlag) return 0;
	
	node_t *n = equality();
	while ( token(lk::lexer::OP_LOGIAND) )
	{
		skip();		
		node_t *left = n;
		node_t *right = equality();
		n = new lk::expr_t( line(), expr_t::LOGIAND, left, right );
	}
	
	return n;
}

lk::node_t *lk::parser::equality()
{
	if (m_haltFlag) return 0;
	
	node_t *n = relational();
	
	while ( token( lk::lexer::OP_EQ )
		|| token( lk::lexer::OP_NE ) )
	{
		int oper = token(lk::lexer::OP_EQ) ? expr_t::EQ : expr_t::NE;
		skip();
		
		node_t *left = n;
		node_t *right = relational();
		
		n = new lk::expr_t( line(), oper, left, right );		
	}
	
	return n;
}

lk::node_t *lk::parser::relational()
{
	if (m_haltFlag) return 0;
	
	node_t *n = additive();
	
	while ( token( lk::lexer::OP_LT )
	  || token( lk::lexer::OP_LE )
	  || token( lk::lexer::OP_GT )
	  || token( lk::lexer::OP_GE ) )
	{
		int oper = expr_t::INVALID;
		
		switch( token() )
		{
		case lk::lexer::OP_LT: oper = expr_t::LT; break;
		case lk::lexer::OP_LE: oper = expr_t::LE; break;
		case lk::lexer::OP_GT: oper = expr_t::GT; break;
		case lk::lexer::OP_GE: oper = expr_t::GE; break;
		default:
			error("invalid relational operator: %s", lk::lexer::tokstr( token() ) );
		}
		
		skip();
		
		node_t *left = n;
		node_t *right = additive();
		
		n = new lk::expr_t( line(), oper, left, right );		
	}
	
	return n;	
}



lk::node_t *lk::parser::additive()
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

lk::node_t *lk::parser::multiplicative()
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

lk::node_t *lk::parser::exponential()
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

lk::node_t *lk::parser::unary()
{
	if (m_haltFlag) return 0;

	switch( token() )
	{
	case lk::lexer::OP_BANG:
		skip();
		return new lk::expr_t( line(), expr_t::NOT, unary(), 0 );
	case lk::lexer::OP_MINUS:
		skip();
		return new lk::expr_t( line(), expr_t::NEG, unary(), 0 );
	case lk::lexer::OP_POUND:
		skip();
		return new lk::expr_t( line(), expr_t::SIZEOF, unary(), 0 );
	case lk::lexer::OP_AT:
		skip();
		return new lk::expr_t( line(), expr_t::KEYSOF, unary(), 0 );
	case lk::lexer::IDENTIFIER:
		if (lex.text() == "typeof")
		{
			skip();
			match( lk::lexer::SEP_LPAREN );
			node_t *id = 0;
			if ( token() == lk::lexer::IDENTIFIER )
			{
				id = new iden_t( line(), lex.text(), false, false, false );
				skip();
			}
			else
			{
				error( "expected identifier in typeof(...) expression" );
				return 0;
			}
			match( lk::lexer::SEP_RPAREN );
			return new lk::expr_t( line(), expr_t::TYPEOF, id, 0 );
		}
	default:
		return postfix();
	}		
}

lk::node_t *lk::parser::postfix()
{
	if (m_haltFlag) return 0;
	
	node_t *left = primary();
	
	while ( 1 )
	{
		if ( token( lk::lexer::SEP_LBRACK ) )
		{
			skip();			
			node_t *right = ternary();
			match( lk::lexer::SEP_RBRACK );
			left = new expr_t( line(), expr_t::INDEX, left, right );
		}
		else if ( token( lk::lexer::SEP_LPAREN ) )
		{
			skip();
			node_t *right = ternarylist(lk::lexer::SEP_COMMA, lk::lexer::SEP_RPAREN);
			match( lk::lexer::SEP_RPAREN );
			left = new expr_t( line(), expr_t::CALL, left, right );
		}
		else if ( token( lk::lexer::SEP_LCURLY ) )
		{
			skip();
			node_t *right = ternary();
			match( lk::lexer::SEP_RCURLY );
			left = new expr_t( line(), expr_t::HASH, left, right );
		}
		else if ( token( lk::lexer::OP_DOT ) )
		{
			// x.value is syntactic sugar for x{"value"}
			skip();			
			left = new expr_t( line(), expr_t::HASH, left, new literal_t( line(), lex.text() ) );
			skip();
		}
		else if ( token (lk::lexer::OP_REF ) )
		{
/*
			pure syntactic translation for:

			obj->append(x) is syntactic sugar for 
					obj.append(obj, x)
			and     obj{"append"}( obj, x )

			basic class definition of a pair with a method:
		
			pair = define(f,s) {
			  obj.first = f;
			  obj.second = s;
			  obj.sum = define(this, balance) {
				 return this.first + this.second + balance;
			  };
			  return obj;
			};
		
			usage:
	
			arr[0] = pair(1,2);
			arr[1] = pair(3,4);

			echo( arr[0]->sum(3) );
			echo( arr[1]->sum(5) );
*/
	
			skip();
			
			if ( !token( lk::lexer::IDENTIFIER ) )
			{
				error("expected method-function name after dereference operator ->");
				skip();
			}			

			lk_string method_iden = lex.text();			
			skip();

			match( lk::lexer::SEP_LPAREN );
			list_t *arg_list = ternarylist( lk::lexer::SEP_COMMA, lk::lexer::SEP_RPAREN );
			match( lk::lexer::SEP_RPAREN );
			

			// at execution, the THISCALL mode
			// will cause a reference to the result of
			// the left hand primary expression to be passed
			// as the first argument in the list
			left = new expr_t( line(), expr_t::THISCALL,
						new expr_t( line(), expr_t::HASH, 
							left,  // primary expression on left of ->  (i.e. 'obj')
							new literal_t( line(), method_iden )),
						arg_list );

		}
		else if ( token( lk::lexer::OP_PP ) )
		{
			left = new expr_t( line(), expr_t::INCR, left, 0 );
			skip();
		}
		else if ( token( lk::lexer::OP_MM ) )
		{
			left = new expr_t( line(), expr_t::DECR, left, 0 );
			skip();
		}
		else if ( token( lk::lexer::OP_QMARKAT ) )
		{
			skip();
			left = new expr_t( line(), expr_t::WHEREAT, left, ternary() );
		}
		else
			break;
	}
	
	return left;
}

lk::node_t *lk::parser::primary()
{
	if (m_haltFlag) return 0;
	
	node_t *n = 0;
	switch( token() )
	{
	case lk::lexer::SEP_LPAREN:
		skip();
		n = ternary();
		match(lk::lexer::SEP_RPAREN);
		return n;
	case lk::lexer::SEP_LBRACK:
		skip();
		n = ternarylist(lk::lexer::SEP_COMMA, lk::lexer::SEP_RBRACK);
		match( lk::lexer::SEP_RBRACK );
		return new lk::expr_t( line(), lk::expr_t::INITVEC, n, 0 );
	case lk::lexer::SEP_LCURLY:
		{
			skip();
			list_t *head=0, *tail=0;
			while ( token() != lk::lexer::INVALID
				&& token() != lk::lexer::END
				&& token() != lk::lexer::SEP_RCURLY
				&& !m_haltFlag )
			{
				list_t *link = new list_t( line(), assignment(), 0 );

				if ( !head ) head = link;

				if (tail) tail->next = link;

				tail = link;

				if ( token() != lk::lexer::SEP_RCURLY )
					if (!match( lk::lexer::SEP_COMMA ))
						m_haltFlag = true;
			}
			match(lk::lexer::SEP_RCURLY );
			return new lk::expr_t( line(), lk::expr_t::INITHASH, head, 0 );
		}		
	case lk::lexer::OP_QMARK:
	{
		// inline integer expression switch: ?? opt [ 1, 2, 3, 4, 5, 6 ]
		skip();
		node_t *value = primary();
		match( lk::lexer::SEP_LBRACK );
		list_t *list = ternarylist( lk::lexer::SEP_COMMA, lk::lexer::SEP_RBRACK );
		match( lk::lexer::SEP_RBRACK );
		return new lk::expr_t( line(), lk::expr_t::SWITCH, value, list );
	}
	case lk::lexer::NUMBER:
		n = new lk::constant_t( line(), lex.value() );
		skip();
		return n;
	case lk::lexer::LITERAL:
		n = new lk::literal_t( line(), lex.text() );
		skip();
		return n; 
	case lk::lexer::SPECIAL: // special identifiers like ${ab.fkn_34}
		n = new lk::iden_t( line(), lex.text(), false, false, true );
		skip();
		return n;
	case lk::lexer::IDENTIFIER:
		if (lex.text() == "define")
		{
			n = define();
		}
		else if (lex.text() == "true")
		{
			n = new lk::constant_t( line(), 1.0 );
			skip();
		}
		else if (lex.text() == "false")
		{
			n = new lk::constant_t( line(), 0.0 );
			skip();
		}
		else if (lex.text() == "null")
		{
			n = new lk::null_t( line() );
			skip();
		}
		else
		{
			bool common = false;
			bool constval = false;
			
			int nmod = 0;
			while( nmod++ < 2 && (lex.text() == "common" 
				|| lex.text() == "const" 
				|| lex.text() == "local" ) )
			{
				if (lex.text() == "local")
					error("variable scoping rules have changed in LK, and the 'local' specifier is no longer valid."
						"please refer to the documentation for details and update your codes accordingly.");

				if (lex.text() == "common") common = true;
				else constval = true;
				skip();
			}

			n = new lk::iden_t( line(), lex.text(), common, constval, false );
			match(lk::lexer::IDENTIFIER);
		}
		return n;
	default:
		error("invalid expression beginning with '%s'", lk::lexer::tokstr(token()));
		m_haltFlag = true;
		return 0;
	}
}

lk::list_t *lk::parser::ternarylist( int septok, int endtok)
{
	list_t *head=0, *tail=0;
		
	while ( token() != lk::lexer::INVALID
		&& token() != lk::lexer::END
		&& token() != endtok 
		&& !m_haltFlag )
	{
		list_t *link = new list_t( line(), ternary(), 0 );
		
		if ( !head ) head = link;
		
		if (tail) tail->next = link;
	
		tail = link;

		if ( token() != endtok )
			if (!match( septok ))
				m_haltFlag = true;
	}
	
	return head;
}


lk::list_t *lk::parser::identifierlist( int septok, int endtok)
{
	list_t *head=0, *tail=0;
		
	while ( !m_haltFlag && token(lk::lexer::IDENTIFIER) )
	{
		list_t *link = new list_t( line(), new iden_t( line(), lex.text(), false, false, false ), 0 );
		
		if ( !head ) head = link;
		
		if (tail) tail->next = link;
		
		tail = link;
		
		skip();
								
		if ( token() != endtok )
			if (!match( septok ))
				m_haltFlag = true;
	}
	
	return head;
}
