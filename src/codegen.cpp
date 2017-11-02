/***********************************************************************************************************************
*  LK, Copyright (c) 2008-2017, Alliance for Sustainable Energy, LLC. All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
*  following conditions are met:
*
*  (1) Redistributions of source code must retain the above copyright notice, this list of conditions and the following
*  disclaimer.
*
*  (2) Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
*  following disclaimer in the documentation and/or other materials provided with the distribution.
*
*  (3) Neither the name of the copyright holder nor the names of any contributors may be used to endorse or promote
*  products derived from this software without specific prior written permission from the respective party.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
*  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER, THE UNITED STATES GOVERNMENT, OR ANY CONTRIBUTORS BE LIABLE FOR
*  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
*  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
*  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************************************************************/

#include <limits>
#include <numeric>

#include <lk/stdlib.h>
#include <lk/codegen.h>

namespace lk {
	bool codegen::error(const lk_string &s)
	{
		m_errStr = s;
		return false;
	}

	bool codegen::error(const char *fmt, ...)
	{
		char buf[512];
		va_list args;
		va_start(args, fmt);
		vsprintf(buf, fmt, args);
		va_end(args);
		m_errStr = buf;
		return false;
	}

// context flags for pfgen()
#define F_NONE 0x00
#define F_MUTABLE 0x01

codegen::codegen() {
	m_labelCounter = 1;
}


/// transfers stack instructions & variable lists to bytecode
size_t codegen::get( bytecode &bc )
{
	if ( m_asm.size() == 0 ) return 0;

	bc.program.resize( m_asm.size(), 0 );
	bc.debuginfo.resize( m_asm.size(), srcpos_t() );

	for( size_t i=0;i<m_asm.size();i++ )
	{
		instr &ip = m_asm[i];
		if ( ip.label ) m_asm[i].arg = m_labelAddr[ *ip.label ];
		bc.program[i] = (((unsigned int)ip.op)&0x000000FF) | (((unsigned int)ip.arg)<<8);
		bc.debuginfo[i] = m_asm[i].pos;
	}

	bc.constants = m_constData;
	bc.identifiers = m_idList;

	return m_asm.size();
}

/// generates assembly code
void codegen::textout( lk_string &assembly, lk_string &bytecode )
{
	char buf[128];		
		
	for( size_t i=0;i<m_asm.size();i++ )
	{
		instr &ip = m_asm[i];

		if ( ip.label )
			m_asm[i].arg = m_labelAddr[ *ip.label ];

		bool has_label = false;
		// determine if there's a label for this line (not super efficient)
		for( LabelMap::iterator it = m_labelAddr.begin();
			it != m_labelAddr.end();
			++it )
			if ( (int)i == it->second )
			{
				sprintf(buf, "%4s:", (const char*)it->first.c_str() );
				assembly += buf;
				has_label = true;
			}

		if ( !has_label )
			assembly += "     ";

			
		size_t j=0;
		while( op_table[j].name != 0 )
		{
			if ( ip.op == op_table[j].op )
			{
				sprintf( buf, "%4d{%4d} %4s ", ip.pos.line, ip.pos.stmt, op_table[j].name );
				assembly += buf;

				if ( ip.label )
				{
					assembly += (*ip.label);
				}
				else if ( ip.op == PSH )
				{
					static const size_t MAXWIDTH = 24;
					lk_string nnl( m_constData[ip.arg].as_string() );
					if ( nnl.size() > MAXWIDTH )
					{
						nnl = nnl.substr( 0, MAXWIDTH );
						nnl += "...";
					}					
					lk::replace( nnl, "\n", "" );
					assembly += nnl ;
				}
				else if ( ip.op == SET || ip.op == GET || ip.op == RREF 
					|| ip.op == LREF || ip.op == LCREF || ip.op == LGREF || ip.op == ARG )
				{
					assembly += m_idList[ip.arg];
				}
				else if ( ip.op == TCALL || ip.op == CALL || ip.op == VEC || ip.op == HASH || ip.op == SWI )
				{
					sprintf(buf, "(%d)", ip.arg );
					assembly += buf;
				}

				assembly += '\n';

				unsigned int bc = (((unsigned int)ip.op)&0x000000FF) | (((unsigned int)ip.arg)<<8);
				sprintf(buf, "0x%08X\n", bc);
				bytecode += buf;
				break;
			}
			j++;
		}
	}
		
	for( size_t i=0;i<m_constData.size();i++ )
		bytecode += ".data " + m_constData[i].as_string() + "\n";

	for( size_t i=0;i<m_idList.size();i++ )
		bytecode += ".id " + m_idList[i] + "\n";
}

/// returns true if stack of instructions generated
bool codegen::generate( lk::node_t *root )
{
	m_idList.clear();
	m_constData.clear();
	m_asm.clear();
	m_labelAddr.clear();
	m_labelCounter = 0;
	m_breakAddr.clear();
	m_continueAddr.clear();

	return pfgen(root, F_NONE );
}

/// adds id to m_idList if not already added, return index of d
int codegen::place_identifier( const lk_string &id )
{
	for( size_t i=0;i<m_idList.size();i++ )
		if ( m_idList[i] == id )
			return (int)i;

	m_idList.push_back( id );
	return m_idList.size() - 1;
}

/// adds d to m_constData if not already added, return index of d
int codegen::place_const( vardata_t &d )
{
	if ( d.type() == vardata_t::HASH && d.hash()->size() == 2 )
		printf("stop here" );

	for( size_t i=0;i<m_constData.size();i++ )
		if ( m_constData[i].equals( d ) )
			return (int)i;

	m_constData.push_back( d );
	return (int)m_constData.size()-1;
}

int codegen::const_value( double value )
{
	vardata_t x;
	x.assign( value );
	return place_const( x );
}

/// creates a vardata_t for a const string literal
int codegen::const_literal( const lk_string &lit )
{
	vardata_t x;
	x.assign( lit );
	return place_const( x );
}

lk_string codegen::new_label()
{
	char buf[128];
	sprintf(buf, "L%d", m_labelCounter++);
	return lk_string(buf);
}

void codegen::place_label( const lk_string &s )
{
	m_labelAddr[ s ] = (int)m_asm.size();
}

/// makes instructions & adds to m_asm
int codegen::emit( srcpos_t pos, Opcode o, int arg )
{
	// copy previous line's position if parser doesn't know the statement line,
	// such as when generating if-elseif-else structures for the 'J' instruction
	// or for implicit function returns
	if ( pos == srcpos_t::npos && m_asm.size() > 0 )
		pos = m_asm.back().pos; 

	m_asm.push_back( instr( pos, o, arg ) );
	return m_asm.size();
}

int codegen::emit( srcpos_t pos, Opcode o, const lk_string &L )
{
	// copy previous line's position if parser doesn't know the statement line,
	// such as when generating if-elseif-else structures for the 'J' instruction
	// or for implicit function returns
	if ( pos == srcpos_t::npos && m_asm.size() > 0 )
		pos = m_asm.back().pos; 

	m_asm.push_back( instr(pos, o, 0, (const char*) L.c_str()) );
	return m_asm.size();
}

bool codegen::initialize_const_vec( lk::list_t *v, vardata_t &vvec )
{
	if ( !v ) return true; // empty vector

	for( std::vector<node_t*>::iterator it = v->items.begin();
		it != v->items.end();
		++it )
	{
		if ( lk::constant_t *cc = dynamic_cast<constant_t*>( *it ) )				
			vvec.vec_append( cc->value );
		else if ( lk::literal_t *cc2 = dynamic_cast<literal_t*>( *it ) )
			vvec.vec_append( cc2->value );
		else if ( lk::expr_t *expr = dynamic_cast<expr_t*>( *it ) )
		{
			if ( expr->oper == expr_t::INITVEC )
			{
				lk::vardata_t subvec;
				subvec.empty_vector();
				if ( !initialize_const_vec( dynamic_cast<list_t*>(expr->left), subvec ) )
					return false;
				vvec.vec()->push_back( subvec );
			}
			else if ( expr->oper == expr_t::INITHASH )
			{
				lk::vardata_t subhash;
				subhash.empty_hash();
				if ( !initialize_const_hash( dynamic_cast<list_t*>(expr->left), subhash ) )
					return false;
				vvec.vec()->push_back( subhash );
			}
			else
				return false;
		}
		else
			return false;
	}

	return true;
}

bool codegen::initialize_const_hash( lk::list_t *v, vardata_t &vhash )
{
	if ( !v ) return true;

	for( std::vector<node_t*>::iterator it = v->items.begin();
		it != v->items.end();
		++it )
	{
		expr_t *assign = dynamic_cast<expr_t*>( *it );

		if (assign && assign->oper == expr_t::ASSIGN)
		{
			lk_string key;
			vardata_t val;
			if ( lk::literal_t *pkey = dynamic_cast<literal_t*>(assign->left) ) key = pkey->value;
			else return false;
				
			if ( lk::constant_t *cc = dynamic_cast<constant_t*>(assign->right) )				
				val.assign( cc->value );
			else if ( lk::literal_t *cc2 = dynamic_cast<literal_t*>(assign->right) )
				val.assign( cc2->value );
			else if ( lk::expr_t *expr = dynamic_cast<expr_t*>(assign->right) )
			{
				if ( expr->oper == expr_t::INITVEC )
				{
					val.empty_vector();
					if ( !initialize_const_vec( dynamic_cast<list_t*>(expr->left), val ) )
						return false;
				}
				else if ( expr->oper == expr_t::INITHASH )
				{
					val.empty_hash();
					if ( !initialize_const_hash( dynamic_cast<list_t*>(expr->left), val ) )
						return false;
				}
				else
					return false;
			}
			else
				return false;

			vhash.hash_item(key).copy( val );
		}
		else
			return false;
	}

	return true;
}

/// handles stack popping for statements by adding a POP instruction
bool codegen::pfgen_stmt( lk::node_t *root, unsigned int flags )
{
	bool ok = pfgen( root, flags );

	// expressions always leave their value on the stack, so clean it up
	if (expr_t *e = dynamic_cast<expr_t*>(root)) 
	{
		emit( e->srcpos(), POP );
	}
	else if ( cond_t *c = dynamic_cast<cond_t*>(root))
	{
		// inline ternary expressions also leave value on stack
		if ( c->ternary )
			emit( c->srcpos(), POP );
	}
	return ok;
}

/// returns true if instruction stack generation successful
bool codegen::pfgen( lk::node_t *root, unsigned int flags )
{
	if ( !root ) return true;

	if ( list_t *n1 = dynamic_cast<list_t*>( root ) )
	{
		for( std::vector<node_t*>::iterator it = n1->items.begin();
			it != n1->items.end();
			++it )
			if ( !pfgen_stmt( *it, flags ) )
				return false;
	}
	else if ( iter_t *n2 = dynamic_cast<iter_t*>( root ) )
	{
		if ( n2->init && !pfgen_stmt( n2->init, flags ) ) return false;

		// labels for beginning, advancement, and outside end of loop
		lk_string Lb = new_label();
		lk_string Lc = new_label();
		lk_string Le = new_label();
			
		m_continueAddr.push_back( Lc );
		m_breakAddr.push_back( Le );

		place_label( Lb ) ;

		if ( !pfgen( n2->test, flags ) ) return false;

		emit( n2->srcpos(), JF, Le );
			
		pfgen_stmt( n2->block, flags );

		place_label( Lc );
		if ( n2->adv && !pfgen_stmt( n2->adv, flags ) ) return false;

		emit( n2->srcpos(), J, Lb );
		place_label( Le );

		m_continueAddr.pop_back();
		m_breakAddr.pop_back();
	}
	else if ( cond_t *n3 = dynamic_cast<cond_t*>( root ) )
	{	
		bool ternary = n3->ternary;

		lk_string L1 = new_label();
		lk_string L2 = L1;

		pfgen( n3->test, flags );
		emit( n3->srcpos(), JF, L1 );
		
		// if an inline ternary conditional expression,
		// don't pop the expression value off the stack as in 
		// an expression
		if ( ternary ) pfgen( n3->on_true, false );
		else pfgen_stmt( n3->on_true, flags );

		if ( n3->on_false )
		{
			L2 = new_label();

			// use previous assembly output line as statement position for debugging
			// since it's unknown at parse time
			emit( srcpos_t::npos, J, L2 );
			place_label( L1 );

			if ( ternary ) pfgen( n3->on_false, false );
			else pfgen_stmt( n3->on_false, flags );
		}
		place_label( L2 );
	}
	else if ( expr_t *n4 = dynamic_cast<expr_t*>( root ) )
	{
		switch( n4->oper )
		{
		case expr_t::PLUS:
			pfgen( n4->left, flags );
			pfgen( n4->right, flags );
			emit(  n4->srcpos(), ADD );
			break;
		case expr_t::MINUS:
			pfgen( n4->left, flags );
			pfgen( n4->right, flags );
			emit( n4->srcpos(), SUB );
			break;
		case expr_t::MULT:
			pfgen( n4->left, flags );
			pfgen( n4->right, flags );
			emit( n4->srcpos(), MUL );
			break;
		case expr_t::DIV:
			pfgen( n4->left, flags );
			pfgen( n4->right, flags );
			emit( n4->srcpos(), DIV );
			break;
		case expr_t::LT:
			pfgen( n4->left, flags );
			pfgen( n4->right, flags );
			emit( n4->srcpos(), LT );
			break;
		case expr_t::GT:
			pfgen( n4->left, flags );
			pfgen( n4->right, flags );
			emit( n4->srcpos(), GT );
			break;
		case expr_t::LE:
			pfgen( n4->left, flags );
			pfgen( n4->right, flags );
			emit( n4->srcpos(), LE );
			break;
		case expr_t::GE:
			pfgen( n4->left, flags );
			pfgen( n4->right, flags );
			emit( n4->srcpos(), GE );
			break;
		case expr_t::NE:
			pfgen( n4->left, flags );
			pfgen( n4->right, flags );
			emit( n4->srcpos(), NE );
			break;
		case expr_t::EQ:
			pfgen( n4->left, flags );
			pfgen( n4->right, flags );
			emit( n4->srcpos(), EQ );
			break;
		case expr_t::INCR:
			pfgen( n4->left, flags|F_MUTABLE );
			emit( n4->srcpos(), INC );
			break;
		case expr_t::DECR:
			pfgen( n4->left, flags|F_MUTABLE );
			emit( n4->srcpos(), DEC );
			break;
		case expr_t::LOGIOR:
		{
			lk_string Lsc = new_label();
			pfgen(n4->left, flags );
			emit( n4->srcpos(), DUP );
			emit( n4->srcpos(), JT, Lsc );
			pfgen(n4->right, flags);
			emit( n4->srcpos(), OR );
			place_label( Lsc );
		}
			break;
		case expr_t::LOGIAND:
		{
			lk_string Lsc = new_label();
			pfgen(n4->left, flags );
			emit( n4->srcpos(), DUP );
			emit( n4->srcpos(), JF, Lsc );
			pfgen(n4->right, flags );
			emit( n4->srcpos(), AND );
			place_label( Lsc );
		}
			break;
		case expr_t::NOT:
			pfgen(n4->left, flags);
			emit( n4->srcpos(), NOT );
			break;
		case expr_t::NEG:
			pfgen(n4->left, flags);
			emit( n4->srcpos(), NEG );
			break;				
		case expr_t::EXP:
			pfgen(n4->left, flags);
			pfgen(n4->right, flags);
			emit( n4->srcpos(), EXP );
			break;
		case expr_t::INDEX:
			pfgen(n4->left, flags );
			pfgen(n4->right, F_NONE);
			emit( n4->srcpos(), IDX, flags&F_MUTABLE );
			break;
		case expr_t::HASH:
			pfgen(n4->left, flags);
			pfgen(n4->right, F_NONE);
			emit( n4->srcpos(), KEY, flags&F_MUTABLE );
			break;
		case expr_t::MINUSAT:
			pfgen(n4->left, F_NONE );
			pfgen(n4->right, flags);
			emit( n4->srcpos(), MAT );
			break;
		case expr_t::WHEREAT:
			pfgen(n4->left, F_NONE );
			pfgen(n4->right, flags);
			emit( n4->srcpos(), WAT );
			break;
		case expr_t::PLUSEQ:
			pfgen(n4->left, F_NONE);
			pfgen(n4->right, F_NONE);
			emit( n4->srcpos(), ADD );
			pfgen(n4->left, F_MUTABLE );
			emit( n4->srcpos(), WR );
			break;
		case expr_t::MINUSEQ:
			pfgen(n4->left, F_NONE);
			pfgen(n4->right, F_NONE);
			emit( n4->srcpos(), SUB );
			pfgen(n4->left, F_MUTABLE );
			emit( n4->srcpos(), WR );
			break;
		case expr_t::MULTEQ:
			pfgen(n4->left, F_NONE);
			pfgen(n4->right, F_NONE);
			emit( n4->srcpos(), MUL );
			pfgen(n4->left, F_MUTABLE );
			emit( n4->srcpos(), WR );
			break;
		case expr_t::DIVEQ:
			pfgen(n4->left, F_NONE);
			pfgen(n4->right, F_NONE);
			emit( n4->srcpos(), DIV );
			pfgen(n4->left, F_MUTABLE );
			emit( n4->srcpos(), WR );
			break;
		case expr_t::ASSIGN:
			{
				if ( !pfgen( n4->right, flags ) ) return false;

				// if on the LHS of the assignment we have a special variable i.e. ${xy}, use a 
				// hack to assign the value to the storage location
				if ( lk::iden_t *iden = dynamic_cast<lk::iden_t*>(n4->left) )
				{
					if ( iden->special )
					{
						emit( n4->srcpos(), SET, place_identifier(iden->name) );
						return true;
					}
				}

				if (!pfgen(n4->left, F_MUTABLE)) return false;
				emit(n4->srcpos(), WR);
			}
			break;
			case expr_t::CALL:
			case expr_t::THISCALL:
			{
				// make space on stack for the return value
				emit(n4->srcpos(), NUL);

				// evaluate all the arguments and pushon to stack
				list_t *argvals = dynamic_cast<list_t*>(n4->right);
				int nargs = 0;
				if (argvals)
				{
					for (std::vector<node_t*>::iterator it = argvals->items.begin();
						it != argvals->items.end();
						++it)
					{
						pfgen(*it, F_NONE);
						nargs++;
					}
				}
				expr_t *lexpr = dynamic_cast<expr_t*>(n4->left);
				if (n4->oper == expr_t::THISCALL && 0 != lexpr)
				{
					pfgen(lexpr->left, F_NONE);
					emit(n4->srcpos(), DUP);
					pfgen(lexpr->right, F_NONE);
					emit(n4->srcpos(), KEY);
				}
				else
					pfgen(n4->left, F_NONE);

				emit(n4->srcpos(), (n4->oper == expr_t::THISCALL) ? TCALL : CALL, nargs);
			}
			break;
			case expr_t::SIZEOF:
				pfgen(n4->left, F_NONE);
				emit(n4->srcpos(), SZ);
				break;
			case expr_t::KEYSOF:
				pfgen(n4->left, F_NONE);
				emit(n4->srcpos(), KEYS);
				break;
			case expr_t::TYPEOF:
				if (iden_t *iden = dynamic_cast<iden_t*>(n4->left))
					emit(n4->srcpos(), TYP, place_identifier(iden->name));
				else
					return error(lk_tr("invalid 'typeof' expression, identifier required"));
				break;
			case expr_t::INITVEC:
			{
				list_t *p = dynamic_cast<list_t*>(n4->left);
				vardata_t cvec;
				cvec.empty_vector();
				if (p && initialize_const_vec(p, cvec))
				{
					emit(n4->srcpos(), PSH, place_const(cvec));
				}
				else
				{
					int len = 0;
					if (p)
					{
						for (std::vector<node_t*>::iterator it = p->items.begin();
							it != p->items.end();
							++it)
						{
							pfgen(*it, F_NONE);
							len++;
						}
					}
					emit(n4->srcpos(), VEC, len);
				}
			}
			break;
			case expr_t::INITHASH:
			{
				list_t *p = dynamic_cast<list_t*>(n4->left);
				vardata_t chash;
				chash.empty_hash();
				if (p && initialize_const_hash(p, chash))
				{
					emit(n4->srcpos(), PSH, place_const(chash));
				}
				else
				{
					int len = 0;
					if (p)
					{
						for (std::vector<node_t*>::iterator it = p->items.begin();
							it != p->items.end();
							++it)
						{
							expr_t *assign = dynamic_cast<expr_t*>(*it);
							if (assign && assign->oper == expr_t::ASSIGN)
							{
								pfgen(assign->left, F_NONE);
								pfgen(assign->right, F_NONE);
								len++;
							}
						}
					}

					emit(n4->srcpos(), HASH, len);
				}
			}
			break;
			case expr_t::SWITCH:
			{
				lk_string Le(new_label());
				std::vector<lk_string> labels;

				list_t *p = dynamic_cast<list_t*>(n4->right);

				pfgen(n4->left, F_NONE);
				emit(n4->srcpos(), SWI, p ? (int)p->items.size() : 0);

				if (p)
				{
					for (size_t i = 0; i < p->items.size(); i++)
					{
						labels.push_back(new_label());
						emit(n4->srcpos(), J, labels.back());
					}

					for (size_t i = 0; i < p->items.size(); i++)
					{
						place_label(labels[i]);
						pfgen(p->items[i], F_NONE);
						if (i < p->items.size() - 1)
							emit(p->items[i] ? p->items[i]->srcpos() : n4->srcpos(), J, Le);
					}
				}

				place_label(Le);
			}
			break;

			case expr_t::DEFINE:
			{
				lk_string Le(new_label());
				lk_string Lf(new_label());
				emit(n4->srcpos(), J, Le);
				place_label(Lf);

				list_t *p = dynamic_cast<list_t*>(n4->left);
				if (p)
				{
					for (size_t i = 0; i < p->items.size(); i++)
					{
						iden_t *id = dynamic_cast<iden_t*>(p->items[i]);
						emit(p->items[i] ? p->items[i]->srcpos() : n4->srcpos(), ARG, place_identifier(id->name));
					}
				}

				pfgen(n4->right, F_NONE);

				// if the last statement in the function block,
				// is not a return issue an implicit return statement
				if (m_asm.back().op != RET)
				{
					// for implicit return at end of function, use last code line number of block
					srcpos_t posend = n4->srcpos();
					posend.stmt = posend.stmt_end;
					emit(posend, RET, 0);
				}

				place_label(Le);
				emit(n4->srcpos(), FREF, Lf);
			}
			break;

			default:
				return false;
			}
		}
		else if (ctlstmt_t *n5 = dynamic_cast<ctlstmt_t*>(root))
		{
			switch (n5->ictl)
			{
			case ctlstmt_t::RETURN:
				pfgen(n5->rexpr, F_NONE);
				emit(n5->srcpos(), RET, n5->rexpr ? 1 : 0);
				break;

			case ctlstmt_t::BREAK:
				if (m_breakAddr.size() == 0)
					return error(lk_tr("cannot break from outside a loop"));

				emit(n5->srcpos(), J, m_breakAddr.back());
				break;

			case ctlstmt_t::CONTINUE:
				if (m_continueAddr.size() == 0)
					return error(lk_tr("cannot continue from outside a loop"));

				emit(n5->srcpos(), J, m_continueAddr.back());
				break;

			case ctlstmt_t::EXIT:
				emit(n5->srcpos(), END);
				break;

			default:
				return false;
			}
		}
		else if (iden_t *n6 = dynamic_cast<iden_t*>(root))
		{
			if (n6->special)
			{
				emit(n6->srcpos(), GET, place_identifier(n6->name));
				return true;
			}
			else
			{
				Opcode op = RREF;

				if (flags & F_MUTABLE)
				{
					if (n6->globalval) op = LGREF;
					else if (n6->constval) op = LCREF;
					else op = LREF;
				}

				emit(n6->srcpos(), op, place_identifier(n6->name));
			}
		}
		else if (null_t *n7 = dynamic_cast<null_t*>(root))
		{
			emit(n7->srcpos(), NUL);
		}
		else if (constant_t *n8 = dynamic_cast<constant_t*>(root))
		{
			emit(n8->srcpos(), PSH, const_value(n8->value));
		}
		else if (literal_t *n9 = dynamic_cast<literal_t*>(root))
		{
			emit(n9->srcpos(), PSH, const_literal(n9->value));
		}

		return true;
	}
}; // namespace lk