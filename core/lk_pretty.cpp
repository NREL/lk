#include <cstdio>
#include <iostream>

#include <sstream>

#include "lk_pretty.h"
#include "lk_lex.h"

static std::string spacer(int lev)
{
	std::string ret;
	for (int i=0;i<lev;i++) ret += "   ";
	return ret;
}

void lk::pretty_print( std::string &str, node_t *root, int level )
{
	if (!root) return;
	
	if ( list_t *n = dynamic_cast<list_t*>( root ) )
	{	
		str += spacer(level) + "{\n";
		while (n)
		{
			pretty_print( str, n->item, level+1 );
			str += "\n";
			n = n->next;
		}		
		str += spacer(level) + "}";
	}
	else if ( iter_t *n = dynamic_cast<iter_t*>( root ) )
	{
		str += spacer(level) + "loop(";
		
		pretty_print(str, n->init, level+1);
		str += "\n";
		
		pretty_print(str, n->test, level+1);
		str += "\n";

		pretty_print(str, n->adv, level+1);
		str += "\n";

		pretty_print(str, n->block, level+1 );
		str += "\n" + spacer(level) + ")";
	}
	else if ( cond_t *n = dynamic_cast<cond_t*>( root ) )
	{
		str += spacer(level) + "cond(";
		pretty_print(str, n->test, level + 1 );
		str += "\n";
		pretty_print(str, n->on_true, level+1);
		if (n->on_false )
			str += "\n";
		pretty_print(str, n->on_false, level+1);
		str += " )";		
	}
	else if ( expr_t *n = dynamic_cast<expr_t*>( root ) )
	{
		str += spacer(level) + "(";
		str += n->operstr();
		str += "\n";
		pretty_print(str, n->left, level+1 );
		if (n->right)
			str += "\n";
		pretty_print( str, n->right, level+1 );
		str += ")";		
	}
	else if ( iden_t *n = dynamic_cast<iden_t*>( root ) )
	{
		str += spacer(level) + n->name;
	}
	else if ( constant_t *n = dynamic_cast<constant_t*>( root ) )
	{
		char buf[64];
		sprintf(buf, "%lg", n->value );
		str += spacer(level) + buf;
	}
	else if ( literal_t *n = dynamic_cast<literal_t*>( root ) )
	{
		str += spacer(level) + "'";
		str += n->value;
		str += "'";
	}
	else if ( null_t *n = dynamic_cast<null_t*>( root ) )
	{
		str += spacer(level) + "#null#";
	}
	else
	{
		str += "<!unknown node type!>";
	}
}