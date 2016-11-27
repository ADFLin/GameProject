#ifndef CompilerConfig_h__
#define CompilerConfig_h__

#if defined ( _MSC_VER )
#define CPP_COMPILER_MSVC 1
#elif defined ( __GNUC__ )
#define CPP_COMPILER_GCC 1
#else
#error "unknown compiler"
#endif

#endif // CompilerConfig_h__
