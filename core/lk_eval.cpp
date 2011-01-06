#include "lk_eval.h"


bool lk::eval( node_t *root, 
		env_t *env, 
		io_basic *io, 
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
			&& (ok=eval(n->item, env, io, result, false, ctl_id))
			&& ctl_id == CTL_NONE)
		{
			if (!ok)
			{
				io->output("eval error in statement list\n");
				return false;
			}

			n = n->next;
		}
	}
	else if ( iter_t *n = dynamic_cast<iter_t*>( root ) )
	{
		if (!eval(n->init, env, io, result, false, ctl_id)) return false;

		while (1)
		{
			// test the condition
			vardata_t outcome;
			outcome.assign(0.0);

			if (!eval( n->test, env, io, outcome, false, ctl_id )) return false;

			if (!outcome.as_boolean())
				break;

			if (!eval(n->block, env, io, result, false, ctl_id )) return false;
			
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

			if (!eval(n->adv, env, io, result, false, ctl_id )) return false;
		}
	}
	else if ( cond_t *n = dynamic_cast<cond_t*>( root ) )
	{
		vardata_t outcome;
		outcome.assign(0.0);
		if (!eval(n->test, env, io, outcome, false, ctl_id)) return false;

		if (outcome.as_boolean())
			return eval( n->on_true, env, io, result, false, ctl_id );
		else
			return eval( n->on_false, env, io, result, false, ctl_id );
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
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
				ok = ok && eval(n->right, env, io, r, false, ctl_id);
				if (l.deref().type() == vardata_t::STRING
					|| r.deref().type() == vardata_t::STRING)
				{
					result.assign( l.deref().as_string() + r.deref().as_string() );
				}
				else
					result.assign( l.deref().num() + r.deref().num() );
				return ok;
			case expr_t::MINUS:
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
				ok = ok && eval(n->right, env, io, r, false, ctl_id);
				result.assign( l.deref().num() - r.deref().num() );
				return ok;
			case expr_t::MULT:
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
				ok = ok && eval(n->right, env, io, r, false, ctl_id);
				result.assign( l.deref().num() * r.deref().num() );
				return ok;
			case expr_t::DIV:
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
				ok = ok && eval(n->right, env, io, r, false, ctl_id);
				if (r.deref().num() == 0) 
					result.assign( vardata_t::nanval );
				else
					result.assign( l.deref().num() / r.deref().num() );
				return ok;
			case expr_t::INCR:
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
				result.assign( ++l.deref().num() );
				return ok;
			case expr_t::DECR:
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
				result.assign( --l.deref().num() );
				return ok;
			case expr_t::DEFINE:
				result.assign( n );
				return ok;
			case expr_t::ASSIGN:				
				ok = ok && eval(n->left, env, io, l, true, ctl_id);
				ok = ok && eval(n->right, env, io, r, false, ctl_id);
				l.deref().copy( r.deref() );
				return ok;
			case expr_t::LOGIOR:
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
				ok = ok && eval(n->right, env, io, r, false, ctl_id);
				result.assign( (((int)l.deref().num()) || ((int)r.deref().num() )) ? 1 : 0 );
				return ok;
			case expr_t::LOGIAND:
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
				ok = ok && eval(n->right, env, io, r, false, ctl_id);
				result.assign( (((int)l.deref().num()) && ((int)r.deref().num() )) ? 1 : 0 );
				return ok;
			case expr_t::NOT:
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
				result.assign( ((int)l.deref().num()) ? 0 : 1 );
				return ok;
			case expr_t::EQ:
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
				ok = ok && eval(n->right, env, io, r, false, ctl_id);
				result.assign( l.deref().equals(r.deref()) ? 1 : 0 );
				return ok;
			case expr_t::NE:
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
				ok = ok && eval(n->right, env, io, r, false, ctl_id);
				result.assign( l.deref().equals(r.deref()) ? 0 : 1 );
				return ok;
			case expr_t::LT:
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
				ok = ok && eval(n->right, env, io, r, false, ctl_id);
				result.assign( l.deref().lessthan(r.deref()) ? 1 : 0 );
				return ok;
			case expr_t::LE:
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
				ok = ok && eval(n->right, env, io, r, false, ctl_id);
				result.assign( l.deref().lessthan(r.deref())||l.deref().equals(r.deref()) ? 1 : 0 );
				return ok;
			case expr_t::GT:
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
				ok = ok && eval(n->right, env, io, r, false, ctl_id);
				result.assign( !l.deref().lessthan(r.deref())||!l.deref().equals(r.deref()) ? 1 : 0);
				return ok;
			case expr_t::GE:
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
				ok = ok && eval(n->right, env, io, r, false, ctl_id);
				result.assign( !l.deref().lessthan(r.deref()) ? 1 : 0 );
				return ok;
			case expr_t::EXP:
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
				ok = ok && eval(n->right, env, io, r, false, ctl_id);
				result.assign( pow( l.deref().num(), r.deref().num() ) );
				return ok;
			case expr_t::NEG:
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
				result.assign( 0 - l.deref().num() );
				return ok;
			case expr_t::INDEX:
				{
					ok = ok && eval(n->left, env, io, l, env_mutable, ctl_id);
					vardata_t &arr = l.deref();

					ok = ok && eval(n->right, env, io, r, false, ctl_id);
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
					ok = ok && eval(n->left, env, io, l, env_mutable, ctl_id);
					vardata_t &hash = l.deref();

					if (env_mutable
						&& (hash.type() != vardata_t::HASH))
						hash.empty_hash();

					ok = ok && eval(n->right, env, io, r, false, ctl_id);
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
					ok = ok && eval(n->left, env, io, l, false, ctl_id);
					expr_t *define = dynamic_cast<expr_t*>( l.deref().func() );
					if (!define)
					{
						io->output("error in function call: malformed define\n");
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
						io->output("too few arguments provided to function call\n");
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
							if (!eval(p->item, env, io, v, false, c))
							{
								io->output("failed to initialize function call argument\n");
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
					if (!eval( block, &frame, io, result, false, ctl_id ))
					{
						io->output("error inside function call\n");
						return false;
					}

					// reset the sequence control
					if (ctl_id != CTL_EXIT)	ctl_id = CTL_NONE;

					// environment frame will automatically be destroyed here
					return true;
				}
				break;
			case expr_t::SIZEOF:
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
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
					io->output("operand to # (sizeof) must be a vector, string, or hash type\n");
					return false;
				}
				break;
			case expr_t::TYPEOF:
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
				result.assign(l.deref().typestr());
				return ok;
			/*case expr_t::ADDROF:
				{
					iden_t *id = dynamic_cast<iden_t*>(n->left);
					if (!id)
					{
						io->output("cannot obtain pointer to non-identifier expression\n");
						return false;
					}

					vardata_t *v = env->lookup( id->name, true );
					if (!v)
					{
						io->output("cannot obtain pointer to undefined identifier: " + id->name + "\n");
						return false;
					}

					result.assign_pointer( v );
					return true;
				}
				break;
			case expr_t::DEREF:
				{
					ok = ok && eval( n->left, env, io, l, false, ctl_id );
					
					if (l.deref().type() != vardata_t::POINTER)
					{
						io->output("cannot dereference non-identifier expression\n");
						return false;
					}

					if (l.deref().pointer() == 0)
					{
						io->output("cannot dereference null pointer\n");
						return false;
					}

					result.assign( l.deref().pointer() );
					return ok;
				}
				break;*/
			case expr_t::RETURN:
				ok = ok && eval(n->left, env, io, l, false, ctl_id);
				result.copy( l.deref() );
				ctl_id = CTL_RETURN;
				return ok;
			case expr_t::EXIT:
				ctl_id = CTL_EXIT;
				break;
			case expr_t::BREAK:
				ctl_id = CTL_BREAK;
				break;
			case expr_t::CONTINUE:
				ctl_id = CTL_CONTINUE;
				break;
			default:
				break;
			}
		} catch ( vardata_t::error_t e ) {
			io->output( "!error: " + e.text + "\n" );
			return false;
		}
	}
	else if ( iden_t *n = dynamic_cast<iden_t*>( root ) )
	{
		vardata_t *x = env->lookup(n->name, true);

		if (x)
		{
			result.assign( x );
		}
		else if ( !x && env_mutable )
		{
			x = new vardata_t;
			env->assign( n->name, x );
			result.assign( x );
		}
		else
		{
			io->output( "reference to unassigned variable: " + n->name + "\n");
			result.nullify();
			return false;
		}
	}
	else if ( constant_t *n = dynamic_cast<constant_t*>( root ) )
	{
		result.assign(n->value);
	}
	else if ( literal_t *n = dynamic_cast<literal_t*>( root ) )
	{
		result.assign(n->value);
	}
	else if ( null_t *n = dynamic_cast<null_t*>( root ) )
	{
		result.nullify();
	}
	else
	{
		return false;
	}

	return true;
}