#include <cstdio>
#include <iostream>

#include <sstream>

#include "lk_absyn.h"
#include "lk_lex.h"

int lk::_node_alloc = 0;

const char *lk::expr_t::operstr()
{
	switch(oper)
	{
	case PLUS: return "+";
	case MINUS: return "-";
	case MULT: return "*";
	case DIV: return "/";
	case INCR: return "++";
	case DECR: return "--";
	case DEFINE: return "&define";
	case ASSIGN: return "=";
	case LOGIOR: return "||";
	case LOGIAND: return "&&";
	case NOT: return "!";
	case EQ: return "==";
	case NE: return "!=";
	case LT: return "<";
	case LE: return "<=";
	case GT: return ">";
	case GE: return ">=";
	case EXP: return "^";
	case NEG: return "-";
	case INDEX: return "[]";
	case HASH: return "{}";
	case THISCALL: return "->()";
	case CALL: return "()";
	case RETURN : return "&return";
	case EXIT: return "&exit";
	case BREAK: return "&break";
	case CONTINUE: return "&continue";
	case SIZEOF: return "&sizeof";
	case KEYSOF: return "&keysof";
	case TYPEOF: return "&typeof";
	case INITVEC: return "&initvec";
	case INITHASH: return "&inithash";
	default:
		return "<!inv!>";
	}
}

static lk_string spacer(int lev)
{
	lk_string ret;
	for (int i=0;i<lev;i++) ret += lk_char("   ");
	return ret;
}

void lk::pretty_print( lk_string &str, node_t *root, int level )
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
	else if ( 0 != dynamic_cast<null_t*>( root ) )
	{
		str += spacer(level) + "#null#";
	}
	else
	{
		str += "<!unknown node type!>";
	}
}
