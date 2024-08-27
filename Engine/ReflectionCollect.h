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
			:min(T(rhs.min)), max(T(rhs.max))
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
	template< typename T >
	void beginClass(char const* name);

	template< typename T , typename TBase >
	void addBaseClass();

	template< typename T, typename P >
	void addProperty(P(T::*memberPtr), char const* name);

	template< typename T, typename P , typename ...TMeta >
	void addProperty(P(T::*memberPtr), char const* name, TMeta&& ...meta);

	void endClass();

};

struct ReflectEnumValueInfo
{
	char const* text;
	uint64 value;
};


struct FReflectionCollector
{

	template< typename T, typename TCollector, typename P >
	FORCEINLINE static void AddProperty(TCollector& c, P(T::*memberPtr), char const* name)
	{
		c.addProperty(memberPtr, name);
	}

	template< typename T, typename TCollector, typename P, typename ...TMeta >
	FORCEINLINE static void AddProperty(TCollector& c, P(T::*memberPtr), char const* name, char const* overridedName)
	{
		c.addProperty(memberPtr, overridedName);
	}

	template< typename T, typename TCollector, typename P, typename ...TMeta >
	FORCEINLINE static void AddProperty(TCollector& c, P(T::*memberPtr), char const* name, TMeta&& ...meta)
	{
		c.addProperty(memberPtr, name, std::forward<TMeta>(meta)...);
	}

	template< typename T, typename TCollector, typename P, typename ...TMeta >
	FORCEINLINE static void AddProperty(TCollector& c, P(T::*memberPtr), char const* name, char const* overridedName, TMeta&& ...meta)
	{
		c.addProperty(memberPtr, overridedName, std::forward<TMeta>(meta)...);
	}

	template< typename TEnum >
	FORCEINLINE static ReflectEnumValueInfo MakEnumInfo(TEnum enumValue, char const* name)
	{
		return { name, (uint64)enumValue };
	}

	template< typename TEnum >
	FORCEINLINE static ReflectEnumValueInfo MakEnumInfo(TEnum enumValue, char const* name, char const* overridedName)
	{
		return { overridedName, (uint64)enumValue };
	}
};




template<typename T>
struct TReflectEnumValueTraits
{

};

#define REFLECT_STRUCT_BEGIN( CLASS )\
public:\
	template< class ReflectionCollector >\
	static void CollectReflection(ReflectionCollector& collector)\
	{\
		using namespace ReflectionMeta;\
		using ThisClass = CLASS;\
		collector.beginClass< ThisClass >( #CLASS );

#define REF_BASE_CLASS( BASE )\
		collector.addBaseClass< ThisClass , BASE >();

#define REF_PROPERTY( VAR , ... )\
		FReflectionCollector::AddProperty<ThisClass>(collector, &ThisClass::VAR, #VAR, ##__VA_ARGS__);

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

#define REF_ENUM( VALUE , ... ) FReflectionCollector::MakEnumInfo( EnumType::VALUE , #VALUE, ##__VA_ARGS__),


#define REF_ENUM_END()\
			};\
			return MakeView(StaticValues);\
		}\
	};


#define REF_COLLECT_TYPE( TYPE , COLLECTOR )\
	TYPE::CollectReflection(COLLECTOR)

#define REF_GET_ENUM_VALUES( TYPE )\
	::TReflectEnumValueTraits< TYPE >::GetValues()

#endif // ReflectionCollect_H_990CAAB9_A6AF_48D5_B6A2_79DDB78AE3DD

