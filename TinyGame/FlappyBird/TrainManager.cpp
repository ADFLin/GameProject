#include "TrainManager.h"

#include "Serialize/FileStream.h"
#include "SystemPlatform.h"

#include <algorithm>


namespace FlappyBird
{

	Agent::~Agent()
	{
		if (entity)
		{
			entity->release();
		}
	}

	void Agent::init(FCNNLayout const& layout)
	{
		FNN.init(layout);
		GenotypePtr pGT = std::make_shared<Genotype>();
		pGT->data.resize(layout.getWeightNum());
		setGenotype(pGT);
	}

	void Agent::setGenotype(GenotypePtr inGenotype)
	{
		assert(FNN.getLayout().getWeightNum() == inGenotype->data.size());
		genotype = inGenotype;
		FNN.setWeights(inGenotype->data);
	}


	void Agent::randomizeData(NNRand& random)
	{
		genotype->randomizeData(random, -1, 1);
	}

	void TrainData::init(TrainDataSetting const& inSetting )
	{
		setting = &inSetting;

		FCNNLayout& netLayout = *inSetting.netLayout;
		assert(setting->numAgents > 0);
		generation = 0;
		for( int i = 0; i < setting->numAgents; ++i )
		{
			auto pAgent = std::make_unique< Agent >();
			pAgent->init(netLayout);
			pAgent->randomizeData(GA.mRand);
			mAgents.push_back(std::move(pAgent));
		}
		FCNeuralNetwork& FNN = mAgents[0]->FNN;
		bestInputsAndSignals.resize(netLayout.getInputNum() + netLayout.getHiddenNodeNum() + netLayout.getOutputNum());
	}

	void TrainData::findBestAgnet()
	{
		for( auto& agentPtr : mAgents )
		{
			if( curBestAgent == nullptr || curBestAgent->genotype->fitness < agentPtr->genotype->fitness)
			{
				if (curBestAgent)
				{
					curBestAgent->inputsAndSignals = nullptr;
				}
				curBestAgent = agentPtr.get();
				curBestAgent->inputsAndSignals = &bestInputsAndSignals[0];
			}
		}
	}


	void TrainData::usePoolData(GenePool& pool)
	{
		FCNNLayout const& netLayout = *setting->netLayout;

		assert( setting->numAgents == pool.getDataSet().size() );
		if( mAgents.size() != setting->numAgents )
		{
			if( mAgents.size() > setting->numAgents )
			{
				mAgents.resize(pool.getDataSet().size());
			}
			else
			{
				int numNewAgents = setting->numAgents - mAgents.size();
				for( int i = 0; i < numNewAgents; ++i )
				{
					auto pAgent = std::make_unique< Agent >();
					pAgent->init(netLayout);
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
			int poolSelectionNum = std::min(setting->numPoolDataSelectUsed, (int)genePool->mStorage.size());
			for( int i = 0; i < poolSelectionNum; ++i )
			{
				tempSelections.push_back((*genePool)[i]);
			}
		}

		for(auto& agentPtr : mAgents)
		{
			tempSelections.push_back(agentPtr->genotype);
		}

		std::sort(tempSelections.begin(), tempSelections.end(),
			[](GenotypePtr const& p1, GenotypePtr const& p2) -> bool
			{
				return p1->fitness > p2->fitness;
			}
		);

		tempSelections.resize( setting->numTrainDataSelect );

		std::vector< GenotypePtr > selections;
		GA.select(setting->numTrainDataSelect, tempSelections, selections);
		
		std::vector< GenotypePtr > offsprings;
		GA.crossover(mAgents.size(), selections, offsprings);
		GA.mutate(offsprings, setting->mutationGeneProb ,setting->mutationValueProb, setting->mutationValueDelta);

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
			GenePool::RemoveIdenticalGenotype(temps, fitnessError, dataError);
			for(auto& gene : temps)
			{
				genePool->add(gene);
			}
			genePool->removeIdentical(fitnessError, dataError);
		}
		else
		{
			for( int i = 0; i < mAgents.size(); ++i )
			{
				mAgents[i]->setGenotype(offsprings[i]);
			}
		}
	}

	void TrainData::inputData(IStreamSerializer::ReadOp& op)
	{
		FCNNLayout const& netLayout = *setting->netLayout;
		int numAgentData;
		op & numAgentData;

		mAgents.clear();

		if( numAgentData )
		{
			int dataSize;
			op & dataSize;

			for( int i = 0; i < numAgentData; ++i )
			{
				GenotypePtr gt = std::make_shared<Genotype>();
				op & gt->fitness;
				op & gt->data;

				auto pAgent = std::make_unique< Agent >();
				pAgent->FNN.init(netLayout);
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

			for(auto& agentPtr : mAgents)
			{
				Genotype* gt = agentPtr->genotype.get();
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

	void TrainWork::handleTrainCompleted()
	{
		trainData.runEvolution(&genePool);

		bool const bRestData = ( maxGeneration && ( trainData.generation > maxGeneration ) );
		bool const bClearPool = (trainData.generation > 500 && genePool[0]->fitness < 50);
		bool bSendDataToMaster = bRestData || (SystemPlatform::GetTickCount() - lastUpdateTime > 10000);
		if( bSendDataToMaster )
		{
			sendDataToMaster();
		}

		if( bRestData || bClearPool )
		{
			trainData.randomizeData();
			trainData.generation = 0;
		}
		if( bClearPool )
		{
			genePool.clear();
		}

		world->restart(trainData);
	}

	void TrainWork::executeWork()
	{
		world->onTrainCompleted = std::bind( &TrainWork::handleTrainCompleted , this );
		world->setup(trainData);
		world->restart(trainData);

		while( !bNeedStop )
		{
			//trainData.findBestAgnet();
			world->tick(trainData);
		}
	}

	void TrainWork::stopRun()
	{
		SystemPlatform::AtomExchange(&bNeedStop, 1);
	}

	void TrainWork::sendDataToMaster()
	{
		int numAdded;
		{
			float const fitnessError = 1e-3;
			NNScale const dataError = 1e-6;

			TLockedObject< GenePool > masterPool = manager->lockPool();
			numAdded = masterPool->appendWithCopy(genePool, fitnessError, dataError);

			masterPool->removeIdentical(fitnessError, dataError);
			if (!masterPool->mStorage.empty())
				manager->topFitness = (*masterPool)[0]->fitness;

			lastUpdateTime = SystemPlatform::GetTickCount();
		}

		LogMsg("Num %d genotype add to master Pool : index = %d ,genteration = %d , topFitness = %f",
			numAdded, index, trainData.generation, genePool[0]->fitness);
	}

	TrainManager::~TrainManager()
	{
		stopAllWork();
	}

	void TrainManager::init(TrainWorkSetting const& inSetting, int topology[], int numTopology)
	{
		setting = inSetting;
		setting.dataSetting.netLayout = &mNNLayout;
		mGenePool.maxPoolNum = setting.masterPoolNum;
		mNNLayout.init(topology, numTopology);
		mWorkRunPool.init(setting.numWorker);
	}

	void TrainManager::startTrain()
	{
		for( int i = 0; i < setting.numWorker; ++i )
		{
			auto work = std::make_unique< TrainWork >();
			work->index = i;
			work->manager = this;
			work->maxGeneration = setting.maxGeneration;
			work->genePool.maxPoolNum = setting.workerPoolNum;
			work->world = setting.worldFactory();
			work->trainData.init(setting.dataSetting);

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

		IStreamSerializer::ReadOp op(serializer);
		data.setting = &getDataSetting();

		std::vector<int> topology;
		op & topology;
		mNNLayout.init(&topology[0], topology.size());

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

		if( !serializer.isValid() )
		{
			return false;
		}

		return true;
	}

	bool TrainManager::saveData(char const* path, TrainData& data )
	{
		OutputFileSerializer serializer;
		if( !serializer.open(path) )
		{
			return false;
		}

		IStreamSerializer::WriteOp op(serializer);


		std::vector<int> topology;
		mNNLayout.getTopology(topology);
		op & topology;

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

		if( !serializer.isValid() )
		{
			return false;
		}
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
		FCNNLayout const& NNLayout = FNN.getLayout();
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
			NeuralFullConLayer const& layer = NNLayout.getLayer(i);
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
