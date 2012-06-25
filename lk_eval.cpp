#include <cstdio>
#include <cstring>
#include <cmath>
#include <limits>

#include "lk_eval.h"

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

bool lk::eval( node_t *root, 
		env_t *env, 
		std::vector< lk_string > &errors,
		vardata_t &result,
		unsigned int flags,
		unsigned int &ctl_id,
		bool (*cb_func)(int, void *),
		void *cb_data )
{
	if (!root) return true;

	if (cb_func != 0
		&& ! (*cb_func)(root->line(), cb_data))
	{
		/* abort script execution */
		return false;
	}

	if ( list_t *n = dynamic_cast<list_t*>( root ) )
	{

		ctl_id = CTL_NONE;
		bool ok = true;
		while (n 
			&& (ok=eval(n->item, env, errors, result, flags, ctl_id, cb_func, cb_data))
			&& ctl_id == CTL_NONE)
		{
			if (!ok)
			{
				errors.push_back( make_error(n, "eval error in statement list\n" ));
				return false;
			}

			n = n->next;
		}

		return ok;
	}
	else if ( iter_t *n = dynamic_cast<iter_t*>( root ) )
	{
		if (!eval(n->init, env, errors, result, flags, ctl_id, cb_func, cb_data)) return false;

		while (1)
		{
			// test the condition
			vardata_t outcome;
			outcome.assign(0.0);

			if (!eval( n->test, env, errors, outcome, flags, ctl_id, cb_func, cb_data )) return false;

			if (!outcome.as_boolean())
				break;

			if (!eval(n->block, env, errors, result, flags, ctl_id, cb_func, cb_data )) return false;
			
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

			if (!eval(n->adv, env, errors, result, flags, ctl_id, cb_func, cb_data )) return false;
		}
	
		return true;
	}
	else if ( cond_t *n = dynamic_cast<cond_t*>( root ) )
	{
		vardata_t outcome;
		outcome.assign(0.0);
		if (!eval(n->test, env, errors, outcome, flags, ctl_id, cb_func, cb_data)) return false;

		if (outcome.as_boolean())
			return eval( n->on_true, env, errors, result, flags, ctl_id, cb_func, cb_data );
		else
			return eval( n->on_false, env, errors, result, flags, ctl_id, cb_func, cb_data );
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
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
				ok = ok && eval(n->right, env, errors, r, flags, ctl_id, cb_func, cb_data);
				if (l.deref().type() == vardata_t::STRING
					|| r.deref().type() == vardata_t::STRING)
				{
					result.assign( l.deref().as_string() + r.deref().as_string() );
				}
				else
					result.assign( l.deref().num() + r.deref().num() );
				return ok;
			case expr_t::MINUS:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
				ok = ok && eval(n->right, env, errors, r, flags, ctl_id, cb_func, cb_data);
				result.assign( l.deref().num() - r.deref().num() );
				return ok;
			case expr_t::MULT:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
				ok = ok && eval(n->right, env, errors, r, flags, ctl_id, cb_func, cb_data);
				result.assign( l.deref().num() * r.deref().num() );
				return ok;
			case expr_t::DIV:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
				ok = ok && eval(n->right, env, errors, r, flags, ctl_id, cb_func, cb_data);
				if (r.deref().num() == 0) 
					result.assign( std::numeric_limits<double>::quiet_NaN() );
				else
					result.assign( l.deref().num() / r.deref().num() );
				return ok;
			case expr_t::INCR:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
				newval = l.deref().num() + 1;
				l.deref().assign( newval );
				result.assign( newval );
				return ok;
			case expr_t::DECR:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
				newval = l.deref().num() - 1;
				l.deref().assign( newval );
				result.assign( newval );
				return ok;
			case expr_t::DEFINE:
				result.assign( n );
				return ok;
			case expr_t::ASSIGN:
				// evaluate expression before the lhs identifier
				ok = ok && eval(n->right, env, errors, r, flags, ctl_id, cb_func, cb_data);
				ok = ok && eval(n->left, env, errors, l, flags|ENV_MUTABLE, ctl_id, cb_func, cb_data);
				l.deref().copy( r.deref() );
				result.copy( r.deref() );
				return ok;
			case expr_t::LOGIOR:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
				ok = ok && eval(n->right, env, errors, r, flags, ctl_id, cb_func, cb_data);
				result.assign( (((int)l.deref().num()) || ((int)r.deref().num() )) ? 1 : 0 );
				return ok;
			case expr_t::LOGIAND:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
				ok = ok && eval(n->right, env, errors, r, flags, ctl_id, cb_func, cb_data);
				result.assign( (((int)l.deref().num()) && ((int)r.deref().num() )) ? 1 : 0 );
				return ok;
			case expr_t::NOT:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
				result.assign( ((int)l.deref().num()) ? 0 : 1 );
				return ok;
			case expr_t::EQ:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
				ok = ok && eval(n->right, env, errors, r, flags, ctl_id, cb_func, cb_data);
				result.assign( l.deref().equals(r.deref()) ? 1 : 0 );
				return ok;
			case expr_t::NE:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
				ok = ok && eval(n->right, env, errors, r, flags, ctl_id, cb_func, cb_data);
				result.assign( l.deref().equals(r.deref()) ? 0 : 1 );
				return ok;
			case expr_t::LT:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
				ok = ok && eval(n->right, env, errors, r, flags, ctl_id, cb_func, cb_data);
				result.assign( l.deref().lessthan(r.deref()) ? 1 : 0 );
				return ok;
			case expr_t::LE:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
				ok = ok && eval(n->right, env, errors, r, flags, ctl_id, cb_func, cb_data);
				result.assign( l.deref().lessthan(r.deref())||l.deref().equals(r.deref()) ? 1 : 0 );
				return ok;
			case expr_t::GT:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
				ok = ok && eval(n->right, env, errors, r, flags, ctl_id, cb_func, cb_data);
				result.assign( !l.deref().lessthan(r.deref())&&!l.deref().equals(r.deref()) ? 1 : 0);
				return ok;
			case expr_t::GE:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
				ok = ok && eval(n->right, env, errors, r, flags, ctl_id, cb_func, cb_data);
				result.assign( !l.deref().lessthan(r.deref()) ? 1 : 0 );
				return ok;
			case expr_t::EXP:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
				ok = ok && eval(n->right, env, errors, r, flags, ctl_id, cb_func, cb_data);
				result.assign( pow( l.deref().num(), r.deref().num() ) );
				return ok;
			case expr_t::NEG:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
				result.assign( 0 - l.deref().num() );
				return ok;
			case expr_t::INDEX:
				{
					ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
					bool anonymous = ( l.type() == vardata_t::VECTOR );

					vardata_t &arr = l.deref();

					if (!(flags&ENV_MUTABLE) && arr.type() != vardata_t::VECTOR)
					{
						errors.push_back( "cannot index non array data in non mutable context" );
						return false;
					}

					ok = ok && eval(n->right, env, errors, r, 0, ctl_id, cb_func, cb_data);
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
					ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
					bool anonymous = ( l.type() == vardata_t::HASH );

					vardata_t &hash = l.deref();

					if ( (flags&ENV_MUTABLE)
						&& (hash.type() != vardata_t::HASH))
						hash.empty_hash();

					ok = ok && eval(n->right, env, errors, r, 0, ctl_id, cb_func, cb_data);
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
						void *call_data = 0;
						if ( lk::fcall_t f = env->lookup_func( iden->name, &call_data ) )
						{
							
							lk::invoke_t cxt( iden->name, env, result, call_data );
							
							list_t *argvals = dynamic_cast<list_t*>(n->right);
							while (argvals)
							{
								vardata_t v;
								unsigned int c = CTL_NONE;

								if (!eval( argvals->item, env, errors, v, flags, c, cb_func, cb_data ))
								{
									errors.push_back( make_error( argvals, "failed to evaluate function call argument\n" ));
									return false;
								}

								cxt.arg_list().push_back( v );

								argvals = argvals->next;
							}
							
							try {
								(*f)( cxt );
							}
							catch( std::exception &e )
							{
								cxt.error( e.what() );
							}

							if (cxt.has_error())
								errors.push_back( "error in call to '" + iden->name + "': " + cxt.error() );
							
							return !cxt.has_error();
						}
					}

					ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
					expr_t *define = dynamic_cast<expr_t*>( l.deref().func() );
					if (!define)
					{
						errors.push_back( make_error(n, "error in function call: malformed define\n") );
						return false;
					}

					list_t *argnames = dynamic_cast<list_t*>( define->left );
					node_t *block = define->right;

					// create new environment frame
					env_t frame( env );

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
						errors.push_back( make_error(n, 
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
						if (!eval(thisexpr->left, env, errors, thisobj, flags, c, cb_func, cb_data))
						{
							errors.push_back( make_error(cur_expr,
														 "failed to evaluate 'this' parameter 0 for THISCALL -> method"));
							return false;
						}

						if (thisobj.type() != vardata_t::REFERENCE)
						{
							errors.push_back( make_error(cur_expr,
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
							if (!eval(p->item, env, errors, v, flags, c, cb_func, cb_data))
							{
								errors.push_back( make_error( p, "failed to initialize function call argument\n" ) );
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
					if (!eval( block, &frame, errors, result, flags, ctl_id, cb_func, cb_data ))
					{
						errors.push_back( make_error( block, "error inside function call\n" ));
						return false;
					}

					// reset the sequence control
					if (ctl_id != CTL_EXIT)	ctl_id = CTL_NONE;

					// environment frame will automatically be destroyed here
					return true;
				}
				break;
			case expr_t::SIZEOF:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
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
					errors.push_back( make_error(n, "operand to # (sizeof) must be a array, string, or table type\n"));
					return false;
				}
				break;
			case expr_t::KEYSOF:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
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
					errors.push_back( make_error(n, "operand to @ (keysof) must be a table") );
					return false;
				}
				break;
			case expr_t::TYPEOF:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
				result.assign(l.deref().typestr());
				return ok;
			case expr_t::INITVEC:
				{
					result.empty_vector();
					list_t *p = dynamic_cast<list_t*>( n->left );
					while(p && ok)
					{
						vardata_t v;
						ok = ok && eval(p->item, env, errors, v, flags, ctl_id, cb_func, cb_data );
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
							ok = ok && eval(assign->left, env, errors, vkey, flags, ctl_id, cb_func, cb_data)
								 && eval(assign->right, env, errors, vval, flags, ctl_id, cb_func, cb_data);

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
			case expr_t::RETURN:
				ok = ok && eval(n->left, env, errors, l, flags, ctl_id, cb_func, cb_data);
				result.copy( l.deref() );
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
			errors.push_back( make_error(n, "!error: %s\n", (const char*)e.text.c_str()));
			return false;
		}
	}
	else if ( iden_t *n = dynamic_cast<iden_t*>( root ) )
	{
		vardata_t *x = env->lookup(n->name, (
			 ((flags&ENV_MUTABLE) && n->common ) 
			     || !(flags&ENV_MUTABLE) ) ? true : false );

		if (x)
		{
			if (n->constval)
			{
				errors.push_back( make_error(n, "overriding previous non-const identifier with const-ness not allowed: %s\n", (const char*)n->name.c_str() ));
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

			env->assign( n->name, x );
			result.assign( x );
			return true;
		}
		else
		{
			errors.push_back( make_error( n, "reference to unassigned variable: %s\n", (const char*)n->name.c_str() ));
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
