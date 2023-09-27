#pragma once
#ifndef ReflectionCollect_H_990CAAB9_A6AF_48D5_B6A2_79DDB78AE3DD
#define ReflectionCollect_H_990CAAB9_A6AF_48D5_B6A2_79DDB78AE3DD

#include "Template/ArrayView.h"

namespace ReflectionMeta
{
	template< typename T >
	struct TSlider
	{
		TSlider(T min, T max)
			:min(min), max(max)
		{
		}
		template< typename Q >
		TSlider(TSlider<Q> const& rhs)
			:min(rhs.min), max(rhs.max)
		{
		}
		T min;
		T max;
	};

	struct Category
	{
		Category(char const* name)
			:name(name){ }

		char const* name;
	};

	template< typename T >
	TSlider<T> Slider(T min, T max)
	{
		return TSlider<T>(min, max);
	}
}

class ReflectionCollector
{
	template< class T >
	void beginClass(char const* name);

	template< typename T , typename Base >
	void addBaseClass();

	template< class T, typename P >
	void addProperty(P(T::*memberPtr), char const* name);

	template< class T, typename P , typename ...TMeta >
	void addProperty(P(T::*memberPtr), char const* name, TMeta&& ...meta);

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
		using namespace ReflectionMeta;\
		using ThisClass = CLASS;\
		collector.beginClass< ThisClass >( #CLASS );

#define REF_BASE_CLASS( BASE )\
		collector.addBaseClass< ThisClass , BASE >();

#define REF_PROPERTY( VAR , ... )\
		collector.addProperty(&ThisClass::VAR, #VAR, ##__VA_ARGS__);\

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
			using namespace ReflectionMeta; \
			static ReflectEnumValueInfo const StaticValues[] =\
			{

#define REF_ENUM( VALUE ) { #VALUE , (uint64)EnumType::VALUE },


#define REF_ENUM_END()\
			};\
			return MakeView(StaticValues);\
		}\
	};


#endif // ReflectionCollect_H_990CAAB9_A6AF_48D5_B6A2_79DDB78AE3DD

