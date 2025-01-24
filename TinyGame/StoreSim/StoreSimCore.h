#pragma once
#ifndef StoreSimCore_H_A71F5561_617E_4DCE_8061_B3D38B01B4D5
#define StoreSimCore_H_A71F5561_617E_4DCE_8061_B3D38B01B4D5

#include "MarcoCommon.h"
#include "Math/Vector2.h"
#include "Math/Math2D.h"
#include "Math/GeometryPrimitive.h"

#include "HashString.h"
#include "Template/ArrayView.h"
#include "Collision2D/Bsp2D.h"

#include "StoreSimItem.h"

#include <unordered_map>

namespace StoreSim
{
	using Math::Vector2;
	using Math::XForm2D;


	class IArchive;



	using AABB = Math::TAABBox<Vector2>;
	FORCEINLINE void GetVertices(AABB const& bound, XForm2D const& xForm, Vector2 outVertices[4])
	{
		outVertices[0] = xForm.transformPosition(bound.min);
		outVertices[1] = xForm.transformPosition(Vector2(bound.max.x, bound.min.y));
		outVertices[2] = xForm.transformPosition(bound.max);
		outVertices[3] = xForm.transformPosition(Vector2(bound.min.x, bound.max.y));
	}
	FORCEINLINE void GetVertices(AABB const& bound, XForm2D const& xForm, float border, Vector2 outVertices[4])
	{
		outVertices[0] = xForm.transformPosition(bound.min - Vector2(border, border));
		outVertices[1] = xForm.transformPosition(Vector2(bound.max.x + border, bound.min.y - border));
		outVertices[2] = xForm.transformPosition(bound.max + Vector2(border, border));
		outVertices[3] = xForm.transformPosition(Vector2(bound.min.x - border, bound.max.y + border));
	}


	enum class EAreaFlags
	{
		Empty = 0,
		Block = 0x10,

		Aisle = BIT(0),


		Shared = 0xf,
	};

	FORCEINLINE EAreaFlags UniqueMask(EAreaFlags flags)
	{
		return EAreaFlags(uint32(flags) & ~uint32(EAreaFlags::Shared));
	}
	FORCEINLINE EAreaFlags SharedMask(EAreaFlags flags)
	{
		return EAreaFlags(uint32(flags) & uint32(EAreaFlags::Shared));
	}

	FORCEINLINE bool CanWalkable(EAreaFlags a)
	{
		return a != EAreaFlags::Block;
	}

	FORCEINLINE bool HaveBlock(EAreaFlags a, EAreaFlags b)
	{
		EAreaFlags ua = UniqueMask(a);
		EAreaFlags ub = UniqueMask(b);
		if (ua != EAreaFlags::Empty || ub != EAreaFlags::Empty)
			return true;

		EAreaFlags sa = SharedMask(a);
		EAreaFlags sb = SharedMask(b);

		if (sa == EAreaFlags::Empty || sb == EAreaFlags::Empty)
			return false;

		return sa != sb;
	}

	SUPPORT_ENUM_FLAGS_OPERATION(EAreaFlags);


	struct Area
	{
		AABB bound;
		EAreaFlags flags;
	};

	class Equipment;
	class EquipmentClass
	{
	public:
		virtual Equipment* create() = 0;
		virtual TArrayView<Area const> getAreaList() = 0;

		AABB calcAreaBound(XForm2D const& xForm);

		std::string name;
	};

	class Equipment
	{
	public:
		XForm2D transform;
		EquipmentClass* mClass;
		AABB    bound;


		TArray< Bsp2D::PolyArea > navAreas;

		void notifyTrasformChanged()
		{
			updateBound();
			updateNavArea();
		}

		void updateBound()
		{
			bound = mClass->calcAreaBound(transform);
		}

		static void BuildArea(Bsp2D::PolyArea& area, AABB const& bound, XForm2D const& xForm)
		{
			float border = 0.3;
			area.mVertices.resize(4);
			GetVertices(bound, xForm, border, area.mVertices.data());
		}
		void updateNavArea();

		virtual void serialize(IArchive& ar);
	};


	class EquipmentFactory
	{
	public:
		Equipment* create(char const* name)
		{
			HashString key;
			if (!HashString::Find(name, key))
				return nullptr;

			auto iter = mNameClassMap.find(key);
			if (iter == mNameClassMap.end())
				return nullptr;

			return iter->second->create();
		}

		bool registerClass(EquipmentClass* equipClass)
		{
			CHECK(equipClass);
			HashString key = equipClass->name;
			auto result = mNameClassMap.emplace(key, equipClass).second;
			if (!result)
			{

			}
			return result;
		}

		static EquipmentFactory& Instance()
		{
			static EquipmentFactory StaticInstance;
			return StaticInstance;
		}

		std::unordered_map<HashString, EquipmentClass*> mNameClassMap;
	};
	template< typename MyClass, typename TEquipment >
	class TEquipmentClass : public EquipmentClass
	{
	public:
		TEquipmentClass()
		{
			name = MyClass::GetClassName();
			EquipmentFactory::Instance().registerClass(this);
		}

		static char const* GetClassName();
		virtual Equipment* create()
		{
			Equipment* eqip = new Equipment;
			eqip->mClass = this;
			return eqip;
		}

		static MyClass* StaticClass()
		{
			static MyClass StaticResult;
			return &StaticResult;
		}
	};


	class StoreShelf : public Equipment
		             , public ItemContainer
	{
		ItemData mData;
	};

	class Actor
	{
	public:
		XForm2D transform;

	};



}//namespace StoreSim

#endif // StoreSimCore_H_A71F5561_617E_4DCE_8061_B3D38B01B4D5