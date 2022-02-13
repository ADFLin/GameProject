
#include "CompilerConfig.h"

#if CPP_COMPILER_MSVC
#include <string.h>

#elif CPP_COMPILER_GCC

#define __STDC_WANT_LIB_EXT1__ 1
#include <string.h>
#include <cstdio>

#define strtok_s strtok_r
typedef int errno_t;
errno_t strcpy_s(char *dest, size_t destsz, const char *src)
{
    strcpy(dest , src );
    return 0;
}

#define sscanf_s sscanf

char *strtok_r(char *str, const char *delim, char **save)
{
    char *res, *last;

    if( !save )
        return strtok(str, delim);
    if( !str && !(str = *save) )
        return NULL;
    last = str + strlen(str);
    if( (*save = res = strtok(str, delim)) )
    {
        *save += strlen(res);
        if( *save < last )
            (*save)++;
        else
            *save = NULL;
    }
    return res;
}

errno_t gmtime_s(time_t const* const _Time , struct tm* const _Tm )
{
	gmtime_r( _Time , _Tm );
	return 0;
}


errno_t localtime_s(struct tm* const _Tm, time_t const* const _Time)
{
	localtime_r(_Time, _Tm);
	return 0;
}
#elif CPP_COMPILER_CLANG

#define strtok_s strtok_r

#endif
