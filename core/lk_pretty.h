#ifndef __lk_pretty_h
#define __lk_pretty_h

#include <string>
#include "lk_absyn.h"

namespace lk {
	
	void pretty_print( std::string &str, node_t *root, int level );

};

#endif
