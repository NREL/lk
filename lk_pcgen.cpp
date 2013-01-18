#include <cstdarg>
#include <cstdlib>
#include <cstring>

#include "lk_pcgen.h"

lk::codegen::codegen( input_base &input, const lk_string &name )
	: lex( input )
{
	m_importer = 0;
	m_haltFlag = false;
	m_lastLine = lex.line();
	m_tokType = lex.next();
	m_name = name;
	m_labelCounter = 0;
	m_lastCodeLine = -1;
}

void lk::codegen::set_importer( importer *i )
{
	m_importer = i;
}

void lk::codegen::set_importer( importer *i, const std::vector< lk_string > &nameList )
{
	m_importer = i;
	m_importNameList = nameList;
}

int lk::codegen::token()
{
	return m_tokType;
}

bool lk::codegen::token(int t)
{
	return (m_tokType==t);
}

lk_string lk::codegen::error( int idx, int *line )
{
	if (idx >= 0 && idx < (int)m_errorList.size())
	{
		if ( line != 0 ) *line = m_errorList[idx].line;
		return m_errorList[idx].text;
	}
	return lk_string("");
}


bool lk::codegen::match( const char *s )
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

bool lk::codegen::match(int t)
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

void lk::codegen::skip()
{	
	m_lastLine = lex.line(); // update code line to last successfully accepted token
	m_tokType = lex.next();
	
	if (m_tokType == lk::lexer::INVALID)
		error("invalid sequence in input: %s", (const char*)lex.error().c_str());
}

void lk::codegen::error( const char *fmt, ... )
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

lk_string lk::codegen::label()
{
	char buf[128];
	sprintf(buf, "L%d", ++m_labelCounter );
	return lk_string(buf);
}

void lk::codegen::emit( const lk_string &instruction )
{
	int cur_line = line();
	/*
	if ( cur_line != m_lastCodeLine )
	{
		char buf[64];
		sprintf(buf, "; line %d\n", line());
		m_assembly += buf;
		m_lastCodeLine = cur_line;
	}*/
	m_assembly += "\t" + instruction + "\n";
}

void lk::codegen::label( const lk_string &name )
{
	m_assembly += name + ":";
}

void lk::codegen::comment( const lk_string &cmt )
{
	m_assembly += "; " + cmt + "\n";
}

lk_string lk::codegen::assembly()
{
	return m_assembly;
}

bool lk::codegen::script()
{
	while ( !token(lk::lexer::END)
	   && !token(lk::lexer::INVALID)
		&& statement()
		&& !m_haltFlag );

	return !m_haltFlag;
}


bool lk::codegen::block()
{
	if ( token( lk::lexer::SEP_LCURLY ) )
	{
		match(lk::lexer::SEP_LCURLY);
		
		statement();
		
		if (!token( lk::lexer::SEP_RCURLY ))
		{						
			while ( token() != lk::lexer::END
				&& token() != lk::lexer::INVALID
				&& token() != lk::lexer::SEP_RCURLY
				&& !m_haltFlag )
			{
				statement();
			}
			
		}
		
		if (!match( lk::lexer::SEP_RCURLY ))
		{
			m_haltFlag = true;
			return false;
		}
			
		return true;
	}
	else
		return statement();		
}


bool lk::codegen::statement()
{	
	/*if (lex.text() == "function")
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
			new iden_t( line(), name, true, true ), 
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
		stmt = new expr_t( line(), expr_t::RETURN, ternary(), 0 );
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
	else 	 */
		assignment();
	

	// require semicolon at end of a statement
	if ( !match(lk::lexer::SEP_SEMI) )
	{
		m_haltFlag = true;
		return false;
	}

	return true;
}

/*
bool lk::codegen::enumerate()
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
				new iden_t( line_num, name, true, true ),
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

bool lk::codegen::test()
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

bool lk::codegen::loop()
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

bool lk::codegen::define()
{
	if (m_haltFlag) return 0;
	
	match("define");
	match( lk::lexer::SEP_LPAREN );
	node_t *a = identifierlist( lk::lexer::SEP_COMMA, lk::lexer::SEP_RPAREN );
	match( lk::lexer::SEP_RPAREN );
	node_t *b = block();
	
	return new expr_t( line(), expr_t::DEFINE, a, b );
}

*/
bool lk::codegen::assignment()
{
	if (m_haltFlag) return false;
	
	if (!ternary()) return false;
	
	if ( token(lk::lexer::OP_ASSIGN) )
	{
		skip();
		if (!assignment() ) return false;
		emit("store");
	}
	
	return true;
}

bool lk::codegen::ternary()
{
	if (m_haltFlag) return false;
	
	if (!logicalor()) return false;
	if ( token(lk::lexer::OP_QMARK) )
	{
		lk_string L1 = label();
		lk_string L2 = label();
		emit( "jf " + L1);
		skip();
		if (!ternary()) return false;
		emit ("j " + L2 );
		match( lk::lexer::SEP_COLON );
		label( L1 );
		if (!ternary()) return true;
		label( L2 );
	}
	else
		return true;
}

bool lk::codegen::logicalor()
{
	if (m_haltFlag) return 0;
	
	if (!logicaland()) return false;

	while ( token(lk::lexer::OP_LOGIOR) )
	{
		skip();	
		if (!logicaland()) return false;
		emit( "or" );
	}
	
	return true;
}

bool lk::codegen::logicaland()
{
	if (m_haltFlag) return false;
	
	if (!equality()) return false;

	while ( token(lk::lexer::OP_LOGIAND) )
	{
		skip();	
		if (!equality()) return false;
		emit( "and" );
	}
	
	return true;
}

bool lk::codegen::equality()
{
	if (m_haltFlag) return false;
	
	if (!relational()) return false;
	
	while ( token( lk::lexer::OP_EQ )
		|| token( lk::lexer::OP_NE ) )
	{
		int oper = token(lk::lexer::OP_EQ) ? expr_t::EQ : expr_t::NE;
		skip();
		
		if (!relational()) return false;
		
		emit("equal");
		if ( oper == expr_t::NE )
			emit("not");
	}
	
	return true;
}

bool lk::codegen::relational()
{
	if (m_haltFlag) return false;
	
	if (!additive()) return false;
	
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
		if (!additive()) return false;
		
		if ( oper == expr_t::LT ) emit( "lt" );
		else if ( oper == expr_t::LE ) emit( "le" );
		else if ( oper == expr_t::GT ) emit( "gt" );
		else if ( oper == expr_t::GE ) emit( "ge" );
	}
	
	return true;	
}



bool lk::codegen::additive()
{
	if (m_haltFlag) return false;
	
	if (!multiplicative()) return false;
	
	while ( token( lk::lexer::OP_PLUS )
		|| token( lk::lexer::OP_MINUS ) )
	{
		int oper = token(lk::lexer::OP_PLUS) ? expr_t::PLUS : expr_t::MINUS;
		skip();

		if (!multiplicative()) return false;
		
		if ( oper == expr_t::PLUS ) emit( "add" );
		else emit("sub");
	}
	
	return true;	
}

bool lk::codegen::multiplicative()
{
	if (m_haltFlag) return false;
	
	if (!exponential()) return false;
	
	while ( token( lk::lexer::OP_MULT )
		|| token( lk::lexer::OP_DIV ) )
	{
		int oper = token(lk::lexer::OP_MULT) ? expr_t::MULT : expr_t::DIV ;
		skip();
		
		if (!exponential()) return false;
				
		if ( oper == expr_t::MULT ) emit( "mult" );
		else emit("div");
	}
	
	return true;
}

bool lk::codegen::exponential()
{
	if (m_haltFlag) return false;
	
	if (!unary()) return false;
	
	if ( token(lk::lexer::OP_EXP) )
	{
		skip();
		if (!exponential()) return false;

		emit("power");
	}
	
	return true;
}

bool lk::codegen::unary()
{
	if (m_haltFlag) return 0;
	
	lk::node_t *node = 0;

	switch( token() )
	{
	case lk::lexer::OP_BANG:
		skip();
		unary();
		emit("not");
		return node;
	case lk::lexer::OP_MINUS:
		skip();
		unary();
		emit("neg");
		return node;
	case lk::lexer::OP_POUND:
		skip();
		unary();
		emit("sizeof");
		return node;
	case lk::lexer::OP_AT:
		skip();
		unary();
		emit("keysof");
		return node;
	case lk::lexer::IDENTIFIER:
		if (lex.text() == "typeof")
		{
			skip();
			match( lk::lexer::SEP_LPAREN );
			ternary();
			match( lk::lexer::SEP_RPAREN );
			emit("typeof");
			return node;
		}
	default:
		return postfix();
	}		
}

bool lk::codegen::postfix()
{
	if (m_haltFlag) return false;
	
	if (!primary()) return false;
	
	while ( 1 )
	{
		if ( token( lk::lexer::SEP_LBRACK ) )
		{
			skip();			
			ternary();
			match( lk::lexer::SEP_RBRACK );
			emit( "index" );
		}
		else if ( token( lk::lexer::SEP_LPAREN ) )
		{
			skip();
			ternarylist(lk::lexer::SEP_COMMA, lk::lexer::SEP_RPAREN);
			match( lk::lexer::SEP_RPAREN );
		}
		else if ( token( lk::lexer::SEP_LCURLY ) )
		{
			skip();
			ternary();
			match( lk::lexer::SEP_RCURLY );
			emit( "lookup" );
		}
		else if ( token( lk::lexer::OP_DOT ) )
		{
			// x.value is syntactic sugar for x{"value"}
			skip();
			emit( "push " + lex.text() );
			emit( "lookup" );
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
	/*
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
						*/

		}
		else if ( token( lk::lexer::OP_PP ) )
		{
			emit("inc");
			skip();
		}
		else if ( token( lk::lexer::OP_MM ) )
		{
			emit("dec");
			skip();
		}
		else
			break;
	}
	
	return true;
}

bool lk::codegen::primary()
{
	if (m_haltFlag) return false;
	
	switch( token() )
	{
	case lk::lexer::SEP_LPAREN:
		skip();
		ternary();
		match(lk::lexer::SEP_RPAREN);
		return true;
	/*
	case lk::lexer::SEP_LBRACK:
		skip();
		ternarylist(lk::lexer::SEP_COMMA, lk::lexer::SEP_RBRACK);
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
	*/
	case lk::lexer::NUMBER:
		{
			char buf[256];
			sprintf(buf, "push %lg", lex.value());
			emit(buf);
		}
		skip();
		return true;
	case lk::lexer::LITERAL:
		emit( "push '" + lex.text() + "'" );
		skip();
		return true; 
	case lk::lexer::IDENTIFIER:
		/*
		if (lex.text() == "define")
		{
			n = define();
		}
		else */if (lex.text() == "true")
		{
			emit("push true");
			skip();
		}
		else if (lex.text() == "false")
		{
			emit("push false");
			skip();
		}
		else if (lex.text() == "null")
		{
			emit("push null");
			skip();
		}
		else
		{
			bool common = false;
			bool constval = false;
			
			int nmod = 0;
			while( nmod++ < 2 && (lex.text() == "common" 
				|| lex.text() == "const" ) )
			{
				if (lex.text() == "common") common = true;
				else constval = true;
				skip();
			}

			emit("load " + lex.text() );

			match(lk::lexer::IDENTIFIER);
		}
		return true;
	default:
		error("invalid expression beginning with '%s'", lk::lexer::tokstr(token()));
		m_haltFlag = true;
		return 0;
	}
}

bool lk::codegen::ternarylist( int septok, int endtok)
{
	int count = 0;
		
	while ( token() != lk::lexer::INVALID
		&& token() != lk::lexer::END
		&& token() != endtok 
		&& !m_haltFlag )
	{
		if (!ternary()) return false;
		count++;
		if ( token() != endtok )
			if (!match( septok ))
				m_haltFlag = true;
	}
	
	char buf[64];
	sprintf(buf, "push %d", count);
	emit( buf );
	return true;
}


bool lk::codegen::identifierlist( int septok, int endtok)
{	
	int count = 0;
	while ( !m_haltFlag && token(lk::lexer::IDENTIFIER) )
	{
		emit("push " + lex.text());	
		count++;
		skip();								
		if ( token() != endtok )
			if (!match( septok ))
				m_haltFlag = true;
	}

	char buf[64];
	sprintf(buf, "push %d", count);
	emit( buf );
	
	return !m_haltFlag;
}
