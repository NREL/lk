#include <algorithm>

#include "lk_env.h"

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

static const unsigned long _nan[2]={0xffffffff, 0x7fffffff};
const double lk::vardata_t::nanval = *( double* )_nan;

lk::vardata_t::vardata_t()
{
	m_type = NULLVAL;
}

lk::vardata_t::~vardata_t()
{
	nullify();
}

bool lk::vardata_t::as_boolean()
{
	if (m_type == NUMBER
		&& m_u.v == 0.0) return false;

	if (m_type == STRING
		&& (strcasecmp( cstr(), "false" ) == 0
			|| strcasecmp( cstr(), "f" ) == 0)) return false;

	if (m_type == REFERENCE) return ref()->as_boolean();

	if (m_type == NULLVAL) return false;

	return true;
}

std::string lk::vardata_t::as_string()
{
	switch (m_type)
	{
	case NULLVAL: return "<null>";
	case REFERENCE: return ref()->as_string();
	/*case POINTER: return "<pointer>";*/
	case NUMBER:
		{
			char buf[1024];
			if ( ((double)((int)m_u.v)) == m_u.v )
				sprintf(buf, "%d", (int) m_u.v);
			else
				sprintf(buf, "%lg", m_u.v);
			return std::string(buf);
		}
	case STRING:
		return *reinterpret_cast< std::string* >(m_u.p);
	case VECTOR:
		{
			register std::vector<vardata_t> &v = *reinterpret_cast< std::vector<vardata_t>* >(m_u.p);

			std::string s;
			for ( size_t i=0;i<v.size();i++ )
			{
				s += v[i].as_string();
				if ( v.size() > 1 && i < v.size()-1 )
					s += ",";
			}
			
			return s;
		}
	case HASH:
		{
			register varhash_t &h = *reinterpret_cast< varhash_t* >(m_u.p);
			std::string s("{ ");

			for ( varhash_t::iterator it = h.begin(); it != h.end(); ++it )
			{
				s += it->first;
				s += "=";
				s += it->second->as_string();
				s += " ";
			}

			s += "}";

			return s;
		}
	case FUNCTION:
		return "<function>";
	default:
		return "<invalid?>";
	}
}

double lk::vardata_t::as_number()
{
	switch( m_type )
	{
	case NULLVAL: return 0;
	case NUMBER: return m_u.v;
	case STRING: return atof( cstr() );
	default:
		return vardata_t::nanval;
	}
}

bool lk::vardata_t::copy( vardata_t &rhs )
{
	switch( rhs.m_type )
	{
	case NULLVAL:
		nullify();
		return true;
	case REFERENCE:
		nullify();
		m_type = REFERENCE;
		m_u.p = rhs.m_u.p;
		return true;
	case NUMBER:
		assign( rhs.m_u.v );
		return true;
	case STRING:
		assign( rhs.str() );
		return true;
	case VECTOR:
		{
			resize( rhs.length() );
			register std::vector<vardata_t> &v = *reinterpret_cast< std::vector<vardata_t>* >(m_u.p);
			std::vector<vardata_t> *rv = rhs.vec();
			for (size_t i=0;i<v.size();i++)
				v[i].copy( (*rv)[i] );
		}
		return true;
	case HASH:
		{
			empty_hash();			
			varhash_t &h = (*reinterpret_cast< varhash_t* >(m_u.p));
			varhash_t &rh = (*reinterpret_cast< varhash_t* >(rhs.m_u.p));

			for (varhash_t::iterator it = rh.begin();
				it != rh.end();
				++it)
			{
				vardata_t *cp = new vardata_t;
				cp->copy( *it->second );
				h[ (*it).first ] = cp;
			}
			
		}
		return true;
	case FUNCTION:
		nullify();
		m_type = FUNCTION;
		m_u.p = rhs.m_u.p;
		return true;
	/*case POINTER:
		nullify();
		m_type = POINTER;
		m_u.p = rhs.m_u.p;
		return true;*/

	default:
		return false;
	}
}

bool lk::vardata_t::equals(vardata_t &rhs)
{
	if (m_type != rhs.m_type) return false;

	switch(m_type)
	{
	case NULLVAL:
		return true;
	case NUMBER:
		return m_u.v == rhs.m_u.v;
	case STRING:
		return str() == rhs.str();
	case FUNCTION:
		return func() == rhs.func();
	/*case POINTER:
		return pointer() == rhs.pointer();*/
	default:
		return false;
	}

	return false;
}

bool lk::vardata_t::lessthan(vardata_t &rhs)
{
	if (m_type != rhs.m_type) return false;

	switch(m_type)
	{
	case NUMBER:
		return m_u.v < rhs.m_u.v;
	case STRING:
		return str() < rhs.str();
	default:
		return false;
	}

	return false;
}

std::string lk::vardata_t::typestr()
{
	switch(m_type)
	{
	case NULLVAL: return "null";
	case REFERENCE: return "reference";
	case NUMBER: return "number";
	case STRING: return "string";
	case VECTOR: return "vector";
	case HASH: return "hash";
	case FUNCTION: return "function";
	/*case POINTER: return "pointer";*/
	default: return "invalid?";
	}
}

void lk::vardata_t::nullify()
{
	switch( m_type )
	{
	case STRING:
		delete reinterpret_cast<std::string*>(m_u.p);
		break;
	case HASH:
		{
			varhash_t *h = reinterpret_cast<varhash_t*>(m_u.p);
			for (varhash_t::iterator it = h->begin();
				it != h->end();
				++it)
				delete it->second;
			delete h;
		}
		break;
	case VECTOR:
		delete reinterpret_cast<std::vector<vardata_t>*>(m_u.p);
		break;

	// note: functions not deleted here because they 
	// are pointers into the abstract syntax tree
	}

	m_type = NULLVAL;
}

lk::vardata_t &lk::vardata_t::deref() throw (error_t)
{
	vardata_t *p = this;
	while (p->m_type == REFERENCE)
		p = p->ref();

	if (!p) throw error_t("dereference resulted in null target");

	return *p;
}

void lk::vardata_t::assign( double d )
{
	nullify();
	m_type = NUMBER;
	m_u.v = d;
}

void lk::vardata_t::assign( const char *s )
{
	if (m_type != STRING)
	{
		nullify();
		m_type = STRING;
		m_u.p = new std::string(s);
	}
	else
	{
		*reinterpret_cast<std::string*>(m_u.p) = s;
	}
}

/*void lk::vardata_t::assign_pointer( vardata_t *p )
{
	if (m_type != POINTER)
	{
		nullify();
		m_type = POINTER;
	}

	m_u.p = p;
}*/

void lk::vardata_t::assign( const std::string &s )
{
	if (m_type != STRING)
	{
		nullify();
		m_type = STRING;
		m_u.p = new std::string(s);
	}
	else
	{
		*reinterpret_cast<std::string*>(m_u.p) = s;
	}
}

void lk::vardata_t::empty_vector()
{
	nullify();
	m_type = VECTOR;
	m_u.p = new std::vector<vardata_t>;
}

void lk::vardata_t::empty_hash()
{
	nullify();
	m_type = HASH;
	m_u.p = new varhash_t;
}

void lk::vardata_t::assign( const std::string &key, vardata_t *val )
{
	if (m_type != HASH)
	{
		nullify();
		m_type = HASH;
		m_u.p = new varhash_t;
	}

	(*reinterpret_cast< varhash_t* >(m_u.p))[ key ] = val;
}

void lk::vardata_t::unassign( const std::string &key )
{
	if (m_type != HASH) return;

	varhash_t &h = (*reinterpret_cast< varhash_t* >(m_u.p));
			
	varhash_t::iterator it = h.find( key );
	if (it != h.end())
	{
		delete (*it).second; // delete the associated data
		h.erase( it );
	}
}

void lk::vardata_t::assign( node_t *func )
{
	nullify();
	m_type = FUNCTION;
	m_u.p = func;
}

void lk::vardata_t::assign( vardata_t *ref )
{
	nullify();
	m_type = REFERENCE;
	m_u.p = ref;
}


void lk::vardata_t::resize( size_t n )
{
	if ( m_type != VECTOR )
	{
		nullify();
		m_type = VECTOR;
		m_u.p = new std::vector<vardata_t>;
	}

	reinterpret_cast<std::vector<vardata_t>*>(m_u.p)->resize( n );
}

/*lk::vardata_t *lk::vardata_t::pointer() throw(error_t)
{
	if (m_type != POINTER) throw error_t("access violation to non-pointer data");
	return reinterpret_cast<vardata_t*>(m_u.p);
}*/

double &lk::vardata_t::num() throw(error_t)
{
	if ( m_type != NUMBER ) throw error_t("access violation to non-numeric data");
	return m_u.v;
}

std::string &lk::vardata_t::str() throw(error_t)
{
	if ( m_type != STRING ) throw error_t("access violation to non-string data");
	return *reinterpret_cast<std::string*>(m_u.p);
}

const char *lk::vardata_t::cstr() throw(error_t)
{
	if ( m_type != STRING ) throw error_t("access violation to non-string data");
	return reinterpret_cast<std::string*>(m_u.p)->c_str();
}

lk::vardata_t *lk::vardata_t::ref()
{
	if ( m_type != REFERENCE )
		return 0;
	else
		return reinterpret_cast<vardata_t*>(m_u.p);
}

std::vector<lk::vardata_t> *lk::vardata_t::vec() throw(error_t)
{
	if (m_type != VECTOR) throw error_t("access violation to non-array data");
	return reinterpret_cast< std::vector<vardata_t>* >(m_u.p);
}

size_t lk::vardata_t::length()
{
	switch(m_type)
	{
	case VECTOR:
		return reinterpret_cast<std::vector<vardata_t>* >(m_u.p)->size();
	default:
		return 0;
	}
}

lk::node_t *lk::vardata_t::func() throw(error_t)
{
	if (m_type != FUNCTION) throw error_t("access violation to non-function data");
	return reinterpret_cast< node_t* >(m_u.p);
}

lk::varhash_t *lk::vardata_t::hash() throw(error_t)
{
	if ( m_type != HASH ) throw error_t("access violation to non-hash data");
	return reinterpret_cast< varhash_t* > (m_u.p);
}

lk::vardata_t *lk::vardata_t::index(size_t idx) throw(error_t)
{
	if (m_type != VECTOR) throw error_t("access violation to non-array data");
	register std::vector<vardata_t> &m = *reinterpret_cast< std::vector<vardata_t>* >(m_u.p);
	if (idx < 0 || idx >= m.size()) throw error_t("array index out of bounds: %d (len: %d)", (int)idx, (int)m.size());

	return &m[idx];
}

lk::vardata_t *lk::vardata_t::lookup( const std::string &key ) throw(error_t)
{
	if (m_type != HASH) throw error_t("access violation to non-hash data");
	register varhash_t &h = *reinterpret_cast< varhash_t* >(m_u.p);
	varhash_t::iterator it = h.find( key );
	if ( it != h.end() )
		return (*it).second;
	else
		return 0;
}


lk::env_t::env_t() : m_parent(0), m_varIter(m_varHash.begin()) {  }
lk::env_t::env_t(env_t *p) : m_parent(p), m_varIter(m_varHash.begin()) {  }

lk::env_t::~env_t()
{
	clear();

	// delete the referenced objects
	for ( size_t i=0;i<m_objTable.size();i++ )
		if ( m_objTable[i] )
			delete m_objTable[i];
}

void lk::env_t::clear()
{
	for ( varhash_t::iterator it = m_varHash.begin(); it !=m_varHash.end(); ++it )
		delete it->second; // delete the var_data object
	m_varHash.clear();
}

void lk::env_t::assign( const std::string &name, vardata_t *value )
{
	vardata_t *x = lookup(name, false);
			
	if (x && x!=value)
		delete x;

	m_varHash[name] = value;
}

void lk::env_t::unassign( const std::string &name )
{
	varhash_t::iterator it = m_varHash.find( name );
	if (it != m_varHash.end())
	{
		delete (*it).second; // delete the associated data
		m_varHash.erase( it );
	}
}

lk::vardata_t *lk::env_t::lookup( const std::string &name, bool search_hierarchy )
{
	varhash_t::iterator it = m_varHash.find( name );
	if ( it != m_varHash.end() )
		return (*it).second;
	else if (search_hierarchy && m_parent)
		return m_parent->lookup( name, true );
	else
		return 0;
}
		
const char *lk::env_t::first()
{
	m_varIter = m_varHash.begin();
	if (m_varIter != m_varHash.end())
		return m_varIter->first.c_str();
	else
		return 0;
}

const char *lk::env_t::next()
{
	if (m_varIter == m_varHash.end()) return NULL;

	++m_varIter;

	if (m_varIter != m_varHash.end())	return m_varIter->first.c_str();

	return 0;
}

lk::env_t *lk::env_t::parent()
{
	return m_parent;
}

unsigned int lk::env_t::size()
{
	return m_varHash.size();
}


void lk::env_t::register_func( const std::string &name, fcall_t f )
{
	m_funcHash[name] = f;
}

lk::fcall_t lk::env_t::lookup_func( const std::string &name )
{
	funchash_t::iterator it = m_funcHash.find( name );
	if ( it != m_funcHash.end() )
		return (*it).second;
	else if ( m_parent )
		return m_parent->lookup_func( name );
	else
		return 0;
}

size_t lk::env_t::insert_object( objref_t *o )
{
	std::vector< objref_t* >::iterator pos = std::find(m_objTable.begin(), m_objTable.end(), o);
	if ( pos != m_objTable.end())
	{
		return (pos-m_objTable.begin()) + 1;
	}
	else
	{
		m_objTable.push_back( o );
		return m_objTable.size();
	}
}

bool lk::env_t::destroy_object( objref_t *o )
{
	std::vector< objref_t* >::iterator pos = std::find(m_objTable.begin(), m_objTable.end(), o);
	if (pos != m_objTable.end())
	{
		if (*pos != 0)
			delete (*pos);

		m_objTable.erase( pos );
		return true;
	}
	else
		return false;
}

lk::objref_t *lk::env_t::query_object( size_t ref )
{
	ref--;
	if (ref >= 0 && ref < m_objTable.size())
		return m_objTable[ref];
	else
		return 0;
}

/*bool lk::invoke_t::call( const std::string &name, std::vector< vardata_t > &args, vardata_t &result )
{
	vardata_t *f = m_env->lookup(name, true);
	if (!f) return false;

	if ( expr_t *define =


	
}*/
