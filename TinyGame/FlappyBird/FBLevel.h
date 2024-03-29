#pragma once
#ifndef FBLevel_H_E70ADBCE_DC63_4421_9DA0_8BFEA4706C39
#define FBLevel_H_E70ADBCE_DC63_4421_9DA0_8BFEA4706C39

#include "Math/Math2D.h"
#include "Delegate.h"

#include <list>

namespace FlappyBird
{
	typedef Vector2 Vector2;

	float const TickTime = gDefaultTickTime / 1000.0f;

	float const GravityForce = 20.0;

	float const WorldWidth = 9.0f;
	float const WorldHeight = 12.0f;

	float const BirdPosX = 2.0f;
	float const BirdVelX = 2.8f;
	float const BirdRadius = 0.4f;
	Vector2 const BirdSize = Vector2(2 * BirdRadius, 2 * BirdRadius);
	float const BirdFlyVel = 9.0f;

	float const BirdTickOffset = BirdVelX * TickTime;

	float const GScale = 40;



	class BirdEntity
	{
	public:
		bool  bActive;
		float flyVel;
		int   randValue;

		BirdEntity()
		{
			randValue = ::rand();
		}
		void  reset()
		{
			bActive = true;
			mVel = 0.0f;
			flyVel = BirdFlyVel;
		}

		void         setPos(Vector2 const& pos) { mPos = pos; }
		Vector2 const& getPos() const { return mPos; }

		float getVel() { return mVel; }
		void  setVel(float v) { mVel = v; }
		void  tick()
		{
			mVel += -GravityForce * TickTime;
			mPos.y += mVel * TickTime;
			if( !bActive )
			{
				mPos.x -= BirdTickOffset;
			}
		}

		void fly()
		{
			mVel = flyVel;
		}
	private:
		float mVel;
		Vector2 mPos;

	};

	enum ColType
	{
		CT_PIPE_TOP,
		CT_PIPE_BOTTOM,
		CT_SCORE,
	};

	enum class EColResponse
	{
		Keep,
		Stop,
		Remove,
		RemoveAndStop,
	};

	inline EColResponse GetDefaultColTypeResponse(ColType type)
	{
		switch( type )
		{
		case CT_PIPE_BOTTOM:
		case CT_PIPE_TOP:
			return EColResponse::Stop;
		case CT_SCORE:
			return EColResponse::Remove;
		}
		return EColResponse::Keep;
	}

	struct ColObject
	{
		ColType  type;
		Vector2  pos;
		Vector2  size;
	};

	static bool TestInterection(float len, float min, float max)
	{
		if( max < 0.0f )
			return false;
		if( min > len )
			return false;
		return true;
	}

	static bool TestInterection(Vector2 const& size, Vector2 const& offset, Vector2 const& sizeOther)
	{
		return TestInterection(size.x, offset.x, offset.x + sizeOther.x) &&
			TestInterection(size.y, offset.y, offset.y + sizeOther.y);
	}

	struct PipeInfo
	{
		float posX;
		float topHeight;
		float buttonHeight;
		float width;
	};


	class GameLevel;
	class IBirdController
	{
	public:
		virtual void updateInput(GameLevel& level, BirdEntity& bird) = 0;
		virtual void release() {}
	};

	DECLARE_DELEGATE( LevelOverDelegate , void (GameLevel& level) );
	DECLARE_DELEGATE( CollisionDelegate , EColResponse (BirdEntity& bird, ColObject& obj ) );


	class GameLevel
	{
	public:
		GameLevel()
		{

		}

		CollisionDelegate onBirdCollsion;
		LevelOverDelegate onGameOver;

		void restart();

		void tick();

		auto getColObjects() const
		{
			return MakeIterator(mColObjects);
		}
		void addPipe(PipeInfo const& block);
		void addBird(BirdEntity& bird , IBirdController* controller = nullptr );
		void removeAllBird();

		void testCollision(BirdEntity& bird);
		void overGame();

		int  getNeighborPipes(PipeInfo pipes , int num )
		{

		}

		float mMoveDistance;

		bool mIsOver = true;
		bool bDiffcultMode = true;
		float mLastPipeHeight = 0;
		int  mTimerProduce;

		using ColObjectList = std::list< ColObject >;

		TArray< PipeInfo > mPipes;
		ColObjectList mColObjects;
		TArray<BirdEntity*> mAllBirds;
		TArray<BirdEntity*> mActiveBirds;
		struct BirdControl
		{
			BirdEntity*  bird;
			IBirdController* controller;
		};
		TArray< BirdControl > mBirdControls;
	};

}//namespace FlappyBird

#endif // FBLevel_H_E70ADBCE_DC63_4421_9DA0_8BFEA4706C39
