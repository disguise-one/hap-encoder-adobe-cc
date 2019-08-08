#ifndef _FNDObjC_h_
#define _FNDObjC_h_

#ifndef FOUNDATION_OBJC_PREFIX
#error FOUNDATION_OBJC_PREFIX must be defined
#endif

#define FND_CONCAT(x, y) FND_CONCAT_EXPANDED(x, y)
#define FND_CONCAT_EXPANDED(x, y) x ## y
#define FND_OBJC(c) FND_CONCAT(FOUNDATION_OBJC_PREFIX, c)

#endif
