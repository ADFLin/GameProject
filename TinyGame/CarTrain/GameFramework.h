#pragma once
#ifndef GameFramework_H_D92E68B4_6F34_4D30_AC05_36AB57678E0E
#define GameFramework_H_D92E68B4_6F34_4D30_AC05_36AB57678E0E

#include "DataStructure/ClassTree.h"
#include "Math/Math2D.h"
#include "Renderer/RenderTransform2D.h"

namespace CarTrain
{
	using Math::XForm2D;
	using Math::Rotation2D;
	using Math::Vector2;
	using Render::RenderTransform2D;

	namespace ECollision
	{
		enum Type : uint8
		{
			Wall,
			Car,
		};
	}

	struct PhyObjectDef
	{
		bool    bCollisionDetection = true;
		bool    bCollisionResponse = false;
		bool    bEnablePhysics = false;
		uint8   collisionType;
		uint16  collisionMask;
		float   mass;

		PhyObjectDef()
		{
			collisionType = ECollision::Wall;
			collisionMask = 0xffff;
		}

		template< class OP >
		void serialize(OP& op)
		{
			op & bCollisionDetection;
			op & bCollisionResponse;
			op & bEnablePhysics;
			op & collisionType;
			op & collisionMask & mass;
		}
	};

	struct BoxObjectDef : PhyObjectDef
	{
		Vector2 extend;

		template< class OP >
		void serialize(OP& op)
		{
			PhyObjectDef::serialize(op);
			op & extend;
		}
	};

	struct RayHitInfo
	{
		float   fraction;
		Vector2 normal;
		Vector2 hitPos;
	};

	class IPhysicsBody;
	using PhyCollisionFunc = std::function< void(IPhysicsBody*) >;

	class IPhysicsBody
	{
	public:
		virtual float getMass() = 0;
		virtual PhyObjectDef* getDefine() = 0;
		virtual void    setTransform(XForm2D const& xform) = 0;
		virtual XForm2D getTransform() const = 0;
		virtual Vector2 getLinearVel() const = 0;
		virtual float   getAngle() const = 0;
		virtual float   getAngularVel() const = 0;
		virtual void    setLinearVel(Vector2 const& vel) = 0;

		virtual void    setRotation(float angle) = 0;
		virtual float   getRotation() const = 0;


		virtual void*   getUserData() = 0;
		virtual void    setUserData(void* data) = 0;

		PhyCollisionFunc onCollision;
	};

	class IPhysicsScene
	{
	public:
		virtual bool initialize() = 0;
		virtual void release() = 0;
		virtual IPhysicsBody* createBox(BoxObjectDef const& def, XForm2D xForm) = 0;
		virtual void tick(float deltaTime) = 0;

		virtual bool rayCast(Vector2 const& startPos, Vector2 const& endPos, RayHitInfo& outInfo, uint16 collisionMask = 0xff) = 0;

		virtual void setupDebugView(RHIGraphics2D& g) = 0;
		virtual void drawDebug() = 0;

		static IPhysicsScene* Create();
	};


	class GameWorld;


	class GameEntityClass : public ClassTreeNode
	{
	public:
		GameEntityClass(GameEntityClass* superClass)
			:ClassTreeNode(GetTree(), superClass)
		{
		}

		bool isSubClassOf(GameEntityClass& other) const
		{
			return ClassTreeNode::isChildOf(&other);
		}

		static ClassTree& GetTree()
		{
			static ClassTree sTree;
			return sTree;
		}
	};

	template< typename TEntity >
	class TGameEntityClass : public GameEntityClass
	{
	public:
		TGameEntityClass(GameEntityClass* superClass)
			:GameEntityClass(superClass)
		{
		}

	};


	class GameEntity
	{
	public:
		GameWorld* getWorld() { return mWorld; }

		virtual ~GameEntity(){}
		virtual GameEntityClass* getClass() { return &StaticClass(); }
		virtual void beginPlay() {}
		virtual void endPlay() {}
		virtual void tick(float deltaTime) {}
		virtual void postTick(float deltaTime) {}

		void destroyEntity();
		bool bActive;

		GameWorld* mWorld;

		static TGameEntityClass<GameEntity>& StaticClass()
		{
			static TGameEntityClass<GameEntity> sClass(nullptr);
			return sClass;
		}

		template< typename TEntity >
		TEntity* cast()
		{
			if (getClass()->isSubClassOf(TEntity::StaticClass()))
			{
				return static_cast<TEntity*>(this);
			}
			return nullptr;
		}
	};

#define DECLARE_GAME_ENTITY(TYPE , PARENT_TYPE)\
	using BaseClass = PARENT_TYPE;\
	static TGameEntityClass<TYPE>& StaticClass()\
	{\
		static TGameEntityClass<TYPE> sClass(&BaseClass::StaticClass());\
		return sClass;\
	}\
	GameEntityClass* getClass() override { return &TYPE::StaticClass(); }


	class IEntityController
	{
	public:
		virtual void updateInput(GameEntity& entity) = 0;
		virtual void release() {}
	};



	class GameWorld
	{
	public:
		bool initialize();
		void clearnup();

		void drawDebug()
		{
			mPhyScene->drawDebug();
		}

		void updateControl()
		{
			for (EntityControl& control : mControlList)
			{
				if (control.entity->bActive)
				{
					control.controller->updateInput(*control.entity);
				}
			}
		}

		void tick(float deltaTime);

		template< typename TEntity, typename ...Args >
		TEntity* spawnEntity(Args&& ...args)
		{
			TEntity* entity = new TEntity(std::forward<Args>(args)...);
			entity->mWorld = this;
			mEntities.push_back(entity);
			entity->beginPlay();
			return entity;
		}


		std::vector<GameEntity*> mPendingDestoryEntities;
		void markEntityDestroy(GameEntity* entity)
		{
			AddUnique(mPendingDestoryEntities, entity);
		}

		void destroyEntity(GameEntity* entity)
		{
			RemoveValue(mEntities, entity);
			entity->endPlay();
			delete entity;
		}

		IPhysicsScene* getPhysicsScene() { return mPhyScene; }
		IPhysicsScene* mPhyScene = nullptr;

		std::vector<GameEntity*> mEntities;

		struct EntityControl
		{
			IEntityController* controller;
			GameEntity* entity;
		};
		std::vector<EntityControl> mControlList;
	};

}
#endif // GameFramework_H_D92E68B4_6F34_4D30_AC05_36AB57678E0E