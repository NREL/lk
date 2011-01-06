#ifndef __lk_eval_h
#define __lk_eval_h


#include <string>
#include "lk_absyn.h"
#include "lk_env.h"

namespace lk {

	class std_io
	{
	public:
		virtual std::string input() = 0;
		virtual void output(const std::string &text) = 0;
	};

	enum { CTL_NONE, CTL_BREAK, CTL_CONTINUE, CTL_RETURN, CTL_EXIT };

	bool eval( node_t *root, 
		env_t *env, 
		std_io *io, 
		vardata_t &result,
		bool env_mutable,
		int &ctl_id);
};

#endif
