#include "TinyGamePCH.h"
#include "FBLevel.h"

namespace FlappyBird
{

	void GameLevel::restart()
	{
		mColObjects.clear();
		mPipes.clear();

		mMoveDistance = 0;
		mTimerProduce = 0;
		mIsOver = false;
		mActiveBirds.clear();
		mLastPipeHeight = 0;

		for( auto bird : mAllBirds )
		{
			bird->reset();
			bird->setPos(Vector2(BirdPosX, WorldHeight / 2));
			mActiveBirds.push_back(bird);
		}
	}

	void GameLevel::tick()
	{
		if( mIsOver )
			return;

		mMoveDistance += BirdTickOffset;
		for( auto iter = mPipes.begin(); iter != mPipes.end(); )
		{
			PipeInfo& block = *iter;
			block.posX -= BirdTickOffset;
			if( block.posX + block.width + BirdRadius < BirdPosX )
			{
				iter = mPipes.erase(iter);
			}
			else
			{
				++iter;
			}
		}

		for( ColObjectList::iterator iter = mColObjects.begin(), itEnd = mColObjects.end();
			iter != itEnd; )
		{
			ColObject& obj = *iter;
			obj.pos.x -= BirdTickOffset;

			bool needRemove = false;
			if( obj.pos.x + obj.size.x < 0 )
				needRemove = true;

			if( needRemove )
				iter = mColObjects.erase(iter);
			else
				++iter;
		}

		for( auto& control : mBirdControls )
		{
			if( control.bird->bActive )
			{
				control.controller->updateInput(*this, *control.bird);
			}
		}
		
		for( auto bird : mAllBirds )
		{
			bird->tick();
			Vector2 const& pos = bird->getPos();
			if(!bird->bActive && pos.y < BirdRadius )
			{
				bird->setPos(Vector2(pos.x, BirdRadius));
			}
		}

		for( auto iter = mActiveBirds.begin(); iter != mActiveBirds.end(); )
		{
			BirdEntity* bird = *iter;
			Vector2 const& pos = bird->getPos();
			if( pos.y < BirdRadius )
			{
				bird->setPos(Vector2(pos.x, BirdRadius));
				bird->bActive = false;
			}
			else if( pos.y > WorldHeight - BirdRadius )
			{
				bird->bActive = false;
			}

			testCollision(*bird);
			if( !bird->bActive )
			{
				iter = mActiveBirds.erase(iter);
			}
			else
			{
				++iter;
			}
		}

		if( mTimerProduce == 0 )
		{
			float const BlockGap = 2.9f /** 0.8*/;
			float const BlockDistance = 5.5;
			float const BlockWidth = 1.5f;

			int const BlockTimer = int(BlockDistance / BirdTickOffset);

			if( bDiffcultMode )
			{
				float const HeightMax = WorldHeight - BlockGap - 0.5f - 1.5;
				float const HeightMin = 1.0f;

				float halfHeight = 0.5 * (HeightMax + HeightMin);
				float deltaHeight = 0.5 * (HeightMax - HeightMin);
				float height;
				if( mLastPipeHeight > halfHeight )
				{
					height = halfHeight - deltaHeight * ::Global::Random() / float(RAND_MAX);
				}
				else
				{
					height = halfHeight + deltaHeight * ::Global::Random() / float(RAND_MAX);
				}
				float const posProduce = WorldWidth + 1.0f;

				PipeInfo pipe;
				pipe.buttonHeight = height;
				pipe.topHeight = height + BlockGap;
				pipe.width = BlockWidth;
				pipe.posX = posProduce;
				addPipe(pipe);
				mLastPipeHeight = height;
			}
			else
			{
				float const HeightMax = WorldHeight - BlockGap - 0.5f - 1.5;
				float const HeightMin = 1.0f;

				float height = HeightMin + (HeightMax - HeightMin) * ::Global::Random() / float(RAND_MAX);
				float const posProduce = WorldWidth + 1.0f;

				PipeInfo pipe;
				pipe.buttonHeight = height;
				pipe.topHeight = height + BlockGap;
				pipe.width = BlockWidth;
				pipe.posX = posProduce;
				addPipe(pipe);
				mLastPipeHeight = height;
			}

			mTimerProduce = BlockTimer;
		}
		else
		{
			--mTimerProduce;
		}


		if( mActiveBirds.empty() )
			overGame();
	}

	void GameLevel::addPipe(PipeInfo const& pipe)
	{
		assert(pipe.topHeight > pipe.buttonHeight);
		float gap = pipe.topHeight - pipe.buttonHeight;
		ColObject obj;
		obj.type = CT_PIPE_BOTTOM;
		obj.pos = Vector2(pipe.posX, 0);
		obj.size = Vector2(pipe.width, pipe.buttonHeight);
		mColObjects.push_back(obj);

		float const ScoreWidth = 0.5f;
		obj.type = CT_SCORE;
		obj.pos = Vector2(pipe.posX + (pipe.width - ScoreWidth) / 2, pipe.buttonHeight);
		obj.size = Vector2(ScoreWidth, gap);
		mColObjects.push_back(obj);

		obj.type = CT_PIPE_TOP;
		obj.pos = Vector2(pipe.posX, pipe.buttonHeight + gap);
		obj.size = Vector2(pipe.width, WorldHeight - obj.pos.y);
		mColObjects.push_back(obj);
		mPipes.push_back(pipe);
	}

	void GameLevel::addBird(BirdEntity& bird, IBirdController* controller)
	{
		mAllBirds.push_back(&bird);
		if( bird.bActive )
		{
			mActiveBirds.push_back(&bird);
		}
		if( controller )
		{
			BirdControl control;
			control.bird = &bird;
			control.controller = controller;
			mBirdControls.push_back(control);
		}
	}

	void GameLevel::removeAllBird()
	{
		mAllBirds.clear();
		mActiveBirds.clear();
		mBirdControls.clear();
	}

	void GameLevel::testCollision(BirdEntity& bird)
	{
		bool bStop = false;
		for( ColObjectList::iterator iter = mColObjects.begin(), itEnd = mColObjects.end();
			iter != itEnd; )
		{
			ColObject& obj = *iter;
			bool needRemove = false;

			Vector2 offset = obj.pos - (bird.getPos() - Vector2(BirdRadius, BirdRadius));
			if( TestInterection(BirdSize, offset, obj.size) )
			{
				switch( onBirdCollsion(bird, obj) )
				{
				case EColResponse::Remove:
					needRemove = true;
					break;
				case EColResponse::Stop:
					bStop = true;
					break;
				case EColResponse::RemoveAndStop:
					needRemove = true;
					bStop = true;
					break;

				default:
					break;
				};
			}

			if( needRemove )
				iter = mColObjects.erase(iter);
			else
				++iter;

			if( bStop )
				break;
		}
	}

	void GameLevel::overGame()
	{
		if( mIsOver )
			return;

		mIsOver = true;
		if( onGameOver )
			onGameOver(*this);
	}

}//namespace FlappyBird