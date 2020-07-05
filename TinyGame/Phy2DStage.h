#ifndef Phy2DStage_h__
#define Phy2DStage_h__

#include "GameConfig.h"
#include "StageBase.h"
#include "Phy2D/Phy2D.h"

namespace Phy2D
{
	void jumpDebug();

	class TINY_API Phy2DStageBase : public StageBase
	{
		typedef StageBase BaseClass;
	public:

		virtual bool onInit();


		virtual void onEnd();

		virtual void onUpdate( long time );

		virtual void tick(){}
		virtual void updateFrame( int frame ){}

		void renderObject( RHIGraphics2D& g , CollideObject& obj );


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

		bool onMouse( MouseMsg const& msg )
		{
			if ( !BaseClass::onMouse( msg ) )
				return false;
			return true;
		}
		void moveObject( Vector2 const& offset )
		{
			mObjects[1].mXForm.translate( offset );
			mIsCollided = mCollision.test( &mObjects[0] , &mObjects[1] , mContact );
		}

		bool onKey(KeyMsg const& msg);
	protected:
		Contact mContact;

		bool  mIsCollided;
		CollideObject mObjects[2];
		BoxShape     mShape1;
		CircleShape  mShape2;
		PolygonShape mShape3;
		Shape*       mShapes[3];
		CollisionManager mCollision;
	};


	class TINY_API WorldTestStage : public Phy2DStageBase
	{
		typedef Phy2DStageBase BaseClass;
	public:
		WorldTestStage(){}

		virtual bool onInit();

		void debugEntry();

		virtual void onEnd()
		{
			BaseClass::onEnd();
		}


		void onRender( float dFrame );



		void restart()
		{

		}


		virtual void tick();

		virtual void updateFrame( int frame )
		{

		}

		bool onMouse( MouseMsg const& msg )
		{
			if ( !BaseClass::onMouse( msg ) )
				return false;
			return true;
		}


		bool onKey(KeyMsg const& msg);
	protected:

		RigidBody*   mBody[2];

		World        mWorld;
		BoxShape     mBoxShape[2];
		BoxShape     mBoxShape2;
		CircleShape  mCircleShape;
		PolygonShape mShape3;

	};

}


#endif // Phy2DStage_h__
