#pragma once
#ifndef ReflectionCollect_H_990CAAB9_A6AF_48D5_B6A2_79DDB78AE3DD
#define ReflectionCollect_H_990CAAB9_A6AF_48D5_B6A2_79DDB78AE3DD

#include "Template/ArrayView.h"

class ReflectionCollector
{
	template< class T >
	void beginClass(char const* name);

	template< typename T , typename Base >
	void addBaseClass();

	template< class T, typename P >
	void addProperty(P(T::*memberPtr), char const* name);

	void endClass();
};


struct ReflectEnumValueInfo
{
	char const* text;
	uint64 value;
};
template<typename T>
struct TReflectEnumValueTraits
{

};

#define REFLECT_STRUCT_BEGIN( CLASS )\
	template< class ReflectionCollector >\
	static void CollectReflection(ReflectionCollector& collector)\
	{\
		using ThisClass = CLASS;\
		collector.beginClass< ThisClass >( #CLASS );

#define REF_BASE_CLASS( BASE )\
		collector.addBaseClass< ThisClass , BASE >();

#define REF_PROPERTY( VAR )\
		collector.addProperty( &ThisClass::VAR , #VAR );

#define REFLECT_STRUCT_END()\
		collector.endClass();\
	}

#define REF_ENUM_BEGIN( NAME )\
	template<>\
	struct ::TReflectEnumValueTraits< NAME >\
	{\
		using EnumType = NAME;\
		static TArrayView< ReflectEnumValueInfo const > GetValues()\
		{\
			static ReflectEnumValueInfo const StaticValues[] =\
			{

#define REF_ENUM( VALUE ) { #VALUE , (uint64)EnumType::VALUE },


#define REF_ENUM_END()\
			};\
			return MakeView(StaticValues);\
		}\
	};



#endif // ReflectionCollect_H_990CAAB9_A6AF_48D5_B6A2_79DDB78AE3DD

