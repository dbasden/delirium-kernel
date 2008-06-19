#ifndef __ASSERT_H
#define __ASSERT_H

#include <delirium.h>
#include "klib.h"

#define __MACRO_TO_STRING(_sval) __VAL_TO_STRING(_sval)
#define __VAL_TO_STRING(_sval) #_sval
#define ASSERT_MSG	" @ line " __MACRO_TO_STRING(__LINE__) " from " \
			__MACRO_TO_STRING(__FILE__) "\n"

#ifdef ENABLE_ASSERTS
#ifdef KERNEL
#define Assert(aexp)	((aexp) ? : kpanic("\nFailed assertion ("#aexp")"ASSERT_MSG))
#endif
#ifndef KERNEL
#define Assert(aexp)	((aexp) ? : (print("\nFailed assertion ("#aexp")"ASSERT_MSG), kill_current_thread()))
#endif
#else
#define	Assert(aexp)
#endif
#define assert	Assert

#endif
