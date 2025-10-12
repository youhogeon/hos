#ifndef __SWITCHCONTEXT_H__
#define __SWITCHCONTEXT_H__

#include "tcbpool.h"

void kSwitchContext(CONTEXT* pstCurrentContext, CONTEXT* pstNextContext);

#endif /*__SWITCHCONTEXT_H__*/
