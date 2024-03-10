#pragma once
#ifndef Reflection_H_BE8DB697_69C7_4488_8225_F320806C5D10
#define Reflection_H_BE8DB697_69C7_4488_8225_F320806C5D10

#include "DataStructure/Array.h"
#include "Meta/Concept.h"
#include "ReflectionCollect.h"

#include <typeindex>

namespace Reflection
{
	using namespace ReflectionMeta;

	template< typename T >
	struct TMetaFactory
	{
		template< typename TMeta >
		static TMeta Make(TMeta&& data) { return std::forward<TMeta>(data); }

		template< typename Q >
		static TSlider<T> Make(TSlider<Q>&& data) { return TSlider<T>(std::forward<TSlider<Q>>(data)); }
	};

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
		Array,
		StdVector,
		StdMap,
		StdUnorderedMap,
		Struct,

	};

	class MetaDataElement
	{
	public:
		template< typename T >
		MetaDataElement( T&& data )
			:mImpl(std::make_unique<TModel<T>>(std::forward<T>(data)))
		{

		}

		template< typename T >
		bool isA()
		{
			return mImpl->isA(typeid(T));
		}
		template< typename T >
		T& get() { return *(T*)mImpl->getData(); }
		template< typename T >
		T const& get() const { return *(T const*)mImpl->getData(); }

		struct IConcept
		{
			IConcept(std::type_info const& info)
				:typeIndex(info)
			{

			}
			virtual ~IConcept() = default;

			bool  isA(std::type_info const& info) { return typeIndex == info; }
			virtual void* getData() = 0;
			std::type_index typeIndex;
		};

		template< typename T >
		struct TModel : public IConcept
		{
		public:
			TModel(T&& data)
				:IConcept(typeid(T))
				,mData(std::forward<T>(data))
			{

			}

			void* getData() override { return &mData; }

			T mData;
		};
		std::unique_ptr< IConcept > mImpl;
	};

	class MetaData
	{
	public:
		template< typename T >
		bool contain() const
		{
			for (auto const& element : mElements)
			{
				if (element.isA<T>())
					return true;
			}
			return false;
		}
		template< typename T >
		T const* find() const
		{
			for (auto const& element : mElements)
			{
				if (element.isA<T>())
					return &element.get<T>();
			}
			return nullptr;
		}
		template< typename T >
		T* find()
		{
			for (auto& element : mElements)
			{
				if (element.isA<T>())
					return &element.get<T>();
			}
			return nullptr;
		}

		template< typename T >
		void add(T&& data)
		{
			mElements.emplace_back(std::forward<T>(data));
		}

		template< typename T, typename ...TList >
		void add(T&& data, TList&& ...dataList)
		{
			mElements.emplace_back(std::forward<T>(data));
			add(std::forward<TList>(dataList)...);
		}

		template< typename TFactory, typename T >
		void addWithFactory(T&& data)
		{
			mElements.emplace_back(TFactory::Make(std::forward<T>(data)));
		}

		template< typename TFactory, typename T, typename ...TList >
		void addWithFactory(T&& data, TList&& ...dataList)
		{
			mElements.emplace_back(TFactory::Make(std::forward<T>(data)));
			addWithFactory<TFactory>(std::forward<TList>(dataList)...);
		}


		TArray<MetaDataElement> mElements;
	};


	class PropertyBase
	{
	public:
		PropertyBase(std::type_info const& info)
			:typeIndex(info)
		{

		}

		char const* name = nullptr;
		uint32 offset = 0;
		std::type_index typeIndex;
		MetaData meta;

		virtual ~PropertyBase() = default;


		template< class T, typename P >
		void setOffset(P(T::*memberPtr))
		{
			T* ptr = (T*)(0);
			offset = (uint8*)(&(ptr->*memberPtr)) - (uint8*)ptr;
		}


		void* getDataInStruct(void* ptr) { return (uint8*)(ptr)+offset; }
		void* getDataInStruct(void* ptr, int index) { return ((uint8*)(ptr) + offset ) + index * getTypeSize(); }
		virtual EPropertyType getType() = 0;
		virtual int getTypeSize() = 0;
		virtual void constructDefault(void* ptr) = 0;


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
	struct ContainerTraitsBase
	{
		static constexpr int IsArray = 0;
		static constexpr int IsStdVector = 0;
	};
	template< typename T >
	struct TContainerTraits : public ContainerTraitsBase
	{
		static constexpr int IsArray = 0;
		static constexpr int IsStdVector = 0;
	};

	template< typename T, typename A >
	struct TContainerTraits< TArray< T, A > > : public ContainerTraitsBase
	{
		static constexpr int IsArray = 1;
		using ElementType = T;
		using AllocatorType = A;
	};


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
		int  getTypeSize() override { return sizeof(T); }
		void constructDefault(void* ptr) override {  *reinterpret_cast<T*>(ptr) = 0; }
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
		MetaData meta;

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
		int getTypeSize() override 
		{
			switch (underlyingType)
			{
			case Reflection::EPropertyType::Uint8:
			case Reflection::EPropertyType::Int8:
				return sizeof(int8);
			case Reflection::EPropertyType::Uint16:
			case Reflection::EPropertyType::Int16:
				return sizeof(int16);
			case Reflection::EPropertyType::Uint32:
			case Reflection::EPropertyType::Int32:
				return sizeof(int32);
			case Reflection::EPropertyType::Uint64:
			case Reflection::EPropertyType::Int64:
				return sizeof(int64);
			}
			NEVER_REACH("getTypeSize");
			return 0; 
		}
		int64 getUnderlyingValue(void* data)
		{
			return EnumType::GetUnderlyingValue(data, underlyingType);
		}

		void setUnderlyingValue(void* data, int64 value)
		{
			EnumType::SetUnderlyingValue(data, value, underlyingType);
		}
		void constructDefault(void* ptr) override
		{
			EnumType::SetUnderlyingValue(ptr, 0, underlyingType);
		}
	};

	class StructProperty : public PropertyBase
	{
	public:
		using PropertyBase::PropertyBase;

		StructType*     structData;
		MetaData meta;
		int  size;

		EPropertyType getType() override
		{
			return EPropertyType::Struct;
		}


	};

	class ArrayPropertyBase : public PropertyBase
	{
	public:
		PropertyBase* elementProperty;
		using PropertyBase::PropertyBase;
		
		virtual int   getElementCount(void* ptr) = 0;
		virtual void* getElement(void* ptr, int index) = 0;
		EPropertyType getType() override
		{
			return EPropertyType::Array;
		}
	};

	template< typename T >
	class TArrayProperty : public ArrayPropertyBase
	{
	public:
		TArrayProperty()
			:ArrayPropertyBase(typeid(T))
		{

		}

		virtual int   getElementCount(void* ptr)
		{
			return reinterpret_cast<T*>(ptr)->size();
		}
		virtual void* getElement(void* ptr, int index)
		{
			if (reinterpret_cast<T*>(ptr)->data())
			{
				return reinterpret_cast<T*>(ptr)->data() + index;
			}
			return nullptr;
		}

		int getTypeSize() override
		{
			return sizeof(T);
		}

		void constructDefault(void* ptr) override
		{
			new (ptr) T();
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

		int getTypeSize() override
		{
			return sizeof(TStruct);
		}

		void constructDefault(void* ptr) override
		{
			new (ptr) TStruct();
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

	template< typename TStruct >
	class TUnformattedStructProperty : public UnformattedStructProperty
	{
	public:
		TUnformattedStructProperty()
			:UnformattedStructProperty(typeid(TStruct))
		{

		}

		int getTypeSize() override
		{
			return sizeof(TStruct);
		}

		void constructDefault(void* ptr) override
		{
			new (ptr) TStruct();
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
			REF_GET_ENUM_VALUES(T)
		);
	};

	class PropertyCollector
	{
	public:
		template< typename T >
		void beginClass(char const* name)
		{
			mOwner->name = name;
		}

		template< typename T, typename TBase >
		void addBaseClass()
		{

		}

		template< typename P >
		static PropertyBase* CreateProperty()
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
			else if constexpr (TContainerTraits<P>::IsArray)
			{
				TArrayProperty<P>* myProperty = new TArrayProperty<P>;
				myProperty->elementProperty = CreateProperty< TContainerTraits<P>::ElementType >();
				property = myProperty;
			}
			else if constexpr (TCheckConcept< CStructTypeAvailable, P >::Value)
			{
				StructProperty* myProperty = new TNativeStructProperty<P>;
				property = myProperty;
			}
			else
			{
				TUnformattedStructProperty<P>* myProperty = new TUnformattedStructProperty<P>();
				property = myProperty;
			}
			return property;
		}
		
		template< typename T, typename P >
		void addProperty(P (T::*memberPtr), char const* name)
		{
			PropertyBase* property = CreateProperty<P>();
			if (property)
			{
				property->name = name;
				property->setOffset(memberPtr);
				mOwner->properties.push_back(property);
			}
		}

		template< typename T, typename P, typename ...TMeta >
		void addProperty(P(T::*memberPtr), char const* name, TMeta&& ...meta)
		{
			PropertyBase* property = CreateProperty<P>();
			if (property)
			{
				property->name = name;
				property->setOffset(memberPtr);
				property->meta.addWithFactory<TMetaFactory<P>>(std::forward<TMeta>(meta)...);
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
			REF_COLLECT_TYPE(T, collector);
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
			mEnum.values = REF_GET_ENUM_VALUES(T);
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
