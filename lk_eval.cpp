#include <cstdio>
#include <cstring>
#include <cmath>
#include <limits>

#include "lk_eval.h"
#include "lk_invoke.h"

static lk_string make_error( lk::node_t *n, const char *fmt, ...)
{
	char buf[512];

	sprintf(buf, "[%d]: ", n->line() );

	char *p = buf + strlen(buf);

	va_list list;
	va_start( list, fmt );	
	
	#if defined(_WIN32)&&defined(_MSC_VER)
		_vsnprintf( p, 480, fmt, list );
	#else
		vsnprintf( p, 480, fmt, list );
	#endif
	
	va_end( list );

	return lk_string(buf);
}

#define ENV_MUTABLE 0x0001


lk::eval::eval( lk::node_t *tree )
	: m_tree(tree), m_env(&m_localEnv)
{
}

lk::eval::eval( lk::node_t *tree, lk::env_t *env )
	: m_tree(tree), m_env(env)
{
}

lk::eval::~eval()
{
	/* nothing to do */
}

bool lk::eval::run()
{
	unsigned int ctl = CTL_NONE;
	return interpret( m_tree, m_env, m_result, 0, ctl );
}
		
bool lk::eval::special_set( const lk_string &name, vardata_t &val )
{
	throw error_t( "no defined mechanism to set special variable '" + name + "'" );
}

bool lk::eval::special_get( const lk_string &name, vardata_t &val )
{
	throw error_t( "no defined mechanism to get special variable '" + name + "'" );
}


bool lk::eval::interpret( node_t *root, 
		env_t *cur_env, 
		vardata_t &result,
		unsigned int flags,
		unsigned int &ctl_id )
{
	if (!root) return true;


	if ( !on_run( root->line() ) )
		return false; /* abort script execution */
	
	if ( list_t *n = dynamic_cast<list_t*>( root ) )
	{

		ctl_id = CTL_NONE;
		bool ok = true;
		while (n 
			&& (ok=interpret(n->item, cur_env, result, flags, ctl_id))
			&& ctl_id == CTL_NONE)
		{
			if (!ok)
			{
				m_errors.push_back( make_error(n, "eval error in statement list\n" ));
				return false;
			}

			n = n->next;
		}

		return ok;
	}
	else if ( iter_t *n = dynamic_cast<iter_t*>( root ) )
	{
		if (!interpret(n->init, cur_env, result, flags, ctl_id)) return false;

		while (1)
		{
			// test the condition
			vardata_t outcome;
			outcome.assign(0.0);

			if (!interpret( n->test, cur_env, outcome, flags, ctl_id )) return false;

			if (!outcome.as_boolean())
				break;

			if (!interpret(n->block, cur_env, result, flags, ctl_id )) return false;
			
			switch(ctl_id)
			{
			case CTL_BREAK:
			case CTL_RETURN:
				ctl_id = CTL_NONE;
			case CTL_EXIT:
				return true;

			case CTL_CONTINUE:
			default:
				ctl_id = CTL_NONE;
			}

			if (!interpret(n->adv, cur_env, result, flags, ctl_id )) return false;
		}
	
		return true;
	}
	else if ( cond_t *n = dynamic_cast<cond_t*>( root ) )
	{
		vardata_t outcome;
		outcome.assign(0.0);
		if (!interpret(n->test, cur_env, outcome, flags, ctl_id)) return false;

		if (outcome.as_boolean())
			return interpret( n->on_true, cur_env, result, flags, ctl_id );
		else
			return interpret( n->on_false, cur_env, result, flags, ctl_id );
	}
	else if ( expr_t *n = dynamic_cast<expr_t*>( root ) )
	{
		try
		{
			bool ok = true;
			vardata_t l, r;
			double newval;
		
			switch( n->oper )
			{	
			case expr_t::PLUS:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				ok = ok && interpret(n->right, cur_env, r, flags, ctl_id);
				if (l.deref().type() == vardata_t::STRING
					|| r.deref().type() == vardata_t::STRING)
				{
					result.assign( l.deref().as_string() + r.deref().as_string() );
				}
				else
					result.assign( l.deref().num() + r.deref().num() );
				return ok;
			case expr_t::MINUS:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				ok = ok && interpret(n->right, cur_env, r, flags, ctl_id);
				result.assign( l.deref().num() - r.deref().num() );
				return ok;
			case expr_t::MULT:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				ok = ok && interpret(n->right, cur_env, r, flags, ctl_id);
				result.assign( l.deref().num() * r.deref().num() );
				return ok;
			case expr_t::DIV:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				ok = ok && interpret(n->right, cur_env, r, flags, ctl_id);
				if (r.deref().num() == 0) 
					result.assign( std::numeric_limits<double>::quiet_NaN() );
				else
					result.assign( l.deref().num() / r.deref().num() );
				return ok;
			case expr_t::INCR:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				newval = l.deref().num() + 1;
				l.deref().assign( newval );
				result.assign( newval );
				return ok;
			case expr_t::DECR:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				newval = l.deref().num() - 1;
				l.deref().assign( newval );
				result.assign( newval );
				return ok;
			case expr_t::DEFINE:
				result.assign( n );
				return ok;
			case expr_t::ASSIGN:
				// evaluate expression before the lhs identifier
				ok = ok && interpret(n->right, cur_env, r, flags, ctl_id);

				// if on the LHS of the assignment we have a special variable i.e. ${xy}, use a 
				// hack to assign the value to the storage location
				if ( lk::iden_t *iden = dynamic_cast<lk::iden_t*>(n->left) )
					if ( iden->special )
						return ok && special_set( iden->name, r.deref() ); // don't bother to copy rhs to result either.

				// otherwise evaluate the LHS in a mutable context, as normal.
				ok = ok && interpret(n->left, cur_env, l, flags|ENV_MUTABLE, ctl_id);
				l.deref().copy( r.deref() );
				result.copy( r.deref() );
				return ok;
			case expr_t::LOGIOR:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				if ( ((int)l.deref().num()) != 0 ) // short circuit evaluation
				{
					result.assign( 1.0 );
					return ok;
				}
				ok = ok && interpret(n->right, cur_env, r, flags, ctl_id);
				result.assign( (((int)l.deref().num()) || ((int)r.deref().num() )) ? 1 : 0 );
				return ok;
			case expr_t::LOGIAND:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				if ( ((int)l.deref().num()) == 0 ) // short circuit evaluation
				{
					result.assign( 0.0 );
					return ok;
				}
				ok = ok && interpret(n->right, cur_env, r, flags, ctl_id);
				result.assign( (((int)l.deref().num()) && ((int)r.deref().num() )) ? 1 : 0 );
				return ok;
			case expr_t::NOT:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				result.assign( ((int)l.deref().num()) ? 0 : 1 );
				return ok;
			case expr_t::EQ:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				ok = ok && interpret(n->right, cur_env, r, flags, ctl_id);
				result.assign( l.deref().equals(r.deref()) ? 1 : 0 );
				return ok;
			case expr_t::NE:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				ok = ok && interpret(n->right, cur_env, r, flags, ctl_id);
				result.assign( l.deref().equals(r.deref()) ? 0 : 1 );
				return ok;
			case expr_t::LT:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				ok = ok && interpret(n->right, cur_env, r, flags, ctl_id);
				result.assign( l.deref().lessthan(r.deref()) ? 1 : 0 );
				return ok;
			case expr_t::LE:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				ok = ok && interpret(n->right, cur_env, r, flags, ctl_id);
				result.assign( l.deref().lessthan(r.deref())||l.deref().equals(r.deref()) ? 1 : 0 );
				return ok;
			case expr_t::GT:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				ok = ok && interpret(n->right, cur_env, r, flags, ctl_id);
				result.assign( !l.deref().lessthan(r.deref())&&!l.deref().equals(r.deref()) ? 1 : 0);
				return ok;
			case expr_t::GE:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				ok = ok && interpret(n->right, cur_env, r, flags, ctl_id);
				result.assign( !l.deref().lessthan(r.deref()) ? 1 : 0 );
				return ok;
			case expr_t::EXP:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				ok = ok && interpret(n->right, cur_env, r, flags, ctl_id);
				result.assign( pow( l.deref().num(), r.deref().num() ) );
				return ok;
			case expr_t::NEG:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				result.assign( 0 - l.deref().num() );
				return ok;
			case expr_t::INDEX:
				{
					ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
					bool anonymous = ( l.type() == vardata_t::VECTOR );

					vardata_t &arr = l.deref();

					if (!(flags&ENV_MUTABLE) && arr.type() != vardata_t::VECTOR)
					{
						m_errors.push_back( make_error( n->left, "cannot index non array data in non mutable context"  ) );
						return false;
					}

					ok = ok && interpret(n->right, cur_env, r, 0, ctl_id);
					size_t idx = r.deref().as_unsigned();

					if ( (flags&ENV_MUTABLE)
						&& (arr.type() != vardata_t::VECTOR 
							|| arr.length() <= idx))
						arr.resize( idx+1 );

					vardata_t *item = arr.index(idx);
					if ( anonymous )
						result.copy( *item );
					else
						result.assign( item );

					return ok;
				}
			case expr_t::HASH:
				{
					ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
					bool anonymous = ( l.type() == vardata_t::HASH );

					vardata_t &hash = l.deref();

					if ( (flags&ENV_MUTABLE)
						&& (hash.type() != vardata_t::HASH))
						hash.empty_hash();

					ok = ok && interpret(n->right, cur_env, r, 0, ctl_id);
					vardata_t &val = r.deref();

					vardata_t *x = hash.lookup( val.as_string() );
					if ( x )
					{
						if ( anonymous )
							result.copy( *x );
						else
							result.assign( x );
					}
					else if ( (flags&ENV_MUTABLE) )
					{
						hash.assign( val.as_string(), x=new vardata_t );
						result.assign( x );
					}
					else 
						result.nullify();

					return ok;
				}
			case expr_t::CALL:
			case expr_t::THISCALL:
				{
					expr_t *cur_expr = n;

					if ( iden_t *iden = dynamic_cast<iden_t*>( n->left ) )
					{

						// query function table for identifier
						if ( lk::fcallinfo_t *fi = cur_env->lookup_func( iden->name ) )
						{
							
							lk::invoke_t cxt( iden->name, cur_env, result, fi->user_data );
							
							list_t *argvals = dynamic_cast<list_t*>(n->right);
							int nargidx = 0;
							while (argvals)
							{
								vardata_t v;
								unsigned int c = CTL_NONE;

								if (!interpret( argvals->item, cur_env, v, flags, c ))
								{
									m_errors.push_back( make_error( argvals, "failed to evaluate function call argument %d to '", nargidx) + iden->name + "()'\n" );
									return false;
								}

								cxt.arg_list().push_back( v );
								nargidx++;
								argvals = argvals->next;
							}
							
							try {
								if ( fi->f ) (*(fi->f))( cxt );
								else if ( fi->f_ext ) lk::external_call( fi->f_ext, cxt );
								else cxt.error( "invalid internal reference to function callback " + iden->name );
							}
							catch( std::exception &e )
							{
								cxt.error( e.what() );
							}

							if (cxt.has_error())
								m_errors.push_back( make_error( iden, "error in call to '") + iden->name + "()': " + cxt.error() );
							
							return !cxt.has_error();
						}
					}

					ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
					expr_t *define = dynamic_cast<expr_t*>( l.deref().func() );
					if (!define)
					{
						m_errors.push_back( make_error(n, "error in function call: malformed define\n") );
						return false;
					}

					list_t *argnames = dynamic_cast<list_t*>( define->left );
					node_t *block = define->right;

					// create new environment frame
					env_t frame( cur_env );

					// count up expected arguments
					int nargs_expected = 0;
					list_t *p = argnames;
					while(p)
					{
						nargs_expected++;
						p = p->next;
					}

					// count up provided arguments

					list_t *argvals = dynamic_cast<list_t*>(n->right);
					p = argvals;
					int nargs_given = 0;
					while(p)
					{
						nargs_given++;
						p=p->next;
					}

					if (n->oper == expr_t::THISCALL)
					{
						nargs_given++;
					}

					if (nargs_given < nargs_expected)
					{
						m_errors.push_back( make_error(n, 
							"too few arguments provided to function call: %d expected, %d given\n",
							nargs_expected, nargs_given) );
						return false;
					}

					// evaluate each argument and assign it into the new environment
					expr_t *thisexpr =  dynamic_cast<expr_t*>(cur_expr->left);
					if (cur_expr->oper == expr_t::THISCALL
						&&  thisexpr != 0
						&&  thisexpr->left != 0)
					{
						vardata_t thisobj;
						unsigned int c = CTL_NONE;
						if (!interpret(thisexpr->left, cur_env, thisobj, flags, c))
						{
							m_errors.push_back( make_error(cur_expr,
														 "failed to evaluate 'this' parameter 0 for THISCALL -> method"));
							return false;
						}

						if (thisobj.type() != vardata_t::REFERENCE)
						{
							m_errors.push_back( make_error(cur_expr,
														 "'this' parameter did not evaluate to a reference, rather %s!",
														 thisobj.typestr()));
							return false;
						}

						frame.assign( "this", new vardata_t(thisobj) );
					}


					unsigned int c = CTL_NONE;
					list_t *n = argnames;
					p = argvals;
					vardata_t *__args = new vardata_t;
					__args->empty_vector();

					int argindex = 0;
					while (p||n)
					{
						vardata_t v;
						iden_t *id = 0;
						if (p)
						{
							if (!interpret(p->item, cur_env, v, flags, c))
							{
								m_errors.push_back( make_error( p, "failed to initialize function call argument\n" ) );
								return false;
							}

							if (n && (id = dynamic_cast<iden_t*>(n->item)))
								frame.assign( id->name, new vardata_t( v ) );

							__args->vec()->push_back( vardata_t( v ) );
						}

						if (p) p = p->next;
						if (n) n = n->next;

						argindex++;
					}

					frame.assign( "__args", __args );

					// now evaluate the function block in the new environment
					if (!interpret( block, &frame, result, flags, ctl_id ))
					{
						m_errors.push_back( make_error( block, "error inside function call\n" ));
						return false;
					}

					// reset the sequence control
					if (ctl_id != CTL_EXIT)	ctl_id = CTL_NONE;

					// environment frame will automatically be destroyed here
					return true;
				}
				break;
			case expr_t::SIZEOF:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				if (l.deref().type() == vardata_t::VECTOR)
				{
					result.assign( (int) l.deref().length() );
					return ok;
				}
				else if (l.deref().type() == vardata_t::STRING)
				{
					result.assign( (int) l.deref().str().length() );
					return ok;
				}
				else if (l.deref().type() == vardata_t::HASH)
				{
					int count = 0;

					varhash_t *h = l.deref().hash();
					for( varhash_t::iterator it = h->begin();
						it != h->end();
						++it )
					{
						if ( (*it).second->deref().type() != vardata_t::NULLVAL )
							count++;
					}
					result.assign( count );
					return ok;
				}
				else
				{
					m_errors.push_back( make_error(n, "operand to # (sizeof) must be a array, string, or table type\n"));
					return false;
				}
				break;
			case expr_t::KEYSOF:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				if (l.deref().type() == vardata_t::HASH)
				{
					varhash_t *h = l.deref().hash();
					result.empty_vector();
					result.vec()->reserve( h->size() );
					for( varhash_t::iterator it = h->begin();
						it != h->end();
						++it )
					{
						if ( (*it).second->deref().type() != vardata_t::NULLVAL )
							result.vec_append( (*it).first );
					}
					return true;
				}
				else
				{
					m_errors.push_back( make_error(n, "operand to @ (keysof) must be a table") );
					return false;
				}
				break;
			case expr_t::TYPEOF:
				ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
				result.assign(l.deref().typestr());
				return ok;
			case expr_t::INITVEC:
				{
					result.empty_vector();
					list_t *p = dynamic_cast<list_t*>( n->left );
					while(p && ok)
					{
						vardata_t v;
						ok = ok && interpret(p->item, cur_env, v, flags, ctl_id );
						result.vec()->push_back( v.deref() );
						p = p->next;
					}
				}
				return ok && ctl_id==CTL_NONE;
			case expr_t::INITHASH:
				{
					result.empty_hash();
					list_t *p = dynamic_cast<list_t*>( n->left );
					while (p && ok)
					{
						expr_t *assign = dynamic_cast<expr_t*>(p->item);
						if (assign && assign->oper == expr_t::ASSIGN)
						{
							vardata_t vkey, vval;
							ok = ok && interpret(assign->left, cur_env, vkey, flags, ctl_id)
								 && interpret(assign->right, cur_env, vval, flags, ctl_id);

							if (ok)
							{
								lk_string key = vkey.as_string();
								varhash_t *h = result.hash();
								varhash_t::iterator it = h->find( key );
								if (it != h->end())
									(*it).second->copy( vval );
								else
									(*h)[key] = new vardata_t( vval );
							}
						}

						p = p->next;
					}
				}
				return ok && ctl_id == CTL_NONE;
			case expr_t::SWITCH:
			{			   
				vardata_t switchval;
				switchval.assign( -1.0 );
				if (!interpret(n->left, cur_env, switchval, flags, ctl_id)) return false;

				int index = switchval.as_integer();
				int i = 0;
				list_t *p = dynamic_cast<list_t*>( n->right );
				while( i < index && p != 0 )
				{
					p = p->next;
					i++;
				}

				if ( i != index || p == 0 )
				{
					m_errors.push_back( make_error(n, "invalid switch statement index of %d", index));
					return false;
				}

				if (!interpret( p->item, cur_env, result, flags, ctl_id)) return false;

				return ok;
			}
			case expr_t::RETURN:
				if ( n->left != 0 ) // handle empty return statements.
				{
					ok = ok && interpret(n->left, cur_env, l, flags, ctl_id);
					result.copy( l.deref() );
				}
				ctl_id = CTL_RETURN;
				return ok;
			case expr_t::EXIT:
				ctl_id = CTL_EXIT;
				return true;
				break;
			case expr_t::BREAK:
				ctl_id = CTL_BREAK;
				return true;
				break;
			case expr_t::CONTINUE:
				ctl_id = CTL_CONTINUE;
				return true;
				break;
			default:
				break;
			}
		} catch ( lk::error_t &e ) {
			m_errors.push_back( make_error(n, "!error: %s\n", (const char*)e.text.c_str()));
			return false;
		}
	}
	else if ( iden_t *n = dynamic_cast<iden_t*>( root ) )
	{
		if ( n->special && !(flags&ENV_MUTABLE) )
			return special_get( n->name, result );

		vardata_t *x = cur_env->lookup(n->name, (
			 ((flags&ENV_MUTABLE) && n->common ) 
			     || !(flags&ENV_MUTABLE) ) ? true : false );

		if (x)
		{
			if (n->constval)
			{
				m_errors.push_back( make_error(n, "overriding previous non-const identifier with const-ness not allowed: %s\n", (const char*)n->name.c_str() ));
				result.nullify();
				return false;
			}

			result.assign( x );
			return true;
		}
		else if ( !x && (flags&ENV_MUTABLE)  )
		{
			x = new vardata_t;

			if (n->constval)
			{
				x->set_flag( vardata_t::CONSTVAL );
				x->clear_flag( vardata_t::ASSIGNED );
			}

			cur_env->assign( n->name, x );
			result.assign( x );
			return true;
		}
		else
		{
			m_errors.push_back( make_error( n, "reference to unassigned variable: %s\n", (const char*)n->name.c_str() ));
			result.nullify();
			return false;
		}
	}
	else if ( constant_t *n = dynamic_cast<constant_t*>( root ) )
	{
		result.assign(n->value);
		return true;
	}
	else if ( literal_t *n = dynamic_cast<literal_t*>( root ) )
	{
		result.assign(n->value);
		return true;
	}
	else if ( 0 != dynamic_cast<null_t*>( root ) )
	{
		result.nullify();
		return true;
	}

	return false;
}
