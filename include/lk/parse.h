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

#ifndef __lk_parse_h
#define __lk_parse_h

#include <vector>
#include <lk/lex.h>
#include <lk/absyn.h>

namespace lk
{
	class parser
	{
	public:
		parser(input_base &input, const lk_string &name = "");

		void add_search_path(const lk_string &path) { m_searchPaths.push_back(path); }
		void add_search_paths(const std::vector<lk_string> &paths);
		std::vector<lk_string> get_search_paths() const { return m_searchPaths; }

		node_t *script();
		node_t *block();
		node_t *statement();
		node_t *test();
		node_t *enumerate();
		node_t *loop();
		node_t *define();
		node_t *assignment();
		node_t *ternary();
		node_t *logicalor();
		node_t *logicaland();
		node_t *equality();
		node_t *relational();
		node_t *additive();
		node_t *multiplicative();
		node_t *exponential();
		node_t *unary();
		node_t *postfix();
		node_t *primary();

		srcpos_t srcpos();
		int line() { return lex.line(); }
		int error_count() { return m_errorList.size(); }
		lk_string error(int idx, int *line = 0);

		int token();
		bool token(int t);

		void skip();
		bool match(int t);
		bool match(const char *s);

	private:
		list_t *ternarylist(int septok, int endtok);
		list_t *identifierlist(int septok, int endtok);

		void error(const lk_string &s);
		void error(const char *fmt, ...);

		lexer lex;
		int m_tokType;
		int m_lastLine;
		int m_lastStmt;
		int m_lastBlockEnd;
		lk_string m_lexError;
		bool m_haltFlag;
		struct errinfo { int line; lk_string text; };
		std::vector<errinfo> m_errorList;
		std::vector< lk_string > m_importNameList, m_searchPaths;
		lk_string m_name;
	};
};

#endif
