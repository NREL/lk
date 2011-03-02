#ifndef __lk_var_h
#define __lk_var_h

#include <string>
#include <vector>
#include <cstdio>
#include <cstdarg>
#include <exception>

#ifdef _MSC_VER
#include <unordered_map>
using std::tr1::unordered_map;
#pragma warning(disable: 4290)  // ignore warning: 'C++ exception specification ignored except to indicate a function is not __declspec(nothrow)'
#else
#include <tr1/unordered_map>
using std::tr1::unordered_map;
#endif

#include "lk_absyn.h"

namespace lk {

	class vardata_t;
	typedef unordered_map< std::string, vardata_t* > varhash_t;
		
	
	class error_t : public std::exception
	{
	public:
		error_t() : text("general data exception") {  }
		error_t(const char *fmt, ...) {
			char buf[512];
			va_list args;
			va_start( args, fmt );
			vsprintf( buf, fmt, args );
			va_end( args );
			text = buf;
		}
		error_t(const std::string &t) : text(t) {  }
		std::string text;
		virtual const char *what() const { return text.c_str(); }
	};

	class vardata_t
	{
	private:
		unsigned char m_type;
		union {
			void *p;
			double v;
		} m_u;

	public:

		enum {
			NULLVAL,
			REFERENCE,
			NUMBER,
			STRING,
			VECTOR,
			HASH,
			FUNCTION/*,
			POINTER*/
		};

		explicit vardata_t();
		explicit vardata_t( const vardata_t &cp ) { copy( const_cast<vardata_t&>(cp) ); }
		~vardata_t();

		static const double nanval;

		inline unsigned char type() { return m_type; }
		std::string typestr();

		void nullify();
		bool as_boolean();
		std::string as_string();
		double as_number();

		bool copy( vardata_t &rhs );
		bool equals( vardata_t &rhs );
		bool lessthan( vardata_t &rhs );

		
		vardata_t &operator=(const vardata_t &rhs)
		{
			copy( const_cast<vardata_t&>(rhs) );
			return *this;
		}

		vardata_t &deref() throw (error_t);
		
		void assign( double d );
		void assign( const char *s );
		void assign( const std::string &s );
		//void assign_pointer( vardata_t *p );
		void empty_vector();
		void empty_hash();
		void assign( const std::string &key, vardata_t *val );
		void unassign( const std::string &key );
		void assign( node_t *func ); // takes ownership
		void assign( vardata_t *ref );
		void resize( size_t n );

		vardata_t *ref();
		//vardata_t *pointer() throw (error_t);
		double &num() throw(error_t);
		std::string &str() throw(error_t);
		const char *cstr() throw(error_t);
		node_t *func() throw(error_t);
		vardata_t *index(size_t idx) throw(error_t);
		size_t length();
		vardata_t *lookup( const std::string &key ) throw(error_t);

		std::vector<vardata_t> *vec() throw(error_t);
		varhash_t *hash() throw(error_t);
	};
	
	class objref_t
	{
	public:
		virtual ~objref_t() {  }
		virtual std::string type_name() = 0;
	};
	
	class env_t;

	class invoke_t
	{
	private:
		int m_mode;
		std::string m_funcName;
		env_t *m_env;
		vardata_t &m_resultVal;
		std::vector< vardata_t > m_argList;

		std::string m_error;
		bool m_hasError;
		
	public:
		enum { CALL, INFO };

		invoke_t( int mode, const std::string &n, env_t *e, vardata_t &result) 
			: m_mode(mode), m_funcName(n), m_env(e), m_hasError(false), m_resultVal(result) { }

		env_t *env() { return m_env; }
		std::string name() { return m_funcName; }
		vardata_t &result() { return m_resultVal; }

		std::vector< vardata_t > &arg_list() { return m_argList; }
		size_t arg_count() { return m_argList.size(); }
		vardata_t &arg(size_t idx) throw( error_t ) {
			if (idx < m_argList.size())	return m_argList[idx].deref();
			else throw error_t( "invalid access to function argument %d, only %d given", idx, m_argList.size());
		}

		//bool call( const std::string &name, std::vector< vardata_t > &args, vardata_t &result );

		void error( const std::string &text ) { m_error = text; m_hasError = true; }
		std::string error() { return m_error; }
		bool has_error() { return m_hasError; }
	};

	typedef void (*fcall_t)( lk::invoke_t& );
	typedef unordered_map< std::string, fcall_t > funchash_t;
	
	class env_t
	{
	private:
		varhash_t m_varHash;
		varhash_t::iterator m_varIter;

		funchash_t m_funcHash;
		std::vector< objref_t* > m_objTable;
	
		env_t *m_parent;		
	public:
		env_t();
		env_t(env_t *p);
		~env_t();

		void clear();
		void assign( const std::string &name, vardata_t *value );
		void unassign( const std::string &name );
		vardata_t *lookup( const std::string &name, bool search_hierarchy);
		const char *first();
		const char *next();
		unsigned int size();
		env_t *parent();
		
		void register_func( const std::string &name, fcall_t f );
		fcall_t lookup_func( const std::string &name );

		size_t insert_object( objref_t *o );
		bool destroy_object( objref_t *o );
		objref_t *query_object( size_t ref );
		
	};

}; // namespace lk

#endif

