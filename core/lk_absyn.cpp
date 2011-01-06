#include "lk_absyn.h"

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
	case CALL: return "()";
	case RETURN : return "&return";
	case EXIT: return "&exit";
	case BREAK: return "&break";
	case CONTINUE: return "&continue";
	case SIZEOF: return "&sizeof";
	case ADDROF: return "&";
	case DEREF: return "*";
	case TYPEOF: return "&typeof";
	default:
		return "<!inv!>";
	}
}

lk::node_t *lk::list_t::make_copy()
{
	list_t *p = this;

	list_t *head = 0, *tail = 0;
	while (p)
	{
		list_t *link = new list_t(p->line(),
							p->item ? p->item->make_copy() : 0,
							0 );

		if ( !head ) head = link;
		if ( tail ) tail->next = link;
		tail = link;

		p = p->next;
	}

	return head;
}

lk::node_t *lk::iter_t::make_copy()
{
	return new lk::iter_t( line(),
		this->init ? this->init->make_copy() : 0,
		this->test ? this->test->make_copy() : 0,
		this->adv ? this->adv->make_copy() : 0,
		this->block ? this->block->make_copy() : 0 );
}

lk::node_t *lk::cond_t::make_copy()
{
	return new lk::cond_t( line(),
		this->test ? this->test->make_copy() : 0,
		this->on_true ? this->on_true->make_copy() : 0,
		this->on_false ? this->on_false->make_copy() : 0 );
}

lk::node_t *lk::expr_t::make_copy()
{
	return new lk::expr_t( line(), this->oper,
		this->left ? this->left->make_copy() : 0,
		this->right ? this->right->make_copy() : 0 );
}

lk::node_t *lk::iden_t::make_copy()
{
	return new lk::iden_t( line(), this->name );
}

lk::node_t *lk::constant_t::make_copy()
{
	return new lk::constant_t( line(), this->value );
}

lk::node_t *lk::literal_t::make_copy()
{
	return new lk::literal_t( line(), this->value );
}