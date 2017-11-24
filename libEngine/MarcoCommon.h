#pragma once
#ifndef MarcoCommon_H_5F8ACEA8_F34E_4CAA_9C04_F663367A6F07
#define MarcoCommon_H_5F8ACEA8_F34E_4CAA_9C04_F663367A6F07

#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif 

template< class T, size_t N >
size_t array_size(T(&ar)[N]) { return N; }
#define ARRAY_SIZE( var ) array_size( var )

#define MAKE_STRING_INNER(A) #A
#define MAKE_STRING(A) MAKE_STRING_INNER(A)

#define MARCO_NAME_COMBINE_2_INNER(s1, s2) s1##s2
#define MARCO_NAME_COMBINE_2(s1, s2) MARCO_NAME_COMBINE_2_INNER(s1, s2)

#define MARCO_NAME_COMBINE_3_INNER(s1, s2 ,s3) s1##s2##s3
#define MARCO_NAME_COMBINE_3(s1, s2 , s3) MARCO_NAME_COMBINE_3_INNER(s1, s2, s3)

#ifdef _MSC_VER // Necessary for edit & continue in MS Visual C++.
# define ANONYMOUS_VARIABLE(str) MARCO_NAME_COMBINE_2(str, __COUNTER__)
#else
# define ANONYMOUS_VARIABLE(str) MARCO_NAME_COMBINE_2(str, __LINE__)
#endif 

#define MAKE_VERSION( a , b , c ) ( ( unsigned char(a)<< 24 ) | ( unsigned char(b)<< 16 ) | unsigned short(c) )


#define CHECK( EXPR ) assert( EXPR )
#define NEVER_REACH( STR ) assert( !STR )

#endif // MarcoCommon_H_5F8ACEA8_F34E_4CAA_9C04_F663367A6F07
