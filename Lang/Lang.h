#pragma warning(push)
#pragma warning(disable: 4005)

#define English 0
#define Chinese 1

#include"en.h"

#if (LANGUAGE == Chinese)
#include"zh.h"
#endif


#pragma warning(pop)