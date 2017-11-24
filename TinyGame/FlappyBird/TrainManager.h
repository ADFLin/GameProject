#pragma once
#ifndef TrainManager_H_1AA8F01D_3E72_4D7F_8447_38AF459C75A2
#define TrainManager_H_1AA8F01D_3E72_4D7F_8447_38AF459C75A2

#include "FBLevel.h"

#include "RenderUtility.h"
#include "DataStream.h"

#include "AI/NeuralNetwork.h"
#include "AI/GeneticAlgorithm.h"
#include "AsyncWork.h"

namespace FlappyBird
{


	class Agent : public IController
	{
	public:

		bool  bCompleted = false;
		float lifeTime = 0;

		BirdEntity    hostBird;
		GenotypePtr   genotype;
		FCNeuralNetwork FNN;
		NNScale*      inputsAndSignals = nullptr;

		void init(FCNNLayout& layout);
		void setGenotype(GenotypePtr  inGenotype);
		void restart();
		void tick();
		void randomizeData(NNRand& random);
		virtual void updateInput(GameLevel& world, BirdEntity& bird) override;

		void getPipeInputs(NNScale inputs[], PipeInfo const& pipe );

	};

	struct TrainDataSetting
	{
		int numAgents;

		int numPoolDataSelectUsed;
		int numTrainDataSelect;

		float mutationGeneProb;
		float mutationValueProb;
		float mutationValueDelta;
		TrainDataSetting()
		{
			numAgents = 200;
			numPoolDataSelectUsed = 5;
			numTrainDataSelect = 10;

			mutationGeneProb = 0.8;
			mutationValueProb = 0.5;
			mutationValueDelta = 2.0;
		}
	};


	class TrainData
	{
	public:
		void init(TrainDataSetting const& inSetting);
		
		void tick();
		void restart();
		void usePoolData(GenePool& pool);
		void runEvolution(GenePool* genePool);

		void addAgentToLevel(GameLevel& level);

		void inputData(DataSerializer::ReadOp& op );
		void outputData(DataSerializer::WriteOp& op );

		void randomizeData();
		void clearData();


		FCNNLayout mNNLayout;

		TrainDataSetting setting;
		int    generation;
		Agent* curBestAgent = nullptr;
		std::vector< NNScale > bestInputsAndSignals;
		std::vector< std::unique_ptr< Agent > > mAgents;
		GeneticAlgorithm GA;

	};

	class TrainManager;
	class TrainWork : public IQueuedWork
	{
	public:

		TrainWork()
		{

		}

		~TrainWork()
		{
			if( bManageLevel )
			{
				delete level;
			}
		}
		void restartGame()
		{
			level->restart();
			trainData.restart();
		}
		void setupWorkEnv();
		void stopRun();

		void notifyGameOver(GameLevel& level);
		CollisionResponse notifyCollision(BirdEntity& bird, ColObject& obj);
		//IQueuedWork
		virtual void executeWork();
		virtual void abandon()
		{

		}
		virtual void release()
		{

		}
		//~IQueuedWork
		int  index = 0;
		uint32 lastUpdateTime = 0;
		volatile int32 bNeedStop = 0;
		TrainData  trainData;
		GenePool genePool;
		GameLevel* level = nullptr;
		bool bManageLevel = false;
		TrainManager* Manager = nullptr;
		int maxGeneration = 0;
	};

	struct TrainWorkSetting
	{
		int numWorker;
		int maxGeneration;
		int masterPoolNum;
		int workerPoolNum;

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
		void init(TrainWorkSetting const& inSetting);

		void startTrain();
		void stopTrain();

		bool loadData(char const* path, TrainData& data );
		bool saveData(char const* path, TrainData& data );

		void clearData();

		auto lockPool() 
		{ 
			return MakeLockedObjectHandle(mGenePool, &mPoolMutex); 
		}

		void stopAllWork();


		TrainWorkSetting setting;
		float topFitness = 0;
	private:
		
		std::vector< std::unique_ptr<TrainWork> > mWorks;
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