#pragma once
#ifndef TrainManager_H_1AA8F01D_3E72_4D7F_8447_38AF459C75A2
#define TrainManager_H_1AA8F01D_3E72_4D7F_8447_38AF459C75A2

#include "AI/NeuralNetwork.h"
#include "AI/GeneticAlgorithm.h"

#include "Serialize/DataStream.h"
#include "Async/AsyncWork.h"

#include <functional>

namespace AI
{
	class TrainAgent;
	class TrainData;


	class AgentEntity
	{
	public:
		virtual ~AgentEntity() {}
		virtual void release() = 0;
	};

	using TrainCompletedDelegate = std::function< void() >;
	class AgentWorld
	{
	public:
		virtual ~AgentWorld() {}
		virtual void release() = 0;

		virtual void setup(TrainData& trainData) = 0;
		virtual void restart(TrainData& trainData) = 0;
		virtual void tick(TrainData& trainData) = 0;

		TrainCompletedDelegate onTrainCompleted;
	};


	class TrainAgent
	{
	public:

		~TrainAgent();

		int index;
		FCNeuralNetwork FNN;
		GenotypePtr     genotype;
		NNScalar*       signals = nullptr;
		AgentEntity*    entity = nullptr;

		void init(FCNNLayout const& layout);
		void setGenotype(GenotypePtr  inGenotype);
		void randomizeData(NNRand& random);
	};

	struct TrainDataSetting
	{
		FCNNLayout* netLayout;

		int    numAgents;
		uint64 initWeightSeed;

		int numPoolDataSelectUsed;
		int numTrainDataSelect;

		float mutationGeneProb;
		float mutationValueProb;
		float mutationValueDelta;

		TrainDataSetting()
		{
			netLayout = nullptr;

			numAgents = 200;
			initWeightSeed = 0;
			numPoolDataSelectUsed = 5;
			numTrainDataSelect = 10;

			mutationGeneProb = 0.8;
			mutationValueProb = 0.5;
			mutationValueDelta = 1e-2;
		}
	};


	class TrainData
	{
	public:
		void init( TrainDataSetting const& inSetting );
		
		void findBestAgnet();
		void usePoolData(GenePool& pool);
		void runEvolution(GenePool* genePool);

		void inputData(IStreamSerializer::ReadOp& op );
		void outputData(IStreamSerializer::WriteOp& op );

		void randomizeData();
		void clearData();

		float getMaxFitness() const;

		TrainDataSetting const* setting;
		int    generation;
		TrainAgent* bestAgent = nullptr;

		TArray< NNScalar > mSignals;
		//uint8* bestInputsAndSignals;

		NNScalar* getBestSignals()
		{
			return mSignals.data();
			//return (NNScalar*)((intptr_t(bestInputsAndSignals) + 15) & ~15);
		}
		TArray< std::unique_ptr< TrainAgent > > mAgents;
		GeneticAlgorithm GA;

	};

	class TrainManager;
	class TrainWork : public IQueuedWork
	{
	public:

		TrainWork() = default;

		~TrainWork()
		{
			if (world)
			{
				world->release();
			}
		}
		void restartGame()
		{
			world->restart(trainData);
		}

		void stopRun();
		void sendDataToMaster();

		void handleTrainCompleted();

		//IQueuedWork
		void executeWork() override;
		void abandon() override
		{

		}
		void release() override
		{

		}
		//~IQueuedWork
		int       index = 0;
		int64     lastUpdateTime = 0;
		volatile int32 bNeedStop = 0;
		TrainData  trainData;
		GenePool   genePool;
		TrainManager* manager = nullptr;
		int maxGeneration = 0;
		AgentWorld* world = nullptr;
	};

	using AgentWorldCreationDeletgate = std::function < AgentWorld* () >;
	struct TrainWorkSetting
	{
		int numWorker;
		int maxGeneration;
		int masterPoolNum;
		int workerPoolNum;

		AgentWorldCreationDeletgate worldFactory;
		TrainDataSetting dataSetting;

		TrainWorkSetting()
		{
			numWorker = 8;
			maxGeneration = 120;
			masterPoolNum = 40;
			workerPoolNum = 30;
		}
	};

	class TrainManager
	{
	public:
		~TrainManager();
		void init(TrainWorkSetting const& inSetting , uint32 topology[] , uint32 numTopology);

		void startTrain();
		void stopTrain();

		bool loadData(char const* path, TrainData& data );
		bool saveData(char const* path, TrainData& data );

		void clearData();

		auto lockPool() 
		{ 
			return MakeLockedObjectHandle(mGenePool, &mPoolMutex); 
		}

		TrainDataSetting& getDataSetting() { return setting.dataSetting; }

		void stopAllWork();
		FCNNLayout& getNetLayout() { return mNNLayout; }


		static void OutputData(FCNNLayout const& layout, GenePool const& pool, IStreamSerializer& serializer);
		static void IntputData(FCNNLayout& layout, GenePool& pool, IStreamSerializer& serializer);


		TrainWorkSetting setting;

		float topFitness = 0;
	private:
		
		TArray< std::unique_ptr<TrainWork> > mWorks;
		FCNNLayout mNNLayout;
		Mutex    mPoolMutex;
		GenePool mGenePool;
		QueueThreadPool  mWorkRunPool;
	};

} //namespace AI

#endif // TrainManager_H_1AA8F01D_3E72_4D7F_8447_38AF459C75A2