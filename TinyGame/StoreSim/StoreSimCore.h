#pragma once
#ifndef StoreSimCore_H_A71F5561_617E_4DCE_8061_B3D38B01B4D5
#define StoreSimCore_H_A71F5561_617E_4DCE_8061_B3D38B01B4D5

#include "Math/Vector2.h"
#include "Math/Math2D.h"
#include "MarcoCommon.h"
#include "Math/GeometryPrimitive.h"
#include "Template/ArrayView.h"
#include "Collision2D/Bsp2D.h"
#include "StoreSimItem.h"

class IStreamSerializer;


namespace StoreSim
{
	using Math::Vector2;
	using Math::XForm2D;


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
			float border = 0.1;
			area.mVertices.resize(4);
			GetVertices(bound, xForm, border, area.mVertices.data());
		}
		void updateNavArea();

		virtual void serialize(IStreamSerializer& serializer)
		{


		}
	};



	template< typename MyClass, typename TEquipment >
	class TEquipmentClass : public EquipmentClass
	{
	public:
		virtual Equipment* create()
		{
			return new TEquipment;
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