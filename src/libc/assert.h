#ifndef ASSERT_H_
#define ASSERT_H_

#include "panic.h"

#define assert(x) \
	if(!(x)) { \
		panic("Kernel assertion failed at " __FILE__ ":%d: " #x "\n", __LINE__); \
	}

#endif	/* ASSERT_H_ */
