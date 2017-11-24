#pragma once
#ifndef ECSBase_H_6465714A_7ACE_4CF3_89D6_FE84F26B6074
#define ECSBase_H_6465714A_7ACE_4CF3_89D6_FE84F26B6074

#include "Core/IntegerType.h"
namespace ECS
{

	//typedef uint32 ComponentId;

	struct ComponentId
	{




	}
	class ComponentInitializer
	{

	};

	class WorldAdmin;
	class System
	{
	public:
		virtual void updateFrame(){}
	protected:
		WorldAdmin* mAdmin;
	};

	class Component
	{
	public:
		virtual void construct() = 0;
		virtual void destroy() = 0;

		Entity* getOwner() { return mOwner; }
	private:
		friend class Entity;
		Entity* mOwner;
	};

	class Entity
	{
	public:
		int getComponentNum() { mComponents.size(); }






		std::vector< ComponentId > mComponents;
	};

	class WorldAdmin
	{





	};

}//namespace ECS

#endif // ECSBase_H_6465714A_7ACE_4CF3_89D6_FE84F26B6074
