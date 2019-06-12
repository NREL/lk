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

#ifndef __lk_eval_h
#define __lk_eval_h

#include <lk/absyn.h>
#include <lk/env.h>

namespace lk {
	enum { CTL_NONE, CTL_BREAK, CTL_CONTINUE, CTL_RETURN, CTL_EXIT };

	class eval
	{
		lk::node_t *m_tree;
		lk::env_t m_localEnv;
		lk::env_t *m_env;
		vardata_t m_result;
		std::vector< lk_string > m_errors;

	public:
		eval(lk::node_t *tree);
		eval(lk::node_t *tree, lk::env_t *env);
		virtual ~eval();

		bool run();

		virtual bool special_set(const lk_string &name, vardata_t &val);
		virtual bool special_get(const lk_string &name, vardata_t &val);
		inline virtual bool on_run(int /* line */) { return true; }

		std::vector<lk_string> &errors() { return m_errors; }
		size_t error_count() { return m_errors.size(); }
		lk_string get_error(size_t i) { if (i < m_errors.size()) return m_errors[i]; else return lk_string(""); }

		lk::env_t &env() { return *m_env; }
		vardata_t &result() { return m_result; }

	protected:
		virtual bool interpret(node_t *root,
			lk::env_t *cur_env,
			vardata_t &result,
			unsigned int flags, /* normally 0 */
			unsigned int &ctl_id);

		bool do_op_eq(void(*oper)(lk::vardata_t &, lk::vardata_t &),
			lk::expr_t *n, lk::env_t *cur_env, unsigned int &flags, unsigned int &ctl_id,
			lk::vardata_t &result, lk::vardata_t &l, lk::vardata_t &r);
	};
};

#endif
