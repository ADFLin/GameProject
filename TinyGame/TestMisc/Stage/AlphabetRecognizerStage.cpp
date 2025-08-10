
#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "AI/NeuralNetwork.h"

#include "RHI/RHICommand.h"
#include "GameGlobal.h"
#include "RHI/RHIGraphics2D.h"
#include "ProfileSystem.h"
#include "Async/AsyncWork.h"

namespace AR
{

	NNScalar Parameters0[] =
	{
		-0.1486,  0.1954,  0.427,  -0.0972,  0.6795,  0.2573,  0.363,   0.481,   0.6536,  0.0017,
		-0.4872, -0.1346,  0.2494,  0.3854,  0.023,  -0.0038,  0.3377, -.2748,   0.076,   0.5066,
		-0.0647, -0.4726, -0.2256,  0.4003, -0.2734,  0.099,   0.4775,  0.4412,  0.3696,  0.1814,
		-0.05,    0.0491, -0.8318, -1.1349, -0.4355,  0.0529, -0.7672, -0.2598,  0.0783,  0.6147,
		-0.5161,  0.5607,  0.2497,  0.2084,  0.5713,  0.5264, -0.2486,  0.44,    0.3286,  0.0896,
		-0.4547, -0.4105, -0.343,  -0.0175, -0.2769, -0.2417,  0.5793,  0.5533,  0.4434,  0.2215,
		 0.5154,  0.3763,  0.339,  -0.0752, -0.0239, -0.1771, -0.2101, -0.197,  -0.3623,  0.2826,
		 0.4506,  0.4106, -0.3482,  0.2583, -0.1089,  0.2302,  0.0907,  0.0353, -0.1514,  0.3524,
	};

	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		typedef StageBase BaseClass;
	public:
		TestStage() {}


		TArray< NNScalar > mParamters;
		void addCNNParam(NeuralConv2DLayer& inoutlayer, int numSliceInput, TArrayView<NNScalar> parameters)
		{
			int const convLen = inoutlayer.convSize * inoutlayer.convSize;
			int const nodeWeightLen = numSliceInput * convLen;
			int numParameters = (nodeWeightLen + 1) * inoutlayer.numNode;

			inoutlayer.weightOffset = mParamters.size();
			inoutlayer.biasOffset = mParamters.size() + nodeWeightLen * inoutlayer.numNode;

			mParamters.resize(mParamters.size() + numParameters);
			
			NNScalar* pWeight = mParamters.data() + layer.weightOffset;
			NNScalar* pBias = mParamters.data() + layer.biasOffset;
			NNScalar const* pCopyParams = parameters.data();
			for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
			{
				FMemory::Copy(pWeight, pCopyParams, nodeWeightLen * sizeof(NNScalar));
				pWeight += nodeWeightLen;
				pCopyParams += nodeWeightLen;

				*pBias = *pCopyParams;
				pBias += 1;
				pCopyParams += 1;
			}
		}

		NeuralConv2DLayer layer;
		virtual bool onInit()
		{
			if (!BaseClass::onInit())
				return false;

			layer.numNode = 8;
			layer.convSize = 3;
			layer.dataSize[0] = 26;
			layer.dataSize[1] = 26;
			layer.setFuncionT< NNFunc::ReLU >();
			addCNNParam(layer, 1, MakeView(Parameters0));

			//FNNAlgo::ForwardFeedback(layer, )
		}

		virtual void onEnd()
		{
			BaseClass::onEnd();
		}

		void restart()
		{

		}
	};

}