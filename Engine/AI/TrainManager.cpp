#include "TrainManager.h"

#include "Serialize/FileStream.h"
#include "SystemPlatform.h"
#include "LogSystem.h"

#include <algorithm>


namespace AI
{
	TrainAgent::~TrainAgent()
	{
		if (entity)
		{
			entity->release();
		}
	}

	void TrainAgent::init(FCNNLayout const& layout)
	{
		FNN.init(layout);
		GenotypePtr pGT = std::make_shared<Genotype>();
		pGT->data.resize(layout.getParameterNum());
		setGenotype(pGT);
	}

	void TrainAgent::setGenotype(GenotypePtr inGenotype)
	{
		assert(FNN.getLayout().getParameterNum() == inGenotype->data.size());
		genotype = inGenotype;
		FNN.setParamsters(inGenotype->data);
	}


	void TrainAgent::randomizeData(NNRand& random)
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
			auto pAgent = std::make_unique< TrainAgent >();
			pAgent->init(netLayout);
			pAgent->index = i;
			pAgent->randomizeData(GA.mRand);
			mAgents.push_back(std::move(pAgent));
		}
		FCNeuralNetwork& FNN = mAgents[0]->FNN;
		mSignals.resize(netLayout.getSignalNum());
	}

	void TrainData::findBestAgnet()
	{
		for( auto& agentPtr : mAgents )
		{
			if( bestAgent == nullptr || bestAgent->genotype->fitness < agentPtr->genotype->fitness)
			{
				if (bestAgent)
				{
					bestAgent->signals = nullptr;
				}
				bestAgent = agentPtr.get();
				bestAgent->signals = getBestSignals();
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
					auto pAgent = std::make_unique< TrainAgent >();
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

		bestAgent = nullptr;
	}

	void TrainData::runEvolution(GenePool* genePool)
	{
		++generation;
		int numGen = mAgents.size();
		TArray< GenotypePtr > tempSelections;

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

		TArray< GenotypePtr > selections;
		GA.select(setting->numTrainDataSelect, tempSelections, selections);
		
		TArray< GenotypePtr > offsprings;
		GA.crossover(mAgents.size(), selections, offsprings);
		GA.mutate(offsprings, setting->mutationGeneProb ,setting->mutationValueProb, setting->mutationValueDelta);

		if ( genePool )
		{
			TArray< GenotypePtr > temps;
			for( int i = 0; i < mAgents.size(); ++i )
			{
				temps.push_back(mAgents[i]->genotype);
				mAgents[i]->setGenotype(offsprings[i]);
			}

			float const fitnessError = 1e-3;
			NNScalar const dataError = 1e-6;
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

				auto pAgent = std::make_unique< TrainAgent >();
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
		if( bestAgent )
			bestAgent->signals = nullptr;

		bestAgent = nullptr;
		mAgents.clear();
	}

	float TrainData::getMaxFitness() const
	{
		float result = 0;
		for (auto const& agentPtr : mAgents)
		{
			float fitness = agentPtr->genotype->fitness;
			if (result < fitness)
			{
				result = fitness;
			}
		}
		return result;
	}

	void TrainWork::handleTrainCompleted()
	{
		float maxFitness = trainData.getMaxFitness();

		trainData.runEvolution(&genePool);

		bool const bRestData = (maxGeneration && (trainData.generation > maxGeneration));
		bool const bClearPool = (trainData.generation > 500 && genePool[0]->fitness < 50);
		if ( manager )
		{
			bool bSendDataToMaster = bRestData || (SystemPlatform::GetTickCount() - lastUpdateTime > 10000);
			if (bSendDataToMaster)
			{
				sendDataToMaster();
				LogMsg("%d TrainCompleted ,genteration = %d , MaxFitness = %f",
					index, trainData.generation, maxFitness);
			}
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
			NNScalar const dataError = 1e-6;

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

	void TrainManager::init(TrainWorkSetting const& inSetting, uint32 topology[], uint32 numTopology)
	{
		setting = inSetting;
		setting.dataSetting.netLayout = &mNNLayout;
		mGenePool.maxPoolNum = setting.masterPoolNum;
		mNNLayout.init(topology, numTopology);
		mWorkRunPool.init(setting.numWorker);

		for (int i = 0; i < setting.numWorker; ++i)
		{
			auto work = std::make_unique< TrainWork >();
			work->index = i;
			work->manager = this;
			work->maxGeneration = setting.maxGeneration;
			work->genePool.maxPoolNum = setting.workerPoolNum;
			work->world = setting.worldFactory();
			work->trainData.init(setting.dataSetting);
			mWorks.push_back(std::move(work));
		}
	}

	void TrainManager::startTrain()
	{
		for(auto const& work : mWorks)
		{
			mWorkRunPool.addWork(work.get());
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

		TArray<uint32> topology;
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

		TArray<uint32> topology;
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

	void TrainManager::OutputData(FCNNLayout const& layout, GenePool const& pool, IStreamSerializer& serializer)
	{
		TArray<uint32> topology;
		layout.getTopology(topology);
		serializer << topology;

		uint32 numPoolData = pool.mStorage.size();
		serializer << numPoolData;
		if (numPoolData)
		{
			uint32 dataSize = pool[0].get()->data.size();
			serializer << dataSize;
			for (int i = 0; i < numPoolData; ++i)
			{
				Genotype* gt = pool[i].get();
				serializer << gt->fitness;
				serializer << IStreamSerializer::MakeSequence(gt->data.data(), dataSize);
			}
		}
	}

	void TrainManager::IntputData(FCNNLayout& layout, GenePool& pool, IStreamSerializer& serializer)
	{
		TArray<uint32> topology;
		serializer >> topology;
		layout.init(&topology[0], topology.size());

		int numPoolData;
		serializer >> numPoolData;
		if (numPoolData)
		{
			uint32 dataSize = 0;
			serializer >> dataSize;

			if (dataSize)
			{
				for (int i = 0; i < numPoolData; ++i)
				{
					GenotypePtr gt = std::make_shared<Genotype>();
					serializer >> gt->fitness;
					gt->data.resize(dataSize);
					auto sequence = IStreamSerializer::MakeSequence(gt->data.data(), dataSize);
					serializer >> sequence;
					pool.add(gt);
				}
			}
		}
	}

}//namespace FlappyBird
