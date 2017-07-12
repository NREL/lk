/***********************************************************************************************************************
*  LK, Copyright (c) 2008-2017, Alliance for Sustainable Energy, LLC. All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
*  following conditions are met:
*
*  (1) Redistributions of source code must retain the above copyright notice, this list of conditions and the following
*  disclaimer.
*
*  (2) Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
*  following disclaimer in the documentation and/or other materials provided with the distribution.
*
*  (3) Neither the name of the copyright holder nor the names of any contributors may be used to endorse or promote
*  products derived from this software without specific prior written permission from the respective party.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
*  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER, THE UNITED STATES GOVERNMENT, OR ANY CONTRIBUTORS BE LIABLE FOR
*  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
*  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
*  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************************************************************/

#ifndef __lk_lex_h
#define __lk_lex_h

#include <cstdio>

/*

For proper compilation:

Define _WIN32 if on Windows
Define _DEBUG if compile with debugging

*/

#include <lk/absyn.h>

namespace lk {
	class input_base
	{
	public:
		virtual ~input_base() { }
		virtual char operator*() = 0;
		virtual char peek() = 0;
		virtual char operator++(int) = 0;
	};

	class input_string : public input_base
	{
	protected:
		char *m_buf;
		char *m_p;
		bool allocate(size_t n);
	public:
		input_string();
		input_string(const lk_string &in);
		virtual ~input_string();
		virtual char operator*();
		virtual char operator++(int);
		virtual char peek();
	};

	class input_file : public input_string
	{
	public:
		input_file(const lk_string &file);
		virtual ~input_file();
	};

	class lexer
	{
	public:
		static const char *tokstr(int t);

		enum {
			INVALID,
			END,
			IDENTIFIER,
			SPECIAL,
			NUMBER,
			LITERAL,

			// one character tokens
			SEP_SEMI,
			SEP_COLON,
			SEP_COMMA,
			SEP_LPAREN,
			SEP_RPAREN,
			SEP_LCURLY,
			SEP_RCURLY,
			SEP_LBRACK,
			SEP_RBRACK,

			OP_PLUS,
			OP_MINUS,
			OP_MULT,
			OP_DIV,
			OP_EXP,
			OP_DOT,
			OP_QMARK,
			OP_QMARKAT,
			OP_POUND,
			OP_TILDE,
			OP_PERCENT,
			OP_AT,
			OP_LOGIAND,
			OP_LOGIOR,
			OP_BITAND,
			OP_BITOR,
			OP_BANG,
			OP_ASSIGN,
			OP_REF,
			OP_PP,
			OP_MM,
			OP_LT,
			OP_GT,
			OP_EQ,
			OP_NE,
			OP_LE,
			OP_GE,
			OP_PLUSEQ,
			OP_MINUSEQ,
			OP_MULTEQ,
			OP_DIVEQ,
			OP_MINUSAT
		};

		lexer(input_base &input);

		int next();

		lk_string text();
		double value();

		int line();
		lk_string error();

	private:
		void whitespace();
		bool comments();

		lk_string m_error;
		int m_line;
		lk_string m_buf;
		double m_val;

		input_base &p;
	};
};

#endif
