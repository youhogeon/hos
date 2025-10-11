#ifndef __SWITCHCONTEXT_H__
#define __SWITCHCONTEXT_H__

#include "task.h"

void kSwitchContext(CONTEXT* pstCurrentContext, CONTEXT* pstNextContext);

#endif /*__SWITCHCONTEXT_H__*/
