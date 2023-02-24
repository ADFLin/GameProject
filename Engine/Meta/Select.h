#pragma once
#ifndef Select_H_3D6BCAD1_C131_4E69_9274_D27DDB1A564F
#define Select_H_3D6BCAD1_C131_4E69_9274_D27DDB1A564F

template < bool, class T1, class T2 >
struct TSelect { typedef T1 Type; };

template < class T1, class T2 >
struct TSelect< false, T1, T2 > { typedef T2 Type; };

template < bool, int N1, int N2 >
struct TSelectValue { enum { Value = N1 }; };

template < int N1, int N2 >
struct TSelectValue< false, N1, N2 > { enum { Value = N2 }; };


#endif // Select_H_3D6BCAD1_C131_4E69_9274_D27DDB1A564F
