#ifndef Phy2DStage_h__
#define Phy2DStage_h__

#include "GameConfig.h"
#include "GameRenderSetup.h"
#include "StageBase.h"
#include "Phy2D/Phy2D.h"

#include "RHI/ShaderCore.h"
#include "RHI/RHICommand.h"


namespace Phy2D
{
	void jumpDebug();

	using namespace Render;

	template<typename T, typename Q>
	FORCEINLINE T AsValue(Q value)
	{
		static_assert(sizeof(T) == sizeof(Q));
		T* ptr = (T*)&value;
		return *ptr;
	}

	struct ObjectData
	{
		Vector3 meta;
		int     type;
		Vector2 pos;
		Vector2 rotation;

		void set(CollideObject& object)
		{
			auto shape = object.getShape();
			switch (shape->getType())
			{
			case Shape::eCircle:
				type = 0;
				meta = Vector3(static_cast<CircleShape*>(shape)->getRadius(), 0, 0);
				break;
			case Shape::eBox:
				type = 1;
				meta = Vector3(static_cast<BoxShape*>(shape)->mHalfExt.x, static_cast<BoxShape*>(shape)->mHalfExt.y, 0);
				break;
			case Shape::ePolygon:
				type = 2;
				meta = Vector3(AsValue<float>(int(0)), AsValue<float>(int(static_cast<PolygonShape*>(shape)->mVertices.size())), 0);
				break;
			}
			pos = object.getPos();
			rotation = object.getRotation().getXDir();
		}
	};

	struct ObjectParamData
	{
		DECLARE_UNIFORM_BUFFER_STRUCT(ObjectParamBlock);

		ObjectData ObjectA;
		ObjectData ObjectB;
	};

	struct VertexData
	{
		DECLARE_BUFFER_STRUCT(Vertices);
	};


	class TINY_API Phy2DStageBase : public StageBase
		                          , public IGameRenderSetup
	{
		typedef StageBase BaseClass;
	public:

		virtual bool onInit();


		virtual void onEnd();

		virtual void onUpdate(GameTimeSpan time);

		virtual void tick(){}
		virtual void updateFrame( int frame ){}

		void renderObject( RHIGraphics2D& g , CollideObject& obj );

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}


		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.bWasUsedPlatformGraphics = true;
		}

	};

	class TINY_API CollideTestStage : public Phy2DStageBase
	{
		typedef Phy2DStageBase BaseClass;
	public:
		CollideTestStage(){}

		virtual bool onInit();

		virtual void onEnd()
		{
			BaseClass::onEnd();
		}

		void onRender( float dFrame );



		void restart()
		{

		}


		virtual void tick()
		{
			//mObjects[0].mXForm.rotate( 0.0051 );
			//mIsCollided = mCollision.test( &mObjects[0] , &mObjects[1] , mContact );
		}

		virtual void updateFrame( int frame )
		{

		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}
		MsgReply onKey(KeyMsg const& msg) override;

		void moveObject(Vector2 const& offset)
		{
			mObjects[0].mXForm.translate(offset);
			doCollisionTest();
		}


		void doCollisionTest()
		{
			//mIsCollided = mCollision.testManifold(&mObjects[0], &mObjects[1], mManifold);
			mIsCollided = mCollision.testManifold(&mObjects[0], &mObjects[1], mManifold);
		}


		virtual bool setupRenderResource(ERenderSystem systemName)
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));
			mObjectParams.initializeResource(1);
			mVertexBuffer.initializeResource(mShape3.mVertices, EStructuredBufferType::Buffer);
			return true;
		}
	protected:
		ContactManifold mManifold;

		bool  mIsCollided;
		CollideObject mObjects[2];
		BoxShape     mShape1;
		CircleShape  mShape2;
		PolygonShape mShape3;
		Shape*       mShapes[3];
		CollisionManager mCollision;

		TStructuredBuffer<ObjectParamData> mObjectParams;
		TStructuredBuffer<Vector2>         mVertexBuffer;
	};


	class WorldTestStage : public Phy2DStageBase
	{
		typedef Phy2DStageBase BaseClass;
	public:
		WorldTestStage(){}

		TINY_API virtual bool onInit();

		TINY_API void debugEntry();

		virtual void onEnd()
		{
			BaseClass::onEnd();
		}


		TINY_API void onRender( float dFrame );



		void restart()
		{

		}


		TINY_API virtual void tick();

		virtual void updateFrame( int frame )
		{

		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		TINY_API MsgReply onKey(KeyMsg const& msg) override;
	protected:

		RigidBody*   mBody[2];

		World        mWorld;
		BoxShape     mBoxShape[2];
		BoxShape     mBoxShape2;
		BoxShape     mDynamicBoxShape;  // Shape for spawning dynamic boxes
		CircleShape  mCircleShape;
		PolygonShape mShape3;

	};

}


#endif // Phy2DStage_h__
