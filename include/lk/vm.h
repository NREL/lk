/*
BSD 3-Clause License

Copyright (c) Alliance for Sustainable Energy, LLC. See also https://github.com/NREL/lk/blob/develop/LICENSE 

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __lk_vm_h
#define __lk_vm_h

#include <lk/absyn.h>
#include <lk/env.h>

namespace lk {

/**
* \enum Opcode
* Operation codes used by interpreter
*/
    enum Opcode {
        ADD, SUB, MUL, DIV, LT, GT, LE, GE, NE, EQ, INC, DEC, OR, AND, NOT, NEG, EXP,
        PSH, ///< push
        POP, ///< pop
        DUP, NUL, ARG, SWI,
        J, ///< if-elseif-else
        JF, JT, IDX, KEY, MAT, WAT, SET, GET, WR,
        RREF, ///< right-hand reference
        LREF, ///< left-hand reference
        LCREF, ///< left-hand constant reference
        LGREF, ///< left-hand global reference
        FREF, CALL, TCALL, RET, END, SZ, KEYS, TYP, VEC, HASH,
        __MaxOp
    };
    struct OpCodeEntry {
        Opcode op;
        const char *name;
    };
    extern OpCodeEntry op_table[];

/**
* \struct bytecode
*
* Stores instruction stack information. Operation to be done and on which argument is stored in program.
* Constants store variable data types while identifiers are names of variables.
*
*/

    struct bytecode {
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

    class vm {
    public:

/**
* \struct frame
*
*	Stores stack frames for vm, default one contains global variables. Each corresponds
* to a call to a subroutine which has not yet terminated with a return, and contains
* an environment with active functions, stack position, return address of the subroutine's
* caller, arguments to subroutine, and local arguments
*
*/
        struct frame {
            frame(lk::env_t *parent, size_t fptr, size_t ret, size_t na)
                    : env(parent), fp(fptr), retaddr(ret), nargs(na), iarg(0), thiscall(false) {
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
        std::vector<vardata_t> stack;

        bytecode *bc;
        /*
        std::vector< unsigned int > program;
        std::vector< vardata_t > constants;
        std::vector< lk_string > identifiers;
        std::vector< srcpos_t > debuginfo;
        */

        std::vector<frame *> frames;
        std::vector<bool> brkpt; ///< breakpoints for debugging

        lk_string errStr;
        srcpos_t lastbrk;

        void free_frames();

        bool error(const char *fmt, ...);


/// global variable keeping track of number of operation types (47)
#ifdef OP_PROFILE
        size_t opcount[__MaxOp];

        void clear_opcount();

#endif

    public:
        enum ExecMode {
            NORMAL,   ///< run with no debugging
            DEBUG,    ///< run until next breakpoint
            STEP,     ///< step 1 code statement
            SINGLE    ///< step 1 assembly instruction
        };

        vm(size_t ssize = 4096);

        virtual ~vm();

        bool initialize(lk::env_t *env);

        bool run(ExecMode mode = NORMAL);

        lk_string error() { return errStr; }

        virtual bool on_run(const srcpos_t &spos);

        void clrbrk();

        int setbrk(int line, const lk_string &file);

        std::vector<srcpos_t> getbrk();

        size_t get_ip() { return ip; }

        frame **get_frames(size_t *nfrm);

        vardata_t *get_stack(size_t *psp);

        void load(bytecode *b);

        bytecode *get_bytecode() { return bc; }

        virtual bool special_set(const lk_string &name, vardata_t &val);

        virtual bool special_get(const lk_string &name, vardata_t &val);

#ifdef OP_PROFILE

        void get_opcount(size_t iop[__MaxOp]);

#endif

    };

} // namespace lk

#endif
