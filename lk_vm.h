#ifndef __lk_vm_h
#define __lk_vm_h

#include "lk_absyn.h"
#include "lk_env.h"

namespace lk {

enum Opcode {
	ADD, SUB, MUL, DIV, LT, GT, LE, GE, NE, EQ, INC, DEC, OR, AND, NOT, NEG, EXP, PSH, POP, DUP, NUL, ARG,
	J, JF, JT, IDX, KEY, MAT, WAT, SET, GET, WR, RREF, NREF, CREF, FREF, CALL, TCALL, RET, END, SZ, KEYS, TYP, VEC, HASH,
	__MaxOp };

struct OpCodeEntry { Opcode op; const char *name; };
extern OpCodeEntry op_table[];

#define OP_PROFILE 1 

class vm
{
public:
	struct frame {
		frame( lk::env_t *parent, size_t fptr, size_t ret, size_t na )
			: env( parent), fp(fptr), retaddr(ret), nargs(na), iarg(0), thiscall( false )
		{
		}

		lk::env_t env;
		size_t fp;
		size_t retaddr;
		size_t nargs;
		size_t iarg;
		bool thiscall;
	};

private:
	size_t ip, sp;
	std::vector< vardata_t > stack;
	std::vector< unsigned int > program;
	std::vector< vardata_t > constants;
	std::vector< lk_string > identifiers;
	std::vector< srcpos_t > debuginfo;
	std::vector< frame* > frames;
	std::vector< bool > brkpt;

	lk_string errStr;
	srcpos_t lastbrk;

	void free_frames();
	bool error( const char *fmt, ... );

#ifdef OP_PROFILE
	size_t opcount[__MaxOp];
	void clear_opcount();
#endif

public:
	enum ExecMode { NORMAL, DEBUG_RUN, DEBUG_STEP, SINGLE };

	vm( size_t ssize = 2048 );
	virtual ~vm();
	
	void initialize( lk::env_t *env );
	bool run( ExecMode mode = NORMAL );
	lk_string error() { return errStr; }
	virtual bool on_run( const srcpos_t &spos);

	void clrbrk();
	int setbrk( int line );

	size_t get_ip() { return ip; }
	frame **get_frames( size_t *nfrm );
	vardata_t *get_stack( size_t *psp );

	const std::vector<unsigned int> &get_program() { return program; }
	const std::vector<vardata_t> &get_constants() { return constants; }
	const std::vector<lk_string> &get_identifiers() { return identifiers; }
	const std::vector<srcpos_t> &get_debuginfo() { return debuginfo; }

	void load( const std::vector<unsigned int> &code,
		const std::vector<vardata_t> &cnstvals,
		const std::vector<lk_string> &ids,
		const std::vector<lk::srcpos_t> &dbginf);

	virtual bool special_set( const lk_string &name, vardata_t &val );
	virtual bool special_get( const lk_string &name, vardata_t &val );
	
#ifdef OP_PROFILE
	void get_opcount( size_t iop[__MaxOp] );
#endif

};

} // namespace lk

#endif
