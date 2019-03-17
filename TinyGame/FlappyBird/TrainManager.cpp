#include "TrainManager.h"

#include "Serialize/FileStream.h"
#include "SystemPlatform.h"

#include <algorithm>

#define INPUT_MODE 0

namespace FlappyBird
{
#if INPUT_MODE == 0
	int gDefaultTopology[] = { 3 , 5 , 7 , 5 , 3 , 1 };
#elif INPUT_MODE == 1
	int gDefaultTopology[] = { 5 , 7 , 4 , 1 };
#else
	int gDefaultTopology[] = { 2 , 4 , 1 };
#endif


	void Agent::init( FCNNLayout& layout )
	{
		FNN.init(layout);
		GenotypePtr pGT = std::make_shared<Genotype>();
		pGT->data.resize(FNN.getLayout().getWeightNum());
		setGenotype(pGT);
	}

	void Agent::setGenotype(GenotypePtr inGenotype)
	{
		genotype = inGenotype;
		FNN.setWeights(inGenotype->data);
	}

	void Agent::restart()
	{
		bCompleted = false;
		lifeTime = 0;
		hostBird.reset();
	}

	void Agent::tick()
	{
		if( bCompleted )
			return;

		if( !hostBird.bActive )
		{
			genotype->fitness = lifeTime;
			bCompleted = true;
			return;
		}

		lifeTime += TickTime;
		genotype->fitness = lifeTime;
	}

	void Agent::randomizeData(NNRand& random)
	{
		genotype->randomizeData(random, -1, 1);
	}

	void Agent::updateInput(GameLevel& world, BirdEntity& bird)
	{
		assert(&hostBird == &bird);
		NNScale inputs[8];
		std::fill_n(inputs, ARRAY_SIZE(inputs), NNScale(0));
		assert(ARRAY_SIZE(inputs) >= FNN.getLayout().getInputNum() );

		switch( FNN.getLayout().getInputNum() )
		{
		case 2:
			{
				inputs[0] = 1;
				if( !world.mPipes.empty() )
				{
					PipeInfo& pipe = world.mPipes[0];
					getPipeInputs(inputs, pipe);
				}
			}
			break;
		case 3:
			{
				inputs[0] = 1;
				if( !world.mPipes.empty() )
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
				if( !world.mPipes.empty() )
				{
					PipeInfo& pipe = world.mPipes[0];
					getPipeInputs(inputs, pipe);

					if( world.mPipes.size() >= 2 )
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
				if( !world.mPipes.empty() )
				{
					PipeInfo& pipe = world.mPipes[0];
					getPipeInputs(inputs, pipe);

					if( pipe.posX - BirdRadius <= bird.getPos().x && bird.getPos().x <= pipe.posX + pipe.width + BirdRadius )
						inputs[4] = 1;

					if( world.mPipes.size() >= 2 )
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
		FNN.calcForwardFeedback(inputs, &output);
		if( output >= 0.5 )
		{
			bird.fly();
		}

		if( inputsAndSignals )
		{
			std::copy(inputs, inputs + FNN.getLayout().getInputNum(), inputsAndSignals);
			FNN.calcForwardFeedbackSignal(inputs, inputsAndSignals + FNN.getLayout().getInputNum());
		}
	}

	void Agent::getPipeInputs(NNScale inputs[], PipeInfo const& pipe )
	{
		inputs[0] = (pipe.posX - hostBird.getPos().x) / WorldHeight;
		inputs[1] = (0.5 * (pipe.topHeight + pipe.buttonHeight) - hostBird.getPos().y) / WorldHeight;
	}

	void TrainData::init(TrainDataSetting const& inSetting)
	{
		setting = inSetting;
		mNNLayout.init(gDefaultTopology, ARRAY_SIZE(gDefaultTopology));

		assert(setting.numAgents > 0);
		generation = 0;
		for( int i = 0; i < setting.numAgents; ++i )
		{
			auto pAgent = std::make_unique< Agent >();
			pAgent->init( mNNLayout );
			pAgent->randomizeData(GA.mRand);
			mAgents.push_back(std::move(pAgent));
		}
		FCNeuralNetwork& FNN = mAgents[0]->FNN;
		bestInputsAndSignals.resize(FNN.getLayout().getInputNum() + FNN.getLayout().getHiddenNodeNum() + FNN.getLayout().getOutputNum());
	}

	void TrainData::tick()
	{
		for( auto& pAgent : mAgents )
		{
			pAgent->tick();
			if( curBestAgent == nullptr || curBestAgent->lifeTime < pAgent->lifeTime )
			{
				curBestAgent = pAgent.get();
				curBestAgent->inputsAndSignals = &bestInputsAndSignals[0];
			}
		}
	}

	void TrainData::restart()
	{
		for( auto& pAgent : mAgents )
		{
			pAgent->restart();
		}
	}

	void TrainData::usePoolData(GenePool& pool)
	{
		setting.numAgents = pool.getDataSet().size();
		if( mAgents.size() != setting.numAgents )
		{
			if( mAgents.size() > setting.numAgents )
			{
				mAgents.resize(pool.getDataSet().size());
			}
			else
			{
				int numNewAgents = setting.numAgents - mAgents.size();
				for( int i = 0; i < numNewAgents; ++i )
				{
					auto pAgent = std::make_unique< Agent >();
					pAgent->init( mNNLayout );
					mAgents.push_back(std::move(pAgent));
				}
			}
		}

		assert(pool.getDataSet().size() == mAgents.size());
		for( int i = 0; i < pool.getDataSet().size(); ++i )
		{
			mAgents[i]->setGenotype(pool[i]);
		}

		curBestAgent = nullptr;
	}

	void TrainData::runEvolution(GenePool* genePool)
	{
		++generation;
		int numGen = mAgents.size();
		std::vector< GenotypePtr > tempSelections;

		if( genePool && !genePool->mStorage.empty() )
		{
			int poolSelectionNum = std::min(setting.numPoolDataSelectUsed, (int)genePool->mStorage.size());
			for( int i = 0; i < poolSelectionNum; ++i )
			{
				tempSelections.push_back((*genePool)[i]);
			}
		}

		for( int i = 0; i < mAgents.size(); ++i )
		{
			tempSelections.push_back(mAgents[i]->genotype);
		}

		std::sort(tempSelections.begin(), tempSelections.end(),
				  [](GenotypePtr const& p1, GenotypePtr const& p2) -> bool
		{
			return p1->fitness > p2->fitness;
		});

		tempSelections.resize( setting.numTrainDataSelect );

		std::vector< GenotypePtr > selections;
		GA.select(setting.numTrainDataSelect, tempSelections, selections);
		
		std::vector< GenotypePtr > offsprings;
		GA.crossover(mAgents.size(), selections, offsprings);
		GA.mutate(offsprings, setting.mutationGeneProb ,setting.mutationValueProb, setting.mutationValueDelta);

		if ( genePool )
		{
			std::vector< GenotypePtr > temps;
			for( int i = 0; i < mAgents.size(); ++i )
			{
				temps.push_back(mAgents[i]->genotype);
				mAgents[i]->setGenotype(offsprings[i]);
			}

			float const fitnessError = 1e-3;
			NNScale const dataError = 1e-6;
			std::sort(temps.begin(), temps.end(), GenePool::FitnessCmp());
			GenePool::RemoveEqualGenotype(temps, fitnessError, dataError);
			for( int i = 0; i < temps.size(); ++i )
			{
				genePool->add(temps[i]);
			}
			genePool->removeEqual(fitnessError, dataError);
		}
		else
		{
			for( int i = 0; i < mAgents.size(); ++i )
			{
				mAgents[i]->setGenotype(offsprings[i]);
			}
		}
	}

	void TrainData::addAgentToLevel(GameLevel& level)
	{
		for( auto& pAgent : mAgents )
		{
			level.addBird(pAgent->hostBird , pAgent.get() );
		}
	}

	void TrainData::inputData(IStreamSerializer::ReadOp& op)
	{
		int numAgentData;
		op & numAgentData;

		if( numAgentData )
		{
			int dataSize;
			op & dataSize;
			std::vector<int> topology;
			op & topology;
			mNNLayout.init(&topology[0], topology.size());

			for( int i = 0; i < numAgentData; ++i )
			{
				GenotypePtr gt = std::make_shared<Genotype>();
				op & gt->fitness;
				op & gt->data;

				auto pAgent = std::make_unique< Agent >();
				pAgent->FNN.init(mNNLayout);
				pAgent->setGenotype(gt);
				mAgents.push_back(std::move(pAgent));
			}

		}
	}

	void TrainData::outputData(IStreamSerializer::WriteOp& op)
	{
		int numAgentData = mAgents.size();
		op & numAgentData;
		if( numAgentData )
		{
			int dataSize = mAgents[0]->genotype->data.size();
			op & dataSize;

			std::vector<int> topology;
			mAgents[0]->FNN.getLayout().getTopology(topology);
			op & topology;
			for( int i = 0; i < mAgents.size(); ++i )
			{
				Genotype* gt = mAgents[i]->genotype.get();
				op & gt->fitness;
				op & gt->data;
			}
		}
	}

	void TrainData::randomizeData()
	{
		for( auto& agent : mAgents )
		{
			agent->randomizeData(GA.mRand);
		}
	}

	void TrainData::clearData()
	{
		if( curBestAgent )
			curBestAgent->inputsAndSignals = nullptr;

		curBestAgent = nullptr;
		mAgents.clear();
	}

	void TrainWork::notifyGameOver(GameLevel&)
	{
		trainData.runEvolution(&genePool);

		bool const bRestData = maxGeneration && ( trainData.generation > maxGeneration );
		bool bSendData = bRestData || (SystemPlatform::GetTickCount() - lastUpdateTime > 10000);
		
		if( bSendData )
		{
			int numAdded;
			{
				float const fitnessError = 1e-3;
				NNScale const dataError = 1e-6;

				TLockedObject< GenePool > masterPool = manager->lockPool();
				numAdded = masterPool->appendWithCopy(genePool , fitnessError , dataError );

				masterPool->removeEqual(fitnessError, dataError);
				if( !masterPool->mStorage.empty() )
					manager->topFitness = (*masterPool)[0]->fitness;

				lastUpdateTime = SystemPlatform::GetTickCount();
			}
		
			LogMsg("Num %d genotype add to master Pool : index = %d ,genteration = %d , topFitness = %f", 
				  numAdded , index , trainData.generation , genePool[0]->fitness);

		}

		if( bRestData )
		{
			trainData.randomizeData();
			trainData.generation = 0;
		}
		restartGame();
	}

	CollisionResponse TrainWork::notifyCollision(BirdEntity& bird, ColObject& obj)
	{
		switch( obj.type )
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

	void TrainWork::executeWork()
	{
		setupWorkEnv();
		while( !bNeedStop )
		{
			trainData.tick();
			level->tick();
		}
	}

	void TrainWork::setupWorkEnv()
	{
		if( level == nullptr )
		{
			level = new GameLevel;
			bManageLevel = true;
		}
		level->onGameOver = LevelOverDelegate(this, &TrainWork::notifyGameOver);
		level->onBirdCollsion = CollisionDelegate(this, &TrainWork::notifyCollision);
		trainData.addAgentToLevel(*level);

		restartGame();
	}

	void TrainWork::stopRun()
	{
		SystemPlatform::InterlockedExchange(&bNeedStop, 1);
	}

	TrainManager::~TrainManager()
	{
		stopAllWork();
	}

	void TrainManager::init(TrainWorkSetting const& inSetting)
	{
		setting = inSetting;
		mGenePool.maxPoolNum = setting.masterPoolNum;
		mWorkRunPool.init(setting.numWorker);
	}

	void TrainManager::startTrain()
	{
		for( int i = 0; i < setting.numWorker; ++i )
		{
			auto work = std::make_unique< TrainWork >();
			work->index = i;
			work->manager = this;
			work->trainData.init(setting.dataSetting);
			work->maxGeneration = setting.maxGeneration;
			work->genePool.maxPoolNum = setting.workerPoolNum;
			mWorkRunPool.addWork(work.get());
			mWorks.push_back(std::move(work));
		}
	}

	void TrainManager::stopTrain()
	{
		stopAllWork();
		mWorks.clear();
	}

	bool TrainManager::loadData(char const* path, TrainData& data)
	{
		InputFileSerializer serializer;
		if( !serializer.open(path) )
		{
			return false;
		}

		InputFileSerializer::ReadOp op(serializer);

		{
			Mutex::Locker locker(mPoolMutex);

			int numPoolData;
			op & numPoolData;
			if( numPoolData )
			{
				for( int i = 0; i < numPoolData; ++i )
				{
					GenotypePtr gt = std::make_shared<Genotype>();
					op & gt->fitness;
					op & gt->data;
					mGenePool.add(gt);
				}
			}

			if( !mGenePool.getDataSet().empty() )
				topFitness = mGenePool[0]->fitness;
		}

		data.inputData(op);


		return true;
	}

	bool TrainManager::saveData(char const* path, TrainData& data )
	{
		OutputFileSerializer serializer;
		if( !serializer.open(path) )
		{
			return false;
		}
		InputFileSerializer::WriteOp op(serializer);

		{
			Mutex::Locker locker(mPoolMutex);
			int numPoolData = mGenePool.mStorage.size();
			op & numPoolData;
			if( numPoolData )
			{
				for( int i = 0; i < numPoolData; ++i )
				{
					Genotype* gt = mGenePool[i].get();
					op & gt->fitness;
					op & gt->data;
				}
			}
		}

		data.outputData(op);
		return true;
	}

	void TrainManager::clearData()
	{
		Mutex::Locker locker(mPoolMutex);
		mGenePool.clear();
	}

	void TrainManager::stopAllWork()
	{
		for( auto& work : mWorks )
		{
			work->stopRun();
		}
	}

	int NeuralNetworkRenderer::getValueColor(NNScale value)
	{
		if( value > 0.9 )
			return EColor::White;
		if( value > 0.75 )
			return EColor::Yellow;
		if( value > 0.5 )
			return EColor::Orange;
		if( value > 0.25 )
			return EColor::Blue;
		if( value > 0.1 )
			return EColor::Gray;
		return EColor::Black;
	}

	void NeuralNetworkRenderer::draw(IGraphics2D& g)
	{
		FCNNLayout& NNLayout = FNN.getLayout();
		for( int i = 0; i < NNLayout.getInputNum(); ++i )
		{
			Vector2 pos = getInputNodePos(i);
			RenderUtility::SetPen(g, EColor::Black);
			RenderUtility::SetBrush(g, getValueColor(inputsAndSignals[i]));
			drawNode(g, pos);
		}

		int idxSignal = NNLayout.getInputNum();
		int idxPrevLayerSignal = 0;
		for( int i = 0; i <= NNLayout.getHiddenLayerNum(); ++i )
		{
			NeuralFullConLayer& layer = NNLayout.getLayer(i);
			int prevNodeNum = NNLayout.getPrevLayerNodeNum(i);
			for( int idxNode = 0; idxNode < layer.numNode; ++idxNode )
			{
				Vector2 pos = getLayerNodePos(i, idxNode);
				RenderUtility::SetPen(g, EColor::Black);
				if( inputsAndSignals )
				{
					RenderUtility::SetBrush(g, getValueColor(inputsAndSignals[idxSignal]));
				}
				else
				{
					RenderUtility::SetBrush(g, EColor::Purple);
				}
				drawNode(g, pos);

				NNScale* weights = FNN.getWeights(i, idxNode);
				for( int n = 0; n < prevNodeNum; ++n )
				{
					Vector2 prevPos = (i == 0) ? getInputNodePos(n) : getLayerNodePos(i - 1, n);
					NNScale value = weights[n + 1];
					if( bShowSignalLink && inputsAndSignals )
					{
						value *= inputsAndSignals[idxPrevLayerSignal+n];
					}
					RenderUtility::SetPen(g, (value > 0) ? EColor::Green : EColor::Red);
					RenderUtility::SetBrush(g, (value > 0) ? EColor::Green : EColor::Red);
					drawLink(g, prevPos, pos, Math::Abs(value));
				}
				++idxSignal;
			}
			idxPrevLayerSignal += prevNodeNum;
		}
	}

	void NeuralNetworkRenderer::drawLink(IGraphics2D& g, Vector2 const& p1, Vector2 const& p2, float width)
	{
		Vector2 dir = p2 - p1;
		if( dir.normalize() < 1e-4 )
			return;

		float halfWidth = 0.25 * width;
		if( bShowSignalLink )
			halfWidth *= 4;

		halfWidth = std::min(10.0f, halfWidth);
		Vector2 normalOffset;
		normalOffset.x = dir.y;
		normalOffset.y = -dir.x;
		normalOffset *= halfWidth;
		Vector2 v[4] = { p1 + normalOffset , p1 - normalOffset , p2 - normalOffset , p2 + normalOffset };
		g.drawPolygon(v, 4);
	}

}//namespace FlappyBird
