#ifndef FBirdStage_h__
#define FBirdStage_h__

#include "StageBase.h"
#include "TVector2.h"
#include <list>


namespace FlappyBird
{
	typedef TVector2< float > Vec2f;

	float const TickTime = gDefaultTickTime / 1000.0f;
	float const GravityForce = 20.0;

	float const WorldWidth  = 5.0f;
	float const WorldHeight = 8.0f;

	float const BirdPosX    = 2.0f;
	float const BirdVelX    = 2.5f;
	float const BirdRadius  = 0.4f;
	Vec2f const BirdSize    = Vec2f( 2 * BirdRadius , 2 * BirdRadius );
	float const BirdFlyVel  = 6.0f;

	float const BirdTickOffset  = BirdVelX * TickTime;

	float const GScale     = 40; 

	class Bird
	{
	public:

		float flyVel;

		void  reset()
		{
			mVel   = 0.0f;
			flyVel = BirdFlyVel;
		}

		void         setPos( Vec2f const& pos ){ mPos = pos; }
		Vec2f const& getPos() const { return mPos; }

		float getVel(){ return mVel; }
		void  setVel( float v ){ mVel = v; }
		void tick()
		{
			mVel += - GravityForce * TickTime;
			mPos.y += mVel * TickTime;
		}

		void fly()
		{
			mVel = flyVel;
		}
	private:
		float mVel;
		Vec2f mPos;
	};

	enum ColType
	{
		CT_BLOCK_UP ,
		CT_BLOCK_DOWN ,
		CT_SCORE ,
	};

	struct ColObject
	{
		ColType type;
		Vec2f   pos;
		Vec2f   size;
	};

	static bool testInterection( float len , float min , float max )
	{
		if ( max < 0.0f )
			return false;
		if ( min > len )
			return false;
		return true;
	}

	static bool testInterection( Vec2f const& size , Vec2f const& offset , Vec2f const& sizeOther )
	{
		return testInterection( size.x , offset.x , offset.x + sizeOther.x ) &&
			   testInterection( size.y , offset.y , offset.y + sizeOther.y );
	}
	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage(){}

		virtual bool onInit();

		virtual void onUpdate( long time )
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();

			updateFrame( frame );
		}


		Vec2i convertToScreen( Vec2f const& pos )
		{
			return Vec2i( 250 + GScale * pos.x , 450 - GScale * pos.y );
		}

		void onRender( float dFrame );


		void restart()
		{
			mColObjects.clear();
			mScore = 0;
			mTimerProduce = 0;
			mIsOver = false;
			mBird.reset();
			mBird.setPos( Vec2f( BirdPosX , WorldHeight / 2 ) );
		}

		void overGame();


		void tick();

		void updateFrame( int frame )
		{

		}

		bool onMouse( MouseMsg const& msg )
		{
			if ( !BaseClass::onMouse( msg ) )
				return false;

			if ( msg.onLeftDown() || msg.onLeftDClick() )
			{
				if ( !mIsOver ) 
					mBird.fly();
			}

			return false;
		}

		bool onWidgetEvent( int event , int id , GWidget* ui )
		{
			switch( id )
			{
			case UI_OK:
				restart();
				return false;
			}

			return true;
		}

		bool onKey( unsigned key , bool isDown )
		{
			if ( !isDown )
				return false;

			switch( key )
			{
			case 'R': restart(); break;
			case 'S': 
				if ( !mIsOver ) 
					mBird.fly();
				break;
			}
			return false;
		}
	protected:
		typedef std::list< ColObject > ColObjectList;



		bool          mIsOver;
		int           mMaxScore;
		int           mScore;
		int           mTimerProduce;
		ColObjectList mColObjects;
		Bird          mBird;


	};


}//namespace FlappyBird

#endif // FBirdStage_h__
