#include "lk_eval.h"

static std::string make_error( lk::node_t *n, const char *fmt, ...)
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

	return std::string(buf);
}

bool lk::eval( node_t *root, 
		env_t *env, 
		std::vector< std::string > &errors,
		vardata_t &result,
		bool env_mutable,
		int &ctl_id)
{
	if (!root) return true;

	if ( list_t *n = dynamic_cast<list_t*>( root ) )
	{
		ctl_id = CTL_NONE;
		bool ok = true;
		while (n 
			&& (ok=eval(n->item, env, errors, result, false, ctl_id))			
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
		if (!eval(n->init, env, errors, result, false, ctl_id)) return false;

		while (1)
		{
			// test the condition
			vardata_t outcome;
			outcome.assign(0.0);

			if (!eval( n->test, env, errors, outcome, false, ctl_id )) return false;

			if (!outcome.as_boolean())
				break;

			if (!eval(n->block, env, errors, result, false, ctl_id )) return false;
			
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

			if (!eval(n->adv, env, errors, result, false, ctl_id )) return false;
		}
	
		return true;
	}
	else if ( cond_t *n = dynamic_cast<cond_t*>( root ) )
	{
		vardata_t outcome;
		outcome.assign(0.0);
		if (!eval(n->test, env, errors, outcome, false, ctl_id)) return false;

		if (outcome.as_boolean())
			return eval( n->on_true, env, errors, result, false, ctl_id );
		else
			return eval( n->on_false, env, errors, result, false, ctl_id );
	}
	else if ( expr_t *n = dynamic_cast<expr_t*>( root ) )
	{
		try
		{
			bool ok = true;
			vardata_t l, r;
		
			switch( n->oper )
			{	
			case expr_t::PLUS:
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
				ok = ok && eval(n->right, env, errors, r, false, ctl_id);
				if (l.deref().type() == vardata_t::STRING
					|| r.deref().type() == vardata_t::STRING)
				{
					result.assign( l.deref().as_string() + r.deref().as_string() );
				}
				else
					result.assign( l.deref().num() + r.deref().num() );
				return ok;
			case expr_t::MINUS:
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
				ok = ok && eval(n->right, env, errors, r, false, ctl_id);
				result.assign( l.deref().num() - r.deref().num() );
				return ok;
			case expr_t::MULT:
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
				ok = ok && eval(n->right, env, errors, r, false, ctl_id);
				result.assign( l.deref().num() * r.deref().num() );
				return ok;
			case expr_t::DIV:
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
				ok = ok && eval(n->right, env, errors, r, false, ctl_id);
				if (r.deref().num() == 0) 
					result.assign( vardata_t::nanval );
				else
					result.assign( l.deref().num() / r.deref().num() );
				return ok;
			case expr_t::INCR:
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
				result.assign( ++l.deref().num() );
				return ok;
			case expr_t::DECR:
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
				result.assign( --l.deref().num() );
				return ok;
			case expr_t::DEFINE:
				result.assign( n );
				return ok;
			case expr_t::ASSIGN:				
				ok = ok && eval(n->left, env, errors, l, true, ctl_id);
				ok = ok && eval(n->right, env, errors, r, false, ctl_id);
				l.deref().copy( r.deref() );
				result.copy( r.deref() );
				return ok;
			case expr_t::LOGIOR:
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
				ok = ok && eval(n->right, env, errors, r, false, ctl_id);
				result.assign( (((int)l.deref().num()) || ((int)r.deref().num() )) ? 1 : 0 );
				return ok;
			case expr_t::LOGIAND:
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
				ok = ok && eval(n->right, env, errors, r, false, ctl_id);
				result.assign( (((int)l.deref().num()) && ((int)r.deref().num() )) ? 1 : 0 );
				return ok;
			case expr_t::NOT:
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
				result.assign( ((int)l.deref().num()) ? 0 : 1 );
				return ok;
			case expr_t::EQ:
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
				ok = ok && eval(n->right, env, errors, r, false, ctl_id);
				result.assign( l.deref().equals(r.deref()) ? 1 : 0 );
				return ok;
			case expr_t::NE:
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
				ok = ok && eval(n->right, env, errors, r, false, ctl_id);
				result.assign( l.deref().equals(r.deref()) ? 0 : 1 );
				return ok;
			case expr_t::LT:
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
				ok = ok && eval(n->right, env, errors, r, false, ctl_id);
				result.assign( l.deref().lessthan(r.deref()) ? 1 : 0 );
				return ok;
			case expr_t::LE:
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
				ok = ok && eval(n->right, env, errors, r, false, ctl_id);
				result.assign( l.deref().lessthan(r.deref())||l.deref().equals(r.deref()) ? 1 : 0 );
				return ok;
			case expr_t::GT:
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
				ok = ok && eval(n->right, env, errors, r, false, ctl_id);
				result.assign( !l.deref().lessthan(r.deref())&&!l.deref().equals(r.deref()) ? 1 : 0);
				return ok;
			case expr_t::GE:
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
				ok = ok && eval(n->right, env, errors, r, false, ctl_id);
				result.assign( !l.deref().lessthan(r.deref()) ? 1 : 0 );
				return ok;
			case expr_t::EXP:
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
				ok = ok && eval(n->right, env, errors, r, false, ctl_id);
				result.assign( pow( l.deref().num(), r.deref().num() ) );
				return ok;
			case expr_t::NEG:
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
				result.assign( 0 - l.deref().num() );
				return ok;
			case expr_t::INDEX:
				{
					ok = ok && eval(n->left, env, errors, l, env_mutable, ctl_id);
					vardata_t &arr = l.deref();

					ok = ok && eval(n->right, env, errors, r, false, ctl_id);
					size_t idx = (size_t)r.deref().as_number();

					if (env_mutable 
						&& (arr.type() != vardata_t::VECTOR 
							|| arr.length() <= idx))
						arr.resize( idx+1 );

					result.assign( arr.index( idx ) );
					return ok;
				}
			case expr_t::HASH:
				{
					ok = ok && eval(n->left, env, errors, l, env_mutable, ctl_id);
					vardata_t &hash = l.deref();

					if (env_mutable
						&& (hash.type() != vardata_t::HASH))
						hash.empty_hash();

					ok = ok && eval(n->right, env, errors, r, false, ctl_id);
					vardata_t &val = r.deref();

					vardata_t *x = hash.lookup( val.as_string() );
					if ( x ) 
						result.assign( x );
					else if ( env_mutable )
					{
						hash.assign( val.as_string(), x=new vardata_t );
						result.assign( x );
					}
					else 
						result.nullify();

					return ok;
				}
			case expr_t::CALL:
				{
					if ( iden_t *iden = dynamic_cast<iden_t*>( n->left ) )
					{

						// query function table for identifier
						if ( lk::fcall_t f = env->lookup_func( iden->name ) )
						{
							
							lk::invoke_t cxt(iden->name, env, result );
							
							list_t *argvals = dynamic_cast<list_t*>(n->right);
							while (argvals)
							{
								vardata_t v;
								int c = CTL_NONE;

								if (!eval( argvals->item, env, errors, v, false, c ))
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

							if (cxt.has_error()) errors.push_back( cxt.error() );
							
							return !cxt.has_error();
						}
					}

					ok = ok && eval(n->left, env, errors, l, false, ctl_id);
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

					if (nargs_given < nargs_expected)
					{
						errors.push_back( make_error(n, 
							"too few arguments provided to function call: %d expected, %d given\n",
							nargs_expected, nargs_given) );
						return false;
					}

					// evaluate each argument and assign it into the new environment
					
					int c = CTL_NONE;
					list_t *n = argnames;
					p = argvals;
					vardata_t *__args = new vardata_t;
					__args->empty_vector();

					while (p||n)
					{
						vardata_t v;
						iden_t *id = 0;
						if (p)
						{
							if (!eval(p->item, env, errors, v, false, c))
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
					}

					frame.assign( "__args", __args );

					// now evaluate the function block in the new environment
					if (!eval( block, &frame, errors, result, false, ctl_id ))
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
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
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
					result.assign( (int) l.deref().hash()->size() );
					return ok;
				}
				else
				{
					errors.push_back( make_error(n, "operand to # (sizeof) must be a vector, string, or hash type\n"));
					return false;
				}
				break;
			case expr_t::TYPEOF:
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
				result.assign(l.deref().typestr());
				return ok;
			/*case expr_t::ADDROF:
				{
					iden_t *id = dynamic_cast<iden_t*>(n->left);
					if (!id)
					{
						errors.push_back( make_error(n, "cannot obtain pointer to non-identifier expression\n"));
						return false;
					}

					vardata_t *v = env->lookup( id->name, true );
					if (!v)
					{
						errors.push_back( make_error(n, "cannot obtain pointer to undefined identifier: %s\n", id->name.c_str()));
						return false;
					}

					result.assign_pointer( v );
					return true;
				}
				break;
			case expr_t::DEREF:
				{
					ok = ok && eval( n->left, env, errors, l, false, ctl_id );
					
					if (l.deref().type() != vardata_t::POINTER)
					{
						errors.push_back( make_error(n, "cannot dereference non-identifier expression\n"));
						return false;
					}

					if (l.deref().pointer() == 0)
					{
						errors.push_back( make_error(n, "cannot dereference null pointer\n"));
						return false;
					}

					result.assign( l.deref().pointer() );
					return ok;
				}
				break;*/
			case expr_t::RETURN:
				ok = ok && eval(n->left, env, errors, l, false, ctl_id);
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
			errors.push_back( make_error(n, "!error: %s\n", e.text.c_str()));
			return false;
		}
	}
	else if ( iden_t *n = dynamic_cast<iden_t*>( root ) )
	{
		vardata_t *x = env->lookup(n->name, true);

		if (x)
		{
			result.assign( x );
			return true;
		}
		else if ( !x && env_mutable )
		{
			x = new vardata_t;
			env->assign( n->name, x );
			result.assign( x );
			return true;
		}
		else
		{
			errors.push_back( make_error( n, "reference to unassigned variable: %s\n", n->name.c_str() ));
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
	else if ( null_t *n = dynamic_cast<null_t*>( root ) )
	{
		result.nullify();
		return true;
	}

	return false;
}