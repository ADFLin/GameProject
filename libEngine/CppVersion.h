#ifndef CppVersion_h__
#define CppVersion_h__

#include "CompilerConfig.h"
#include "MarcoCommon.h"

#ifdef CPP_COMPILER_MSVC
#	if ( _MSC_VER >= 1914 ) 
#		define CPP_CHARCONV_SUPPORT 1
#   endif
#	if ( _MSC_VER >= 1700 ) 
#		define CPP_RVALUE_REFENCE_SUPPORT 1
#	    define CPP_VARIADIC_TEMPLATE_SUPPORT 1
#       define CPP_11_STDLIB_SUPPORT 1
#       define CPP_STATIC_ASSERT_SUPPORT 1
#       define CPP_TYPE_TRAITS_SUPPORT 1
#       define CPP_FUNCTION_DELETE_SUPPORT 1
#       define CPP_USING_TYPE_ALIAS_SUPPORT 1 
#   endif
#	if ( _MSC_VER >= 1600 ) 
#		define CPP_11_KEYWORD_SUPPORT 1
#   endif
#	if ( _MSC_VER >= 1500 ) 
#       define CPP_TR1_SUPPORT 1
#   endif
#endif

#ifndef CPP_RVALUE_REFENCE_SUPPORT
#define CPP_RVALUE_REFENCE_SUPPORT 0
#endif

#ifndef CPP_VARIADIC_TEMPLATE_SUPPORT
#define CPP_VARIADIC_TEMPLATE_SUPPORT 0
#endif

#ifndef CPP_11_KEYWORD_SUPPORT
#define CPP_11_KEYWORD_SUPPORT 0
#endif

#ifndef CPP_11_STDLIB_SUPPORT
#define CPP_11_STDLIB_SUPPORT 0
#endif

#ifndef CPP_STATIC_ASSERT_SUPPORT
#define CPP_STATIC_ASSERT_SUPPORT 0
#endif

#ifndef CPP_TR1_SUPPORT
#define CPP_TR1_SUPPORT 0
#endif

#ifndef CPP_TYPE_TRAITS_SUPPORT
#define CPP_TYPE_TRAITS_SUPPORT 0
#endif

#ifndef CPP_FUNCTION_DELETE_SUPPORT
#define CPP_FUNCTION_DELETE_SUPPORT 0
#endif

#ifndef CPP_USING_TYPE_ALIAS_SUPPORT
#define  CPP_USING_TYPE_ALIAS_SUPPORT 0
#endif

#ifndef CPP_CHARCONV_SUPPORT
#define CPP_CHARCONV_SUPPORT 0
#endif

#if !CPP_11_KEYWORD_SUPPORT
#	define override
#	define final
#	define nullptr 0
#endif

#if CPP_FUNCTION_DELETE_SUPPORT
#	define FUNCTION_DELETE( FUN ) FUN = delete;
#else
#	define FUNCTION_DELETE( FUN ) private: FUN {}
#endif

namespace CppVerPriv
{
	template < bool expr >
	struct StaticAssertSize { enum { Size = 0 }; };
	template <>
	struct StaticAssertSize< true > { enum { Size = 1 }; };
}

//#TODO
#if !CPP_STATIC_ASSERT_SUPPORT
#	define static_assert( EXPR , MSG )\
	typedef int ( ANONYMOUS_VARIABLE( STATIC_ASSERT_ )[ ::CppVerPriv::StaticAssertSize< EXPR >::Size ] );
#endif


#endif // CppVersion_h__
