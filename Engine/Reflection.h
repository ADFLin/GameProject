#pragma once
#ifndef Reflection_H_BE8DB697_69C7_4488_8225_F320806C5D10
#define Reflection_H_BE8DB697_69C7_4488_8225_F320806C5D10

#include "DataStructure/Array.h"
#include "Meta/Concept.h"
#include "ReflectionCollect.h"

#include <typeindex>


namespace Reflection
{

	enum class EPropertyType
	{
		Bool,
		Uint8,
		Uint16,
		Uint32,
		Uint64,
		Int8,
		Int16,
		Int32,
		Int64,
		Float,
		Double,
		Enum ,
		UnformattedStruct,
		Struct,

	};

	class PropertyBase
	{
	public:
		PropertyBase(std::type_info const& info)
			:typeIndex(info)
		{

		}

		char const* name;
		uint32 offset;
		std::type_index typeIndex;

		virtual ~PropertyBase() = default;


		template< class T, typename P >
		void setOffset(P(T::*memberPtr))
		{
			T* ptr = (T*)(0);
			offset = (uint8*)(&(ptr->*memberPtr)) - (uint8*)ptr;
		}

		void* getDataInStruct(void* ptr) { return (uint8*)(ptr)+offset; }
		virtual EPropertyType getType() = 0;

	};

	template< typename T >
	struct PrimaryTypeTraits
	{

	};

#define DEF_PRIMARY_TYPE( TYPE , TYPENAME )\
	template<>\
	struct PrimaryTypeTraits<TYPE>\
	{\
		static constexpr EPropertyType Type = EPropertyType::TYPENAME;\
	}

	DEF_PRIMARY_TYPE(bool, Bool);
	DEF_PRIMARY_TYPE(int8, Int8);
	DEF_PRIMARY_TYPE(int16, Int16);
	DEF_PRIMARY_TYPE(int32, Int32);
	DEF_PRIMARY_TYPE(int64, Int64);
	DEF_PRIMARY_TYPE(uint8, Uint8);
	DEF_PRIMARY_TYPE(uint16, Uint16);
	DEF_PRIMARY_TYPE(uint32, Uint32);
	DEF_PRIMARY_TYPE(uint64, Uint64);
	DEF_PRIMARY_TYPE(float, Float);
	DEF_PRIMARY_TYPE(double, Double);

#undef DEF_PRIMARY_TYPE

	template< typename T >
	class TPrimaryTypePorperty : public PropertyBase
	{
	public:
		TPrimaryTypePorperty()
			:PropertyBase(typeid(T))
		{

		}

		EPropertyType getType() override
		{
			return PrimaryTypeTraits<T>::Type;
		}
	};

	class StructType
	{
	public:
		StructType(std::type_info const& info)
			:typeIndex(info)
		{

		}
		char const* name;
		TArray<PropertyBase*> properties;
		std::type_index typeIndex;

		~StructType()
		{
			for (auto property : properties)
			{
				delete property;
			}
		}
	};

	class EnumType
	{
	public:
		EPropertyType underlyingType;
		TArrayView< ReflectEnumValueInfo const > values;

		int64 getValue(void* data)
		{
			return EnumType::GetUnderlyingValue(data, underlyingType);
		}

		void setValue(void* data, int64 value)
		{
			EnumType::SetUnderlyingValue(data, value, underlyingType);
		}

		template< typename T >
		static T GetValue(void* data) { return *reinterpret_cast<T*>(data); }
		template< typename T >
		static void SetValue(void* data, T value) { *reinterpret_cast<T*>(data) = value; }

		static int64 GetUnderlyingValue(void* data, EPropertyType underlyingType)
		{
			switch (underlyingType)
			{
			case EPropertyType::Uint8: return GetValue<uint8>(data);
			case EPropertyType::Uint16:return GetValue<uint16>(data);
			case EPropertyType::Uint32:return GetValue<uint32>(data);
			case EPropertyType::Uint64:return GetValue<uint64>(data);
			case EPropertyType::Int8:return GetValue<int8>(data);
			case EPropertyType::Int16:return GetValue<int16>(data);
			case EPropertyType::Int32:return GetValue<int32>(data);
			case EPropertyType::Int64:return GetValue<int64>(data);
			}

			NEVER_REACH("getUnderlyingValue");
			return 0;
		}

		static void SetUnderlyingValue(void* data, int64 value, EPropertyType underlyingType)
		{
			switch (underlyingType)
			{
			case EPropertyType::Uint8: return SetValue<uint8>(data, value);
			case EPropertyType::Uint16:return SetValue<uint16>(data, value);
			case EPropertyType::Uint32:return SetValue<uint32>(data, value);
			case EPropertyType::Uint64:return SetValue<uint64>(data, value);
			case EPropertyType::Int8:return SetValue<int8>(data, value);
			case EPropertyType::Int16:return SetValue<int16>(data, value);
			case EPropertyType::Int32:return SetValue<int32>(data, value);
			case EPropertyType::Int64:return SetValue<int64>(data, value);
			}

			NEVER_REACH("SetUnderlyingValue");
		}
	};


	class EnumProperty : public PropertyBase
	{
	public:
		using PropertyBase::PropertyBase;

		EPropertyType underlyingType;
		EnumType*  enumData = nullptr;

		EPropertyType getType() override
		{
			return EPropertyType::Enum;
		}

		int64 getUnderlyingValue(void* data)
		{
			return EnumType::GetUnderlyingValue(data, underlyingType);
		}

		void setUnderlyingValue(void* data, int64 value)
		{
			EnumType::SetUnderlyingValue(data, value, underlyingType);
		}
		


	};

	class StructProperty : public PropertyBase
	{
	public:
		using PropertyBase::PropertyBase;

		StructType*     structData;

		EPropertyType getType() override
		{
			return EPropertyType::Struct;
		}
	};


	template< typename TStruct >
	class TNativeStructProperty : public StructProperty
	{
	public:
		TNativeStructProperty()
			:StructProperty(typeid(TStruct))
		{
			structData = GetStructType<TStruct>();
		}
	};
	class UnformattedStructProperty : public PropertyBase
	{
	public:
		using PropertyBase::PropertyBase;
		EPropertyType getType() override
		{
			return EPropertyType::UnformattedStruct;
		}
	};
	template< typename T >
	StructType* GetStructType();
	class PropertyCollector;
	struct CStructTypeAvailable
	{
		template< typename T >
		static auto Requires(T&) -> decltype
		(
			T::CollectReflection(PropertyCollector())
		);
	};
	struct CEnumValueAvailable
	{
		template< typename T >
		static auto Requires(T&) -> decltype
		(
			::TReflectEnumValueTraits<T>::GetValues()
		);
	};

	class PropertyCollector
	{
	public:
		template< class T >
		void beginClass(char const* name)
		{
			mOwner->name = name;
		}

		template< typename T, typename Base >
		void addBaseClass()
		{

		}

		template< class T, typename P >
		void addProperty(P (T::*memberPtr), char const* name)
		{
			PropertyBase* property = nullptr;
			if constexpr (Meta::IsPrimary<P>::Value)
			{
				TPrimaryTypePorperty<P>* myProperty = new TPrimaryTypePorperty<P>;
				property = myProperty;
			}
			else if constexpr (std::is_enum_v<P>)
			{
				EnumProperty* myProperty = new EnumProperty(typeid(P));
				using UnderlyingType = std::underlying_type_t<P>;
				myProperty->underlyingType = PrimaryTypeTraits<UnderlyingType>::Type;
				if constexpr (TCheckConcept< CEnumValueAvailable, P >::Value)
				{
					myProperty->enumData = GetEnumType<P>();
				}
				property = myProperty;
			}
			else if constexpr (TCheckConcept< CStructTypeAvailable, P >::Value)
			{
				StructProperty* myProperty = new TNativeStructProperty<P>;
				property = myProperty;
			}
			else
			{
				UnformattedStructProperty* myProperty = new UnformattedStructProperty(typeid(P));
				property = myProperty;
			}

			if (property)
			{
				property->name = name;
				property->setOffset(memberPtr);
				mOwner->properties.push_back(property);
			}
		}

		void endClass()
		{

		}

		StructType* mOwner;
	};


	template< typename T >
	struct TStructTypeInitializer
	{
		TStructTypeInitializer()
			:mStruct(typeid(T))
		{
			PropertyCollector collector;
			collector.mOwner = &mStruct;
			T::CollectReflection(collector);
		}
		StructType* getStruct() { return &mStruct; }
		StructType mStruct;
	};

	template< typename T >
	StructType* GetStructType()
	{
		static TStructTypeInitializer<T> StaticInitializer;
		return StaticInitializer.getStruct();
	}


	template< typename T >
	struct TEnumTypeInitializer
	{
		TEnumTypeInitializer()
		{
			using UnderlyingType = std::underlying_type_t<T>;
			mEnum.underlyingType = PrimaryTypeTraits<UnderlyingType>::Type;
			mEnum.values = TReflectEnumValueTraits<T>::GetValues();
		}
		EnumType* getEnum() { return &mEnum; }
		EnumType mEnum;
	};

	template< typename T >
	EnumType* GetEnumType()
	{
		static TEnumTypeInitializer<T> StaticInitializer;
		return StaticInitializer.getEnum();
	}

}

#endif // Reflection_H_BE8DB697_69C7_4488_8225_F320806C5D10
