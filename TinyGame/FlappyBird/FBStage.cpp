#include "TinyGamePCH.h"
#include "FBStage.h"

#include "RenderUtility.h"
#include "GameGUISystem.h"
#include "Widget/WidgetUtility.h"
#include "DrawEngine.h"

#include "TrainManager.h"

#include "SystemPlatform.h"

#include "RHI/RHICommand.h"
#include "GLGraphics2D.h"

namespace FlappyBird
{
	using namespace Render;

	LevelStage::LevelStage()
	{

	}

	LevelStage::~LevelStage()
	{

	}

	bool LevelStage::onInit()
	{
		::Global::GetDrawEngine().startOpenGL();

		::Global::GUI().cleanupWidget();

		if( !loadResource() )
			return false;

		::srand( SystemPlatform::GetTickCount() );
		getLevel().onBirdCollsion = CollisionDelegate(this, &LevelStage::notifyBridCollsion);
		getLevel().onGameOver = LevelOverDelegate(this, &LevelStage::notifyGameOver);

#define INPUT_MODE 0

#if INPUT_MODE == 0
		int gDefaultTopology[] = { 3 , 5 , 7 , 5 , 3 , 1 };
#elif INPUT_MODE == 1
		int gDefaultTopology[] = { 5 , 7 , 4 , 1 };
#else
		int gDefaultTopology[] = { 2 , 4 , 1 };
#endif
		if( mbTrainMode )
		{
			mTrainManager = std::make_unique<TrainManager>();

			float mutationValueDelta = 0.7;
			TrainWorkSetting setting;
			setting.numWorker = 8;
			setting.maxGeneration = 0;

			mTrainManager->init(setting, gDefaultTopology , ARRAY_SIZE(gDefaultTopology));
			mTrainManager->startTrain();

			TrainDataSetting& dataSetting = mTrainManager->getDataSetting();
			dataSetting.numAgents = 200;
			dataSetting.mutationValueDelta = mutationValueDelta;
			mTrainData = std::make_unique< TrainData >();
			mTrainData->init(dataSetting);
			mTrainData->addAgentToLevel(getLevel());
		}
		else
		{
			getLevel().addBird(mBird);
		}

		mMaxScore = 0;
		restart();

		DevFrame* frame = WidgetUtility::CreateDevFrame();

		if( mbTrainMode )
		{
			frame->addButton("Toggle Fast Tick", [&](int event, GWidget* widget)
			{
				mbFastTick = !mbFastTick;
				return false;
			});
			frame->addButton("Save Train Data", [&](int event, GWidget* widget)
			{
				if( mTrainManager )
				{
					mTrainManager->saveData("genepool_001.cpr", *mTrainData);
				}
				return false;
			});
			frame->addButton("Load Train Data", [&](int event, GWidget* widget)
			{
				if( mTrainManager )
				{
					removeTrainData();
					mTrainManager->loadData("genepool_001.cpr", *mTrainData);
					mTrainData->addAgentToLevel(getLevel());
					restart();
				}
				return false;
			});
			frame->addButton("Use Pool Data", [&](int event, GWidget* widget)
			{			
				if( mTrainManager )
				{
					getLevel().removeAllBird();
					TLockedObject< GenePool > pGenePool = mTrainManager->lockPool();
					mTrainManager->getDataSetting().numAgents = pGenePool->getDataSet().size();
					mTrainData->usePoolData(*pGenePool);
					mTrainData->addAgentToLevel(getLevel());
					restart();
				}
				return false;
			});		
		}
		return true;
	}

	void LevelStage::onUpdate(long time)
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		if( mbFastTick && frame == 1 )
			frame *= 20;

		for( int i = 0; i < frame; ++i )
			tick();

		updateFrame(frame);
	}

	bool LevelStage::loadResource()
	{
		using namespace Render;

		char const* fileName[TextureID::Count] =
		{
			"Bird.png" ,
			"Pipe.png",
			"BG01.png",
			"BG02.png",
			"Ground.png",
			"BigNum.png"
		};
		for( int i = 0; i < TextureID::Count; ++i )
		{
			FixString<128> path;
			path.format("FlappyBird/%s", fileName[i]);
			mTextures[i] = RHIUtility::LoadTexture2DFromFile(path);
			if( !mTextures[i].isValid() )
				return false;

			Render::OpenGLCast::To(mTextures[i])->bind();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			Render::OpenGLCast::To(mTextures[i])->unbind();
		}

		return true;
	}

	void LevelStage::restart()
	{
		getLevel().restart();
		if ( mTrainData )
			mTrainData->restart();
		mScore = 0;

		if( rand() % 2 )
		{
			blockType = 1;
			backgroundType = 1;
		}
		else
		{
			blockType = 0;
			backgroundType = 0;
		}

	}

	void LevelStage::tick()
	{
		if( mTrainData )
			mTrainData->tick();

		getLevel().tick();

		if( mbTrainMode )
		{
#if 0
			TLockedObject< GenePool > genePool = mTrainManager->lockPool();
			if( !genePool->mStorage.empty() )
			{
				topFitness = (*genePool)[0]->fitness;
			}
#endif
		}
	}

	bool LevelStage::onWidgetEvent(int event, int id, GWidget* ui)
	{
		switch( id )
		{
		case UI_OK:
			restart();
			return false;
		}
		return true;
	}

	bool LevelStage::onMouse(MouseMsg const& msg)
	{
		if( !BaseClass::onMouse(msg) )
			return false;

		if( msg.onLeftDown() || msg.onLeftDClick() )
		{
			if( !getLevel().mIsOver )
				mBird.fly();
		}

		return false;
	}

	bool LevelStage::onKey(unsigned key, bool isDown)
	{
		if( !isDown )
			return false;

		switch( key )
		{
		case Keyboard::eR: restart(); break;
		case 'S':
			if( !getLevel().mIsOver )
				mBird.fly();
			break;
		}
		return false;
	}

	void LevelStage::notifyGameOver(GameLevel& level)
	{
		assert(&level == &mLevel);
		if( mbTrainMode )
		{
			int64 time;
			if( bRunPoolOnly )
			{
				ScopeTickCount scope(time);
				getLevel().removeAllBird();
				TLockedObject< GenePool > pGenePool = mTrainManager->lockPool();
				mTrainManager->getDataSetting().numAgents = pGenePool->getDataSet().size();
				mTrainData->usePoolData(*pGenePool);
				mTrainData->addAgentToLevel(getLevel());
			}
			else
			{
				ScopeTickCount scope(time);
				TLockedObject< GenePool > genePool = mTrainManager->lockPool();
				mTrainData->runEvolution(genePool);
				if( !genePool->mStorage.empty() )
					mTrainManager->topFitness = (*genePool)[0]->fitness;

			}
			LogMsg("lock time = %ld", time);
			restart();
		}
		else
		{
			if( mScore > mMaxScore )
				mMaxScore = mScore;

			GMsgBox* box = new GMsgBox(UI_ANY, Vec2i(0, 0), NULL, GMB_OK);

			Vec2i pos = GUISystem::calcScreenCenterPos(box->getSize());
			box->setPos(pos);
			box->setTitle("You Die!");
			::Global::GUI().addWidget(box);
		}
	}

	FlappyBird::CollisionResponse LevelStage::notifyBridCollsion(BirdEntity& bird, ColObject& obj)
	{
		switch( obj.type )
		{
		case CT_PIPE_BOTTOM:
		case CT_PIPE_TOP:
			if( mbTrainMode )
			{
				bird.bActive = false;
			}
			else
			{
				getLevel().overGame();
			}
			break;
		case CT_SCORE:
			if( mbTrainMode )
			{
				++mScore;
			}
			else
			{
				++mScore;
			}
			break;
		}
		return GetDefaultColTypeResponse(obj.type);
	}

	void LevelStage::removeTrainData()
	{
		assert(mTrainManager);
		mTrainData->clearData();
		mTrainManager->clearData();
		getLevel().removeAllBird();
	}

	void LevelStage::drawTexture(int id, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vec2i const& framePos, Vec2i const& frameDim)
	{
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		{
			GL_BIND_LOCK_OBJECT(mTextures[id]);
			DrawUtility::Sprite(RHICommandList::GetImmediateList(), pos, size, pivot, framePos, frameDim);
		}
		glDisable(GL_TEXTURE_2D);
	}

	void LevelStage::drawTexture(int id, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vector2 const& texPos, Vector2 const& texSize)
	{
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		{
			GL_BIND_LOCK_OBJECT(mTextures[id]);
			DrawUtility::Sprite(RHICommandList::GetImmediateList(), pos, size, pivot, texPos, texSize);
		}
		glDisable(GL_TEXTURE_2D);
		
	}

	void LevelStage::drawTexture(int id, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot)
	{
		drawTexture(id, pos, size, pivot, Vector2(0, 0), Vector2(1, 1));
	}

	void LevelStage::onRender(float dFrame)
	{
		IGraphics2D& g = Global::GetIGraphics2D();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		Vec2i screenSize = ::Global::GetDrawEngine().getScreenSize();
		glViewport(0, 0, screenSize.x, screenSize.y);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, screenSize.x, screenSize.y, 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		if ( mbDebugDraw )
		{

			RenderUtility::SetBrush(g, EColor::Gray);
			g.drawRect(Vec2i(0, 0), ::Global::GetDrawEngine().getScreenSize());

			RenderUtility::SetBrush(g, EColor::Gray, COLOR_LIGHT);
			g.drawRect(convertToScreen(Vector2(0, WorldHeight)), GScale * Vector2(WorldWidth, WorldHeight));
		}

		glColor3f(1, 1, 1);
		{
			int BGTextureId[] = { TextureID::Background_A  , TextureID::Background_B };
			Vector2 rPos = convertToScreen( Vector2(0.5 *WorldWidth, 0.5 *WorldHeight - 1.6));
			float texFactor = getTextureSizeRatio(BGTextureId[backgroundType]);

			Vector2 rSize;
			rSize.x = WorldWidth * GScale;
			rSize.y = rSize.x / texFactor;

			drawTexture(BGTextureId[backgroundType], rPos , rSize , Vector2(0.5, 0.5), Vector2(getLevel().mMoveDistance / ( 10 * WorldWidth ), 0), Vector2(1, 1));
		}
		

		for( auto iter = getLevel().getColObjects(); iter ; ++iter )
		{
			ColObject const& obj = *iter;

			switch( obj.type )
			{
			case CT_PIPE_BOTTOM:
			case CT_PIPE_TOP:
				drawPipe(g,obj);
				break;
			}
		}



		{
			int texId = TextureID::Ground;
			Vector2 rPos = convertToScreen( Vector2( 0 , 0));
			float texFactor = getTextureSizeRatio(texId);

			Vector2 rSize;
			rSize.x = WorldWidth * GScale;
			rSize.y = rSize.x / texFactor;
			drawTexture(texId, rPos , rSize, Vector2(0, 0) , Vector2(getLevel().mMoveDistance / WorldWidth, 0), Vector2(1, 1) );
		}
		if( mbTrainMode )
		{
			for( auto& pAgent : mTrainData->mAgents )
			{
				BirdEntity& bird = pAgent->hostBird;
				if( bird.bActive || bird.getPos().x > -BirdRadius )
					drawBird(g, bird);
			}
		}
		else
		{
			drawBird(g, mBird);
		}

		{
			Vector2 rPos = convertToScreen(Vector2(0.5 * WorldWidth, 10));
			glPushMatrix();
			
			glTranslatef(rPos.x, rPos.y, 0);
			drawNumber(g, mScore, 20 );
			glPopMatrix();
		}


		{
			RenderUtility::SetPen(g, EColor::Null);
			RenderUtility::SetBrush(g, EColor::Gray);
			Vector2 holePos = convertToScreen(Vector2(0, WorldHeight - 1));
			Vector2 holeSize = GScale * Vector2(WorldWidth, WorldHeight);
			drawScreenHole(g, holePos, holeSize);
		}


		if( mbTrainMode )
		{

			if( mTrainData->curBestAgent )
			{
				FCNeuralNetwork& FNN = mTrainData->curBestAgent->FNN;
				NeuralNetworkRenderer renderer(FNN);
				renderer.basePos = Vector2(400, 300);
				renderer.inputsAndSignals = &mTrainData->bestInputsAndSignals[0];
				renderer.draw(g);
			}

			for( auto& block : getLevel().mPipes )
			{

			}

			Vec2i startPos(600 - 50, 20);
			FixString< 128 > str;
			str.format("Generation = %d", mTrainData->generation);
			g.drawText(startPos, str);

			float curBestFitness = (mTrainData->curBestAgent) ? mTrainData->curBestAgent->lifeTime : 0;
			str.format("TopFitness = %.3f , Fitness = %.3f  ", mTrainManager->topFitness , curBestFitness );
			g.drawText(startPos + Vec2i(0, 15), str);

			str.format("Alive num = %u", getLevel().mActiveBirds.size());
			g.drawText(startPos + Vec2i(0, 2*15), str);
		}
		else
		{
			FixString< 128 > str;
			str.format("Count = %d", mScore);
			g.drawText(Vec2i(10, 10), str);
		}

	}

	Vector2 LevelStage::convertToScreen(Vector2 const& pos)
	{
		if( mbTrainMode )
			return Vector2(30 + GScale * pos.x, 520 - GScale * pos.y);

		return Vector2(170 + GScale * pos.x, 520 - GScale * pos.y);
	}

	void LevelStage::drawScreenHole(IGraphics2D& g, Vec2i const& pos, Vec2i const& size)
	{
		Vec2i screenSize = ::Global::GetDrawEngine().getScreenSize();

		if( pos.y > 0 )
		{
			g.drawRect(Vec2i(0, 0), Vec2i(screenSize.x, pos.y ));
		}
		if( pos.x > 0 )
		{
			g.drawRect(Vec2i(0, pos.y), Vec2i(pos.x , size.y));
		}

		Vec2i pt = pos + size;
		if( pt.x < screenSize.x )
		{
			g.drawRect(Vec2i(pt.x, pos.y), Vec2i(screenSize.x - pt.x, size.y));
		}
		if( pt.y < screenSize.y )
		{
			g.drawRect(Vec2i(0, pt.y), Vec2i(screenSize.x, screenSize.y - pt.y));
		}
	}

	void LevelStage::drawBird(IGraphics2D& g, BirdEntity& bird)
	{
		RHICommandList& commandList = RHICommandList::GetImmediateList();

		Vec2i rPos = convertToScreen(bird.getPos());
		float size = BirdRadius * GScale;
		Vector2 dir = Vector2(BirdVelX, -bird.getVel());

		if ( mbDebugDraw )
		{
			
			RenderUtility::SetPen(g, EColor::Black);
			RenderUtility::SetBrush(g, EColor::Yellow);
			g.drawCircle(rPos, size);
			RenderUtility::SetPen(g, EColor::Red);
			
			dir /= sqrt(dir.length2());
			g.drawLine(rPos, rPos + Vec2i(15 * dir));
		}
		else
		{
			int birdType = bird.randValue % 3;

			float angle;
#if 1
			if( bird.getVel() > 0.5 * bird.flyVel )
			{
				angle = Math::Lerp(0, 30, Math::Clamp((bird.getVel() / bird.flyVel - 0.5f) / (0.3f), 0.0f, 1.0f));
			}
			else if( bird.getVel() < -1.0 * bird.flyVel )
			{
				angle = Math::Lerp(0, -90, Math::Clamp((-bird.getVel() / bird.flyVel - 1.0f) / 2, 0.0f, 1.0f));
			}
			else
			{
				angle = 0;
			}
#else
			float theta = Math::ATan2(dir.y, dir.x);
			angle = Math::Rad2Deg(theta);
#endif

			float sizeFactor = getTextureSizeRatio(TextureID::Bird);
			size *= 2;

			int const frameNum = 3;

			float frame = Math::FloorToInt( ( bird.bActive ? getLevel().mMoveDistance * 6 : 0 ) + 4 * float( bird.randValue ) / RAND_MAX ) % 4;
			if( frame >= frameNum )
				frame = frameNum - frame + 1;
			
			RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA, Blend::eSrcAlpha, Blend::eOneMinusSrcAlpha>::GetRHI());
			glPushMatrix();
			{
				glTranslatef(rPos.x, rPos.y, 0);
				glRotatef(-angle, 0, 0, 1);
				drawTexture(TextureID::Bird, Vector2(0,0), Vector2(size * sizeFactor , size ), Vector2(0.5, 0.5), Vec2i( frame , birdType), Vec2i(3, 3));
			}
			glPopMatrix();
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		}
	}



	void LevelStage::drawPipe(IGraphics2D& g, ColObject const& obj)
	{
		RHICommandList& commandList = RHICommandList::GetImmediateList();

		Vector2 rPos = convertToScreen(obj.pos);
		
		Vector2 rSize = Vector2(GScale * obj.size);
		rPos.y -= rSize.y;
		if( mbDebugDraw )
		{
			RenderUtility::SetBrush(g, EColor::Cyan);
			RenderUtility::SetPen(g, EColor::Black);
			g.drawRect(rPos, rSize);
		}
		else
		{
			int typeNum = 2;
			int type = blockType;
			float texFactor = 160.0 / 26;
			Vector2 texPos = Vector2(type * (1.0 / typeNum), 0);
			Vector2 texSize = Vector2((1.0 / typeNum), rSize.y / (texFactor * rSize.x));
			if( obj.type == CT_PIPE_TOP )
			{
				texPos.y += texSize.y;
				texSize.y = -texSize.y;
			}
			RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA , Blend::eSrcAlpha , Blend::eOneMinusSrcAlpha>::GetRHI());
			drawTexture(TextureID::Pipe, rPos , rSize, Vector2(0, 0), texPos, texSize);
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

		}
	}

	void LevelStage::drawNumber(IGraphics2D& g, int number, float width )
	{
		RHICommandList& commandList = RHICommandList::GetImmediateList();
		int digials[32];
		int numDigial = 0;
		do
		{
			digials[numDigial++] = number % 10;
			number /= 10;
		}
		while( number );

		float offset = 0.5 * width * (numDigial - 1);

		Vector2 size = Vector2(width, 10 * width / getTextureSizeRatio(TextureID::Number));

		RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA, Blend::eSrcAlpha, Blend::eOneMinusSrcAlpha>::GetRHI());
		for( int i = 0; i < numDigial; ++i )
		{
			drawTexture(TextureID::Number, Vector2(offset - i * width , 0 ),  size, 
						Vector2(0.5, 0.5), Vec2i(digials[i] , 0 ), Vec2i(10, 1));
		}
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
	}

}//namespace FlappyBird