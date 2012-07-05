#ifndef __lk_eval_h
#define __lk_eval_h

#include "lk_absyn.h"
#include "lk_env.h"

namespace lk {

	enum { CTL_NONE, CTL_BREAK, CTL_CONTINUE, CTL_RETURN, CTL_EXIT };

	bool eval( node_t *root, 
		env_t *env, 
		std::vector< lk_string > &errors, 
		vardata_t &result,
		unsigned int flags, /* normally 0 */
		unsigned int &ctl_id,
		bool (*cb_func)(int line, void *data),
		void *cb_data );
};

#endif
