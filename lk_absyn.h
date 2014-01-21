#ifndef __lk_absyn_h
#define __lk_absyn_h

#include <vector>

#ifdef _MSC_VER
#include <unordered_map>
using std::tr1::unordered_map;
#pragma warning(disable: 4290)  // ignore warning: 'C++ exception specification ignored except to indicate a function is not __declspec(nothrow)'
#else
#include <tr1/unordered_map>
using std::tr1::unordered_map;
#endif


#if defined(LK_USE_WXWIDGETS)
#include <wx/string.h>
#include <wx/hashmap.h>

#if wxCHECK_VERSION(2,9,0)
typedef wxUniChar lk_char;
#define LK_UNICODE 1
#else
typedef wxChar lk_char;
#endif
typedef wxString lk_string;

typedef wxStringHash lk_string_hash;
typedef wxStringEqual lk_string_equal;

#else
#include <string>

typedef std::string::value_type lk_char;
typedef std::string lk_string;

#ifdef _MSC_VER
typedef std::hash<std::string> lk_string_hash;
#else
typedef std::tr1::hash<std::string> lk_string_hash;
#endif
typedef std::equal_to<std::string> lk_string_equal;

#endif

namespace lk
{
	lk_char lower_char( lk_char c );
	lk_char upper_char( lk_char c );
	bool convert_integer( const lk_string &str, int *x );
	bool convert_double( const lk_string &str, double *x );
	std::string to_utf8( const lk_string &str );
	lk_string from_utf8( const std::string &str );
	lk_string from_utf8( const char *str );
	lk_string to_string( lk_char c );

	extern int _node_alloc;
	
	class attr_t
	{
	public:
		virtual ~attr_t() {  };
	};

	class node_t
	{
	private:
		int m_line;
	public:
		attr_t *attr;
		node_t(int ln) : m_line(ln), attr(0) { _node_alloc++; }
		virtual ~node_t() { _node_alloc--; if (attr) delete attr; }
		inline int line() { return m_line; }
	};
	
	class list_t : public node_t
	{
	public:
		node_t *item;
		list_t *next;
		list_t( int line, node_t *i, list_t *n ) : node_t(line), item(i), next(n) {  };
		virtual ~list_t() { if (item) delete item; if (next) delete next; }
	};
					
	class iter_t : public node_t
	{
	public:
		node_t *init, *test, *adv, *block;
		iter_t( int line, node_t *i, node_t *t, node_t *a, node_t *b ) : node_t(line), init(i), test(t), adv(a), block(b) { }
		virtual ~iter_t() { if (init) delete init; if (test) delete test; if (adv) delete adv; if (block) delete block; }
	};
	
	class cond_t : public node_t
	{
	public:
		node_t *test, *on_true, *on_false;
		cond_t(int line, node_t *t, node_t *ot, node_t *of) : node_t(line), test(t), on_true(ot), on_false(of) { }
		virtual ~cond_t() { if (test) delete test; if (on_true) delete on_true; if (on_false) delete on_false; }
	};
		
	class expr_t : public node_t
	{
	public:
		enum {
			INVALID,
			
			PLUS,
			MINUS,
			MULT,
			DIV,
			INCR,
			DECR,
			DEFINE,
			ASSIGN,
			LOGIOR,
			LOGIAND,
			NOT,
			EQ,
			NE,
			LT,
			LE,
			GT,
			GE,
			EXP,
			NEG,
			INDEX,
			HASH,
			CALL,
			THISCALL,
			RETURN,
			EXIT,
			BREAK,
			CONTINUE,
			SIZEOF,
			KEYSOF,
			TYPEOF,
			INITVEC,
			INITHASH,
			SWITCH
		};
		int oper;
		node_t *left, *right;
		const char *operstr();
		expr_t(int line, int op, node_t *l, node_t *r) : node_t(line), oper(op), left(l), right(r) {  }
		virtual ~expr_t() { if (left) delete left; if (right) delete right; }
	};
				
	class iden_t : public node_t
	{
	public:
		lk_string name;
		bool common;
		bool constval;
		bool special;
		iden_t(int line, const lk_string &n, bool com, bool cons, bool speci) : node_t(line), name(n), common(com), constval(cons), special(speci) {  }
		virtual ~iden_t() { }
	};
				
	class constant_t : public node_t
	{
	public:
		double value;
		constant_t(int line, double v) : node_t(line), value(v) {  }
		virtual ~constant_t() {  }
	};
	
	class literal_t : public node_t
	{
	public:
		lk_string value;
		literal_t(int line, const lk_string &s) : node_t(line), value(s) {  }
		virtual ~literal_t() {  }
	};

	class null_t : public node_t
	{
	public:
		null_t( int line ) : node_t(line) {  }
		virtual ~null_t() {  }
	};

	void pretty_print( lk_string &str, node_t *root, int level );
};

#endif
