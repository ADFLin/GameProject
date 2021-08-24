#include "TinyGamePCH.h"
#include "FBStage.h"

#include "RenderUtility.h"
#include "GameGUISystem.h"
#include "Widget/WidgetUtility.h"
#include "DrawEngine.h"

#include "TrainManager.h"

#include "SystemPlatform.h"

#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"
#include "Image/ImageData.h"
#include "ConsoleSystem.h"

#define USE_TEXTURE_ATLAS 0

namespace FlappyBird
{
	using namespace Render;

	REGISTER_STAGE_ENTRY("FlappyBird Test", LevelStage, EExecGroup::Dev, 2, "Game|AI");

	class AgentBird : public AgentEntity
		            , public IBirdController
	{
	public:
		AgentBird(Agent& agent)
		{
			mAgent = &agent;
			agent.entity = this;
		}
		//IBirdController
		void updateInput(GameLevel& world, BirdEntity& bird) override
		{
			assert(&hostBird == &bird);
			NNScale inputs[8];
			std::fill_n(inputs, ARRAY_SIZE(inputs), NNScale(0));
			assert(ARRAY_SIZE(inputs) >= mAgent->FNN.getLayout().getInputNum());

			switch (mAgent->FNN.getLayout().getInputNum())
			{
			case 2:
				{
					inputs[0] = 1;
					if (!world.mPipes.empty())
					{
						PipeInfo& pipe = world.mPipes[0];
						getPipeInputs(inputs, pipe);
					}
				}
				break;
			case 3:
				{
					inputs[0] = 1;
					if (!world.mPipes.empty())
					{
						PipeInfo& pipe = world.mPipes[0];
						getPipeInputs(inputs, pipe);
					}
					inputs[2] = bird.getVel() / WorldHeight;
				}
				break;
			case 4:
				{
					inputs[0] = inputs[2] = 1;
					if (!world.mPipes.empty())
					{
						PipeInfo& pipe = world.mPipes[0];
						getPipeInputs(inputs, pipe);

						if (world.mPipes.size() >= 2)
						{
							PipeInfo& pipe = world.mPipes[1];
							getPipeInputs(&inputs[2], pipe);
						}
					}
				}
				break;
			case 5:
				{
					inputs[0] = inputs[2] = 1;
					if (!world.mPipes.empty())
					{
						PipeInfo& pipe = world.mPipes[0];
						getPipeInputs(inputs, pipe);

						if (pipe.posX - BirdRadius <= bird.getPos().x && bird.getPos().x <= pipe.posX + pipe.width + BirdRadius)
							inputs[4] = 1;

						if (world.mPipes.size() >= 2)
						{
							PipeInfo& pipe = world.mPipes[1];
							getPipeInputs(&inputs[2], pipe);
						}
					}
				}
				break;
			default:
				break;
			}

			NNScale output;
			mAgent->FNN.calcForwardFeedback(inputs, &output);
			if (output >= 0.5)
			{
				bird.fly();
			}

			if (mAgent->inputsAndSignals)
			{
				std::copy(inputs, inputs + mAgent->FNN.getLayout().getInputNum(), mAgent->inputsAndSignals);
				mAgent->FNN.calcForwardFeedbackSignal(inputs, mAgent->inputsAndSignals + mAgent->FNN.getLayout().getInputNum());
			}
		}
		void getPipeInputs(NNScale inputs[], PipeInfo const& pipe)
		{
			inputs[0] = (pipe.posX - hostBird.getPos().x) / WorldHeight;
			inputs[1] = (0.5 * (pipe.topHeight + pipe.buttonHeight) - hostBird.getPos().y) / WorldHeight;
		}

		void release() override
		{
			delete this;
		}

		void tick()
		{
			if (bCompleted)
				return;

			if (!hostBird.bActive)
			{
				mAgent->genotype->fitness = lifeTime;
				bCompleted = true;
				return;
			}

			lifeTime += TickTime;
			mAgent->genotype->fitness = lifeTime;
		}

		void restart()
		{
			bCompleted = false;
			lifeTime = 0;
		}

		bool       bCompleted = false;
		float      lifeTime = 0;
		Agent*     mAgent;
		BirdEntity hostBird;
	};

	class AgentBirdWorld : public AgentWorld
	{
	public:
		void setup(TrainData& trainData) override
		{
			SpawnAgents(mLevel, trainData);
			mLevel.onBirdCollsion = CollisionDelegate(this, &AgentBirdWorld::handleCollision);
			mLevel.onGameOver = LevelOverDelegate(this, &AgentBirdWorld::handleGameOver);
		}

		void restart(TrainData& trainData) override
		{
			RestartAgents(trainData);
			mLevel.restart();
		}

		void tick(TrainData& trainData) override
		{
			TickAgents(trainData);
			mLevel.tick();
		}

		void handleGameOver(GameLevel& level)
		{
			onTrainCompleted();
		}

		EColResponse handleCollision(BirdEntity& bird, ColObject& obj)
		{
			switch (obj.type)
			{
			case CT_PIPE_BOTTOM:
			case CT_PIPE_TOP:
				bird.bActive = false;
				break;
			case CT_SCORE:
				break;
			}
			return GetDefaultColTypeResponse(obj.type);
		}

		static AgentBird* GetEntity(std::unique_ptr<Agent> const& agentPtr)
		{
			return static_cast<AgentBird*>(agentPtr->entity);
		}

		static void SpawnAgents(GameLevel& level , TrainData& trainData)
		{
			for (auto& agentPtr : trainData.mAgents)
			{
				if (agentPtr->entity == nullptr)
				{
					auto entity = new AgentBird(*agentPtr);
					level.addBird(entity->hostBird, entity);
				}
				else
				{
					auto* entity = GetEntity(agentPtr);
					level.addBird(entity->hostBird, entity);
				}
			}
		}

		static void TickAgents(TrainData& trainData)
		{
			for (auto& agentPtr : trainData.mAgents)
			{
				auto* entity = GetEntity(agentPtr);
				entity->tick();
			}
		}

		static void RestartAgents(TrainData& trainData)
		{
			for (auto& agentPtr : trainData.mAgents)
			{
				auto* entity = GetEntity(agentPtr);
				entity->restart();
			}
		}



		GameLevel mLevel;

		void release() override
		{
			delete this;
		}
	};

	LevelStage::LevelStage()
	{

	}

	LevelStage::~LevelStage()
	{

	}

	bool LevelStage::onInit()
	{
		::Global::GUI().cleanupWidget();

		::srand(SystemPlatform::GetTickCount());
		getLevel().onBirdCollsion = CollisionDelegate(this, &LevelStage::notifyBridCollsion);
		getLevel().onGameOver = LevelOverDelegate(this, &LevelStage::notifyGameOver);

#define INPUT_MODE 0
		int gDefaultTopology[] =
		{
#if INPUT_MODE == 0
			3 , 5 , 7 , 5 , 3 , 1 
#elif INPUT_MODE == 1
			5, 7, 4, 1 
#else
			2, 4, 1 
#endif
		};

		//mbTrainMode = false;
		if( mbTrainMode )
		{
			mTrainManager = std::make_unique<TrainManager>();

			float mutationValueDelta = 0.7;
			TrainWorkSetting setting;
			setting.numWorker = 8;
			setting.maxGeneration = 0;
			setting.worldFactory = []() -> AgentWorld*
			{
				return new AgentBirdWorld;
			};

			mTrainManager->init(setting, gDefaultTopology , ARRAY_SIZE(gDefaultTopology));
			mTrainManager->startTrain();

			TrainDataSetting& dataSetting = mTrainManager->getDataSetting();
			dataSetting.numAgents = 200;
			dataSetting.mutationValueDelta = mutationValueDelta;
			mTrainData = std::make_unique< TrainData >();
			mTrainData->init(dataSetting);
			AgentBirdWorld::SpawnAgents(mLevel, *mTrainData);
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
					AgentBirdWorld::SpawnAgents(mLevel, *mTrainData);
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
					AgentBirdWorld::SpawnAgents(mLevel, *mTrainData);
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
#if USE_TEXTURE_ATLAS
		mTextureAtlas.initialize(ETexture::RGBA8, 1024, 1024, 2);
#endif

		for( int i = 0; i < TextureID::Count; ++i )
		{
			InlineString<128> path;
			path.format("FlappyBird/%s", fileName[i]);

			ImageInfo& image = mImages[i];

#if USE_TEXTURE_ATLAS
			int id = mTextureAtlas.addImageFile(path);
			if (id == INDEX_NONE)
				return false;

			mTextureAtlas.getRectUVChecked(id, image.uvPos, image.uvSize);
			image.uvSize -= image.uvPos;
			image.size = mTextureAtlas.getRectSizeChecked(id);
#else
			mImages[i].texture = RHIUtility::LoadTexture2DFromFile(path);
			if( !mImages[i].texture )
				return false;
#endif
		}

		return true;
	}

	void LevelStage::restart()
	{
		if (mTrainData)
		{
			AgentBirdWorld::RestartAgents(*mTrainData);
		}

		getLevel().restart();

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
		if (mTrainData)
		{
			AgentBirdWorld::TickAgents(*mTrainData);
			mTrainData->findBestAgnet();
		}

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

	ERenderSystem LevelStage::getDefaultRenderSystem()
	{
		return ERenderSystem::D3D11;
	}

	bool LevelStage::setupRenderSystem(ERenderSystem systemName)
	{
		VERIFY_RETURN_FALSE(loadResource());
		return true;
	}

	void LevelStage::preShutdownRenderSystem(bool bReInit /*= false*/)
	{
#if USE_TEXTURE_ATLAS
		mTextureAtlas.finalize();
#else
		for (int i = 0; i < TextureID::Count; ++i)
		{
			ImageInfo& image = mImages[i];
			image.texture.release();
		}
#endif
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

	bool LevelStage::onKey(KeyMsg const& msg)
	{
		if( !msg.isDown() )
			return false;

		switch(msg.getCode())
		{
		case EKeyCode::R: restart(); break;
		case EKeyCode::S:
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
				AgentBirdWorld::SpawnAgents(getLevel(), *mTrainData);
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

			Global::GUI().showMessageBox(UI_ANY, "You Die!", GMB_OK);
		}
	}

	EColResponse LevelStage::notifyBridCollsion(BirdEntity& bird, ColObject& obj)
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
		RHICommandList& commandList = RHICommandList::GetImmediateList();

#if USE_TEXTURE_ATLAS
		ImageInfo const& image = mImages[id];
		RHISetFixedShaderPipelineState(commandList, mXFormStack.get().toMatrix4() * mBaseTransform, LinearColor(1, 1, 1, 1), &mTextureAtlas.getTexture(), &TStaticSamplerState< ESampler::Point >::GetRHI());
		DrawUtility::Sprite(RHICommandList::GetImmediateList(), pos, size, pivot, image.uvPos , image.uvSize,  framePos, frameDim);
#else
		RHISetFixedShaderPipelineState(commandList, mXFormStack.get().toMatrix4() * mBaseTransform, LinearColor(1, 1, 1, 1), mImages[id].texture, &TStaticSamplerState< ESampler::Point >::GetRHI());
		DrawUtility::Sprite(commandList, pos, size, pivot, framePos, frameDim);
#endif

	}

	void LevelStage::drawTexture(int id, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot, Vector2 const& texPos, Vector2 const& texSize)
	{
		RHICommandList& commandList = RHICommandList::GetImmediateList();

#if USE_TEXTURE_ATLAS
		ImageInfo const& image = mImages[id];
		Vector2 uvPos = image.uvPos + texPos.mul(image.uvSize);
		Vector2 uvSize = image.uvSize.mul( texSize );
		RHISetFixedShaderPipelineState(commandList, mXFormStack.get().toMatrix4() * mBaseTransform, LinearColor(1, 1, 1, 1), &mTextureAtlas.getTexture(), &TStaticSamplerState< ESampler::Point >::GetRHI());
		DrawUtility::Sprite(RHICommandList::GetImmediateList(), pos, size, pivot, uvPos, uvSize);
#else
		RHISetFixedShaderPipelineState(commandList, mXFormStack.get().toMatrix4() * mBaseTransform, LinearColor(1, 1, 1, 1), mImages[id].texture, &TStaticSamplerState< ESampler::Point >::GetRHI());
		DrawUtility::Sprite(commandList, pos, size, pivot, texPos, texSize);
#endif

		
	}

	void LevelStage::drawTexture(int id, Vector2 const& pos, Vector2 const& size, Vector2 const& pivot)
	{
		drawTexture(id, pos, size, pivot, Vector2(0, 0), Vector2(1, 1));
	}

	void LevelStage::onRender(float dFrame)
	{
		IGraphics2D& g = Global::GetIGraphics2D();

		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &LinearColor(0.2, 0.2, 0.2), 1);

		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
		RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());

		Vec2i screenSize = ::Global::GetScreenSize();
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

		mBaseTransform = OrthoMatrix(0, screenSize.x, screenSize.y, 0, -1, 1);

		if ( mbDebugDraw )
		{

			RenderUtility::SetBrush(g, EColor::Gray);
			g.drawRect(Vec2i(0, 0), ::Global::GetScreenSize());

			RenderUtility::SetBrush(g, EColor::Gray, COLOR_LIGHT);
			g.drawRect(convertToScreen(Vector2(0, WorldHeight)), GScale * Vector2(WorldWidth, WorldHeight));
		}

		{
			int BGTextureId[] = { TextureID::Background_A  , TextureID::Background_B };
			Vector2 rPos = convertToScreen( Vector2(0.5 *WorldWidth, 0.5 *WorldHeight - 1.6));
			float texFactor = getImageSizeRatio(BGTextureId[backgroundType]);

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
			float texFactor = getImageSizeRatio(texId);

			Vector2 rSize;
			rSize.x = WorldWidth * GScale;
			rSize.y = rSize.x / texFactor;
			drawTexture(texId, rPos , rSize, Vector2(0, 0) , Vector2(getLevel().mMoveDistance / WorldWidth, 0), Vector2(1, 1) );
		}
		if( mbTrainMode )
		{
			for( auto& pAgent : mTrainData->mAgents )
			{
				BirdEntity& bird = static_cast< AgentBird* >( pAgent->entity )->hostBird;
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
			mXFormStack.push();
			mXFormStack.translate(rPos);
			drawNumber(g, mScore, 20 );
			mXFormStack.pop();

		}


		{
			RenderUtility::SetPen(g, EColor::Null);
			RenderUtility::SetBrush(g, EColor::Gray);
			Vector2 holePos = convertToScreen(Vector2(0, WorldHeight - 1));
			Vector2 holeSize = GScale * Vector2(WorldWidth, WorldHeight);
			drawScreenHole(g, holePos, holeSize);
		}

		g.beginRender();

		if( mbTrainMode )
		{

			if( mTrainData->bestAgent )
			{
				FCNeuralNetwork& FNN = mTrainData->bestAgent->FNN;
				NeuralNetworkRenderer renderer(FNN);
				renderer.basePos = Vector2(400, 300);
				renderer.inputsAndSignals = &mTrainData->bestInputsAndSignals[0];
				renderer.draw(g);
			}

			for( auto& block : getLevel().mPipes )
			{

			}

			Vec2i startPos(20, 20);
			InlineString< 128 > str;
			str.format("Generation = %d", mTrainData->generation);
			g.drawText(startPos, str);

			float curBestFitness = (mTrainData->bestAgent) ? mTrainData->bestAgent->genotype->fitness : 0;
			str.format("TopFitness = %.3f , Fitness = %.3f  ", mTrainManager->topFitness , curBestFitness );
			g.drawText(startPos + Vec2i(0, 15), str);

			str.format("Alive num = %u", getLevel().mActiveBirds.size());
			g.drawText(startPos + Vec2i(0, 2*15), str);
		}
		else
		{
			InlineString< 128 > str;
			str.format("Count = %d", mScore);
			g.drawText(Vec2i(10, 10), str);
		}

		g.endRender();

	}

	Vector2 LevelStage::convertToScreen(Vector2 const& pos)
	{
		if( mbTrainMode )
			return Vector2(30 + GScale * pos.x, 520 - GScale * pos.y);

		return Vector2(170 + GScale * pos.x, 520 - GScale * pos.y);
	}

	void LevelStage::drawScreenHole(IGraphics2D& g, Vec2i const& pos, Vec2i const& size)
	{
		Vec2i screenSize = ::Global::GetScreenSize();

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

			float sizeFactor = getImageSizeRatio(TextureID::Bird);
			size *= 2;

			int const frameNum = 3;

			float frame = Math::FloorToInt( ( bird.bActive ? getLevel().mMoveDistance * 6 : 0 ) + 4 * float( bird.randValue ) / RAND_MAX ) % 4;
			if( frame >= frameNum )
				frame = frameNum - frame + 1;
			
			RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA, EBlend::SrcAlpha, EBlend::OneMinusSrcAlpha>::GetRHI());
			mXFormStack.push();
			{
				mXFormStack.translate(rPos);
				mXFormStack.rotate(Math::Deg2Rad(-angle));
				drawTexture(TextureID::Bird, Vector2(0,0), Vector2(size * sizeFactor , size ), Vector2(0.5, 0.5), Vec2i( frame , birdType), Vec2i(3, 3));
			}
			mXFormStack.pop();
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
			RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA , EBlend::SrcAlpha , EBlend::OneMinusSrcAlpha>::GetRHI());
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

		Vector2 size = Vector2(width, 10 * width / getImageSizeRatio(TextureID::Number));

		RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA, EBlend::SrcAlpha, EBlend::OneMinusSrcAlpha>::GetRHI());
		for( int i = 0; i < numDigial; ++i )
		{
			drawTexture(TextureID::Number, Vector2(offset - i * width , 0 ),  size, 
						Vector2(0.5, 0.5), Vec2i(digials[i] , 0 ), Vec2i(10, 1));
		}
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
	}


	float LevelStage::getImageSizeRatio(int id)
	{
#if USE_TEXTURE_ATLAS
		return float(mImages[id].size.x / mImages[id].size.y);
#else
		return float(mImages[id].texture->getSizeX()) / mImages[id].texture->getSizeY();
#endif
	}

}//namespace FlappyBird