#ifndef CompilerConfig_h__
#define CompilerConfig_h__

#if defined ( _MSC_VER )
#define CPP_COMPILER_MSVC 1
#define FORCEINLINE __forceinline 
#define DLLEXPORT __declspec( dllexport )
#define DLLIMPORT __declspec( dllimport )

#elif defined ( __GNUC__ )
#define CPP_COMPILER_GCC 1
#define FORCEINLINE __attribute__((always_inline))
#define DLLEXPORT __attribute__((visibility("default")))
#define DLLIMPORT
#else
#error "unknown compiler"
#endif

#ifndef CPP_COMPILER_MSVC
#define CPP_COMPILER_MSVC 0
#endif // !CPP_COMPILER_MSVC

#ifndef CPP_COMPILER_GCC
#define CPP_COMPILER_GCC 0
#endif


#endif // CompilerConfig_h__
