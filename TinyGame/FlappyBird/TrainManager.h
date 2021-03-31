#pragma once
#ifndef TrainManager_H_1AA8F01D_3E72_4D7F_8447_38AF459C75A2
#define TrainManager_H_1AA8F01D_3E72_4D7F_8447_38AF459C75A2

#include "RenderUtility.h"
#include "Serialize/DataStream.h"

#include "AI/NeuralNetwork.h"
#include "AI/GeneticAlgorithm.h"
#include "AsyncWork.h"

namespace FlappyBird
{
	class Agent;
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


	class Agent
	{
	public:

		~Agent();
		FCNeuralNetwork FNN;
		GenotypePtr     genotype;
		NNScale*        inputsAndSignals = nullptr;
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
		Agent* bestAgent = nullptr;

		std::vector< NNScale > bestInputsAndSignals;
		std::vector< std::unique_ptr< Agent > > mAgents;
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
		void init(TrainWorkSetting const& inSetting , int topology[] , int numTopology);

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

		TrainWorkSetting setting;

		float topFitness = 0;
	private:
		
		std::vector< std::unique_ptr<TrainWork> > mWorks;
		FCNNLayout mNNLayout;
		Mutex    mPoolMutex;
		GenePool mGenePool;
		QueueThreadPool  mWorkRunPool;
	};


	class NeuralNetworkRenderer
	{
	public:
		NeuralNetworkRenderer(FCNeuralNetwork& inFNN)
			:FNN(inFNN)
		{


		}

		

		void draw(IGraphics2D& g);

		void drawNode(IGraphics2D& g, Vector2 pos)
		{
			g.drawCircle(pos, 10);
		}

		void drawLink(IGraphics2D& g, Vector2 const& p1, Vector2 const& p2, float width);
		int  getValueColor(NNScale value);

		float getOffsetY(int idx, int numNode)
		{
			return (float(idx) - 0.5 * float(numNode - 1)) * nodeOffset;
		}
		Vector2 getInputNodePos(int idx)
		{
			int numInput = FNN.getLayout().getInputNum();
			float offsetY = getOffsetY(idx, numInput);
			return basePos + Vector2(0, offsetY);
		}
		Vector2 getLayerNodePos(int idxLayer, int idxNode)
		{
			NeuralFullConLayer const& layer = FNN.getLayout().getLayer(idxLayer);
			float offsetY = getOffsetY(idxNode, layer.numNode);
			return basePos + Vector2((idxLayer + 1) * layerOffset, offsetY);
		}

		NNScale* inputsAndSignals = nullptr;
		bool bShowSignalLink = true;
		float scaleFactor = 1.5;
		float layerOffset = scaleFactor * 40;
		float nodeOffset = scaleFactor * 30;
		Vector2 basePos = Vector2(0, 0);
		FCNeuralNetwork& FNN;
	};

} //namespace FlappyBird

#endif // TrainManager_H_1AA8F01D_3E72_4D7F_8447_38AF459C75A2