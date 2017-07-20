#ifndef __lk_vm_h
#define __lk_vm_h

#include <lk/absyn.h>
#include <lk/env.h>

namespace lk {

/// Opcodes: J, if-elseif-else; PSH, push onto stack; RREF LREF LCREF LGREF, identifers
/**
* \struct Opcode
*
* 
*
*
*/
enum Opcode {
	ADD, SUB, MUL, DIV, LT, GT, LE, GE, NE, EQ, INC, DEC, OR, AND, NOT, NEG, EXP, PSH, POP, DUP, NUL, ARG, SWI,
	J, JF, JT, IDX, KEY, MAT, WAT, SET, GET, WR, RREF, LREF, LCREF, LGREF, FREF, CALL, TCALL, RET, END, SZ, KEYS, TYP, VEC, HASH,
	__MaxOp };
struct OpCodeEntry { Opcode op; const char *name; };
extern OpCodeEntry op_table[];

/**
* \sruct bytecode
*
* Stores instruction stack information. Operation to be done and on which argument is stored in program. 
* Constants store variable data types while identifiers are names of variables.
*
*/

struct bytecode
{
	std::vector<unsigned int> program;
	std::vector<vardata_t> constants;
	std::vector<lk_string> identifiers;
	std::vector<srcpos_t> debuginfo;
};

#define OP_PROFILE 1 

// takes bytecode as input

/**
* \class vm
*
*	
*
*/

class vm
{
public:

/**
* \class frame
*
*	Stores stack frames for vm, default one contains global variables. Each corresponds 
* to a call to a subroutine which has not yet terminated with a return, and contains
* an environment with active functions, stack position, return address of the subroutine's
* caller, arguments to subroutine, and local arguments
*
*/
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
		lk_string id;
	};

private:
	size_t ip;
	int sp; ///< stack size, use int so that values can go negative and errors easier to catch rather than wrapping around to a large number
	std::vector< vardata_t > stack;

	bytecode *bc;
	/*
	std::vector< unsigned int > program;
	std::vector< vardata_t > constants;
	std::vector< lk_string > identifiers;
	std::vector< srcpos_t > debuginfo;
	*/

	std::vector< frame* > frames;
	std::vector< bool > brkpt; ///< breakpoints for debugging

	lk_string errStr;
	srcpos_t lastbrk;

	void free_frames();
	bool error( const char *fmt, ... );


/// global variable keeping track of number of operation types (47)
#ifdef OP_PROFILE
	size_t opcount[__MaxOp];
	void clear_opcount();
#endif

public:
	enum ExecMode { 
		NORMAL,   // run with no debugging
		DEBUG,    // run until next breakpoint
		STEP,     // step 1 code statement
		SINGLE    // step 1 assembly instruction
	};

	vm( size_t ssize = 4096 );
	virtual ~vm();
	
	bool initialize( lk::env_t *env );
	bool run( ExecMode mode = NORMAL );
	lk_string error() { return errStr; }
	virtual bool on_run( const srcpos_t &spos);

	void clrbrk();
	int setbrk( int line, const lk_string &file );
	std::vector<srcpos_t> getbrk();

	size_t get_ip() { return ip; }
	frame **get_frames( size_t *nfrm );
	vardata_t *get_stack( size_t *psp );

	void load( bytecode *b );
	bytecode *get_bytecode() { return bc; }

	virtual bool special_set( const lk_string &name, vardata_t &val );
	virtual bool special_get( const lk_string &name, vardata_t &val );
	
#ifdef OP_PROFILE
	void get_opcount( size_t iop[__MaxOp] );
#endif

};

} // namespace lk

#endif
