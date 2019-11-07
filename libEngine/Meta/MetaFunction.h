#ifndef MetaFunction_hpp__
#define MetaFunction_h__

#include "MetaBase.h"

namespace Meta{

	template < class T >
	struct IsMemberFunPointer : HaveResult< false >{};

	//template < class RT >
	//struct FunTraits
	//{
	//	enum { ParamNum = 0 };
	//};

	//template < class BTraits >
	//struct FunTraitsGen : BTraits 
	//{
	//	enum { ParamNum = BTraits::ParamNum + 1 };
	//};

	//template < class RT >
	//struct FunTraitsGen < FunTraits< RT > >
	//{

	//};

	//template < class RT , class P1  >
	//struct FunTraits< RT , P1 > : FunTraitsGen< RT >
	//{
	//	typedef P1 Param1; 
	//};

	//template < class RT , class P1 , class P2 >
	//struct FunTraits< RT , P1 , P2 > : FunTraitsGen< RT , P1 >
	//{
	//	typedef P2 Param2; 
	//};

	//template < class RT , class P1 , class P2 , class P3 >
	//struct FunTraits< RT , P1 , P2 , P3 >  : FunTraitsGen<  RT , P1 , P2  >
	//{
	//	typedef P3 Param3; 
	//};


	template < class T , class RT >
	struct IsMemberFunPointer< RT (T::*)() > : HaveResult< true >{};
	template < class T , class RT , class ...Args >
	struct IsMemberFunPointer< RT (T::*)( Args... ) > : HaveResult< true >{};
	
	template < class T , int Index >
	struct GetArgType {};





}//namespace Meta

#endif // MetaFunction_h__