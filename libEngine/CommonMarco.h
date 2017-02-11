#pragma once
#ifndef CommonMarco_h__
#define CommonMarco_h__

#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif 

template< class T, size_t N >
size_t array_size(T(&ar)[N]) { return N; }
#define ARRAY_SIZE( var ) array_size( var )

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

#endif // CommonMarco_h__