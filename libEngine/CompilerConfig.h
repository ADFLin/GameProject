#ifndef CompilerConfig_h__
#define CompilerConfig_h__

#if defined ( _MSC_VER )
#define CPP_COMPILER_MSVC 1
#elif defined ( __GNUC__ )
#define CPP_COMPILER_GCC 1
#elif defined (__clang__)
#define CPP_COMPILER_CLANG 1
#else
#error "unknown compiler"
#endif

#ifndef CPP_COMPILER_MSVC
#define CPP_COMPILER_MSVC 0
#endif

#ifndef CPP_COMPILER_GCC
#define CPP_COMPILER_GCC 0
#endif

#ifndef CPP_COMPILER_CLANG
#define CPP_COMPILER_CLANG 0
#endif


#if CPP_COMPILER_MSVC

#define FORCEINLINE __forceinline 
#define DLLEXPORT __declspec( dllexport )
#define DLLIMPORT __declspec( dllimport )
#define RESTRICT __restrict
#define NOINLINE __declspec(noinline)

#define ASSUME_BUILTIN(EXPR)  __assume(EXPR)

#define STD_EXCEPTION_CONSTRUCTOR_WITH_WHAT( CLASSNAME )\
	CLASSNAME(char const* what) :std::exception(what) {}
#elif CPP_COMPILER_GCC

#define FORCEINLINE __attribute__((always_inline))
#define DLLEXPORT __attribute__((visibility("default")))
#define DLLIMPORT
#define RESTRICT __restrict__
#define NOINLINE __attribute__((noinline))

#define ASSUME_BUILTIN(EXPR) { if (!EXPR) __builtin_unreachable(); }

#define STD_EXCEPTION_CONSTRUCTOR_WITH_WHAT( CLASSNAME )\
	CLASSNAME(char const* what) :mWhat(what) {}\
	virtual char const* what() const { return mWhat; }\
	char const* mWhat;

#elif CPP_COMPILER_CLANG

#define FORCEINLINE __attribute__((always_inline))
#define DLLEXPORT __attribute__((visibility("default")))
#define DLLIMPORT
#define RESTRICT __restrict__
#define NOINLINE __attribute__((noinline))

#define ASSUME_BUILTIN(EXPR) __builtin_assume(EXPR)
#endif

#endif // CompilerConfig_h__
