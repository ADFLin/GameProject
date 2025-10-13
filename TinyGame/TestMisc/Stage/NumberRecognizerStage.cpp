
#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "AI/NeuralNetwork.h"

#include "RHI/RHICommand.h"
#include "GameGlobal.h"
#include "RHI/RHIGraphics2D.h"
#include "ProfileSystem.h"
#include "Async/AsyncWork.h"
#include "RenderDebug.h"
#include "FileSystem.h"
#include "Serialize/FileStream.h"
#include "AI/NNTrain.h"
#include "RandomUtility.h"

#if 0
#include "ANNT/ANNT.hpp"
#endif


#include <random>
#include "Async/Coroutines.h"


namespace NR
{
	char const* SampleFileFolder = "mnist";
	char const* TrainImageFileName = "train-images.idx3-ubyte";
	char const* TrainLabelFileName = "train-labels.idx1-ubyte";
	char const* TestImageFileName = "t10k-images.idx3-ubyte";
	char const* TestLabelFileName = "t10k-labels.idx1-ubyte";


	using namespace Render;

	void TestANNT()
	{
#if 0
		using namespace ANNT;
		using namespace ANNT::Neuro;



		fvector_t input =
		{
			1, 2, 3, 4, 5, 6,
			 7, 8, 9,10,11,12,
			13,14,15,16,17,18,
			19,20,21,22,23,24,
			25,26,27,28,29,30,
			31,32,33,34,35,36,
			1, 2, 3, 4, 5, 6,
			 7, 8, 9,10,11,12,
			13,14,15,16,17,18,
			19,20,21,22,23,24,
			25,26,27,28,29,30,
			31,32,33,34,35,36,
		};

		fvector_t w =
		{
			1, 2, 3, 
			4, 5, 6,
			7, 8, 9,
			11, 12, 13,
			14, 15, 16,
			17, 18, 19,
			0,
		};

		XConvolutionLayer layer{ 6, 6, 2 , 3 , 3 , 1 };
		fvector_t output;
		output.resize(4 * 4 * 1);
		XNetworkContext ctx{ true };
		std::vector<fvector_t*> outputs = { &output };




		layer.SetWeights(w);
		fvector_t prevDelta;
		prevDelta.resize(6 * 6 * 2);
		fvector_t gradWeights;
		gradWeights.resize(w.size());
		fvector_t delta =
		{
			0.1, 0.2, 0.3, 0.4,
			0.5, 0.6, 0.7, 0.8,
			0.9, 1.0, 1.1, 1.2,
			1.3, 1.4, 1.5, 1.6,
		};
		std::vector<fvector_t*> prevDeltas = { &prevDelta };

		layer.ForwardCompute({ &input }, outputs, ctx);
		layer.BackwardCompute({ &input }, { &output }, { &delta }, prevDeltas, gradWeights, ctx);


		NNConv2DLayer layer2;
		layer2.init(IntVector3(6, 6, 2), 3, 1);
		layer2.weightOffset = 0;
		layer2.biasOffset = 18;

		int inputSize[] = { 6, 6 };
 		NNScalar output2[4 * 4];
		NNScalar gradWeights2[19] = { 0 };
		NNScalar prevDelta2[6 * 6 * 2];


		FNNAlgo::Forward(layer2, w.data(), input.data(), output2);
		FNNAlgo::BackwardWeight(layer2, input.data(), output2, delta.data(), gradWeights2);
		FNNAlgo::BackwardLoss(layer2, w.data(), delta.data(), prevDelta2);


		XAveragePooling layerB{ 6, 6, 2, 2 };
		fvector_t deltaB =
		{
			0.1, 0.2, 0.3,
			0.4, 0.5, 0.6,
			0.7, 0.8, 0.9,
			1.1, 1.2, 1.3,
			1.4, 1.5, 1.6,
			1.7, 1.8, 1.9,
		};
		fvector_t outputB;
		outputB.resize(3 * 3 * 2);
		std::vector<fvector_t*> outputsB = { &outputB };
		fvector_t prevDeltaB;
		prevDeltaB.resize(6 * 6 * 2);
		std::vector<fvector_t*> prevDeltasB = { &prevDeltaB };
		layerB.ForwardCompute({ &input }, outputsB, ctx);
		layerB.BackwardProcess({ &input }, outputsB, { &deltaB }, prevDeltasB, ctx);


		NNAveragePooling2DLayer layerB2;
		layerB2.init(IntVector3(6, 6, 2), 2);

		NNScalar outputB2[3 * 3 * 2];
		NNScalar gradWeightsB2[19] = { 0 };
		NNScalar prevDeltaB2[6 * 6 * 2];

		FNNAlgo::Forward(layerB2, input.data(), outputB2);
		FNNAlgo::Backward(layerB2, deltaB.data(), prevDeltaB2);

		int i = 1;
#endif

	}
	void Test()
	{
		NNMaxPooling2DLayer layer;
		layer.init(IntVector3(6, 6, 2), 2);

		NNScalar m[] =
		{
			 1, 2, 3, 4, 5, 6,
			 7, 8, 9,10,11,12,
			13,14,15,16,17,18,
			19,20,21,22,23,24,
			25,26,27,28,29,30,
			31,32,33,34,35,36,
			 1, 2, 3, 4, 5, 6,
			 7, 8, 9,10,11,12,
			13,14,15,16,17,18,
			19,20,21,22,23,24,
			25,26,27,28,29,30,
			31,32,33,34,35,36,
		};
		NNScalar output[3 * 3 * 2];
		int inputSize[] = { 6, 6 };
		FNNAlgo::Forward(layer, m, output);

		NNScalar nn[3 * 3 * 2] =
		{
			1,2,3,
			4,5,6,
			7,8,9,
			10,11,12,
			13,14,15,
			16,17,18,
		};
		NNScalar nno[6 * 6 * 2];
		FNNAlgo::Backward(layer, m, output, nn, nno);

		int i = 1;
	}


	struct NNSequenceLayer
	{
		void addLayer(NNConv2DLayer& inoutlayer)
		{
			int numParameters = inoutlayer.getParameterLength();

			inoutlayer.weightOffset = mParameterLength;
			inoutlayer.biasOffset = mParameterLength + inoutlayer.getWeightLength();
			inoutlayer.fastMethod = NNConv2DLayer::eNone;

			mOutputOffsets[mNumLayer] = mPassOutputLength;
			++mNumLayer;
			mPassOutputLength += inoutlayer.getOutputLength();
			mMaxOutputLength = Math::Max(mMaxOutputLength, inoutlayer.getOutputLength());

			mParameterLength += numParameters;
		}

		void addLayer(NNLinearLayer& inoutlayer)
		{
			int parameterLength = inoutlayer.getParameterLength();

			inoutlayer.weightOffset = mParameterLength;
			inoutlayer.biasOffset = mParameterLength + inoutlayer.getWeightLength();

			mOutputOffsets[mNumLayer] = mPassOutputLength;
			++mNumLayer;
			mPassOutputLength += inoutlayer.getOutputLength();
			mMaxOutputLength = Math::Max(mMaxOutputLength, inoutlayer.getOutputLength());

			mParameterLength += parameterLength;
		}

		void addLayer(NNMaxPooling2DLayer& inoutlayer)
		{
			mOutputOffsets[mNumLayer] = mPassOutputLength;
			++mNumLayer;
			mPassOutputLength += inoutlayer.getOutputLength();
			mMaxOutputLength = Math::Max(mMaxOutputLength, inoutlayer.getOutputLength());
		}

		void addLayer(NNAveragePooling2DLayer& inoutlayer)
		{
			mOutputOffsets[mNumLayer] = mPassOutputLength;
			++mNumLayer;
			mPassOutputLength += inoutlayer.getOutputLength();
			mMaxOutputLength = Math::Max(mMaxOutputLength, inoutlayer.getOutputLength());
		}

		void addLayer(NNTransformLayer const& layer, int inputLength)
		{
			mOutputOffsets[mNumLayer] = mPassOutputLength;
			++mNumLayer;
			mPassOutputLength += inputLength;
			mMaxOutputLength = Math::Max(mMaxOutputLength, inputLength);
		}

		void addSoftMaxLayer(int inputLength)
		{
			mOutputOffsets[mNumLayer] = mPassOutputLength;
			++mNumLayer;
			mPassOutputLength += inputLength;
			mMaxOutputLength = Math::Max(mMaxOutputLength, inputLength);
		}

		int mParameterLength = 0;
		int mPassOutputLength = 0;
		int mOutputOffsets[32];
		int mNumLayer = 0;
		int mMaxOutputLength = 0;
	};

	struct Model : public NNSequenceLayer
	{
		Model(int const imageSize[], int numCategory)
		{
			layerReLU.setFuncionT< NNFunc::ReLU >();

			layer1.init(IntVector3(imageSize[0], imageSize[1], 1), 3, 8);
			addLayer(layer1);
			addLayer(layerReLU, layer1.getOutputLength());
			layer2.init(IntVector3(layer1.dataSize[0], layer1.dataSize[1], layer1.numNode), 3, 8);
			addLayer(layer2);
			addLayer(layerReLU, layer2.getOutputLength());

			layer3.init(layer2.getOutputSize(), 2);
			addLayer(layer3);

			layer4.init(IntVector3(layer3.dataSize[0], layer3.dataSize[1], layer3.numNode), 3, 10);
			addLayer(layer4);
			addLayer(layerReLU, layer4.getOutputLength());

			layer5.init(IntVector3(layer4.dataSize[0], layer4.dataSize[1], layer4.numNode), 3, 10);
			addLayer(layer5);
			addLayer(layerReLU, layer5.getOutputLength());

			layer6.init(layer5.getOutputSize(), 2);
			addLayer(layer6);

			layer7.init(layer6.getOutputLength(), numCategory);
			addLayer(layer7);


			addSoftMaxLayer(layer7.numNode);

			CHECK(mNumLayer == 12);
		}

		NNConv2DLayer layer1;
		NNConv2DLayer layer2;
		NNMaxPooling2DLayer layer3;
		NNConv2DLayer layer4;
		NNConv2DLayer layer5;
		NNMaxPooling2DLayer layer6;
		NNLinearLayer layer7;

		NNTransformLayer layerReLU;


		int getParameterLength() const
		{
			return mParameterLength;
		}

		int getOutputLength() const
		{
			return layer7.getOutputLength();
		}
		int getPassOutputLength() const
		{
			return mPassOutputLength;
		}
		int getTempLossGradLength() const
		{
			return 2 * mMaxOutputLength;
		}

		NNScalar* forward(
			NNScalar const parameters[],
			NNScalar const inputs[],
			NNScalar outputs[])
		{
			NNScalar const* pInput = inputs;
			pInput = FNNAlgo::Forward(layer1, parameters, pInput, outputs + mOutputOffsets[0]);
			pInput = FNNAlgo::Forward(layerReLU, layer1.getOutputLength(), pInput, outputs + mOutputOffsets[1]);
			pInput = FNNAlgo::Forward(layer2, parameters, pInput, outputs + mOutputOffsets[2]);
			pInput = FNNAlgo::Forward(layerReLU, layer2.getOutputLength(), pInput, outputs + mOutputOffsets[3]);
			pInput = FNNAlgo::Forward(layer3, pInput, outputs + mOutputOffsets[4]);
			pInput = FNNAlgo::Forward(layer4, parameters, pInput, outputs + mOutputOffsets[5]);
			pInput = FNNAlgo::Forward(layerReLU, layer4.getOutputLength(), pInput, outputs + mOutputOffsets[6]);
			pInput = FNNAlgo::Forward(layer5, parameters, pInput, outputs + mOutputOffsets[7]);
			pInput = FNNAlgo::Forward(layerReLU, layer5.getOutputLength(), pInput, outputs + mOutputOffsets[8]);
			pInput = FNNAlgo::Forward(layer6, pInput, outputs + mOutputOffsets[9]);
			pInput = FNNAlgo::Forward(layer7, parameters, pInput, outputs + mOutputOffsets[10]);

			int index = FNNMath::SoftMax(layer7.getOutputLength(), pInput, outputs + mOutputOffsets[11]);
			return outputs + mOutputOffsets[11];
		}

		void backward(
			NNScalar const parameters[],
			NNScalar const inInputs[],
			NNScalar const inOutputs[],
			NNScalar const inOutputLossGrads[],
			TArrayView<NNScalar> tempLossGrads,
			NNScalar inoutParameterGrads[],
			NNScalar* outLossGrads = nullptr)
		{

			NNScalar* pLossGrad = tempLossGrads.data();
			NNScalar* pOutputLossGrad = tempLossGrads.data() + tempLossGrads.size() / 2;

			NNScalar const* pOutput = inOutputs + mOutputOffsets[11];
			NNScalar const* pInput = inOutputs + mOutputOffsets[10];

			//FNNAlgo::SoftMaxBackward(layer7.getOutputLength(), pInput, pOutput, inOutputLossGrads, pLossGrad);

			//std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[9];
			FNNAlgo::BackwardWeight(layer7, pInput, inOutputLossGrads, inoutParameterGrads);
			FNNAlgo::BackwardLoss(layer7, parameters, inOutputLossGrads, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[8];
			FNNAlgo::Backward(layer6, pInput, pOutput, pOutputLossGrad, pLossGrad);

			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[7];
			FNNAlgo::Backward(layerReLU, layer5.getOutputLength(), pInput, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[6];
			FNNAlgo::BackwardWeight(layer5, pInput, pOutput, pOutputLossGrad, inoutParameterGrads);
			FNNAlgo::BackwardLoss(layer5, parameters, pOutputLossGrad, pLossGrad);

			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[5];
			FNNAlgo::Backward(layerReLU, layer4.getOutputLength(), pInput, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[4];
			FNNAlgo::BackwardWeight(layer4, pInput, pOutput, pOutputLossGrad, inoutParameterGrads);
			FNNAlgo::BackwardLoss(layer4, parameters, pOutputLossGrad, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[3];
			FNNAlgo::Backward(layer3, pInput, pOutput, pOutputLossGrad, pLossGrad);

			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[2];
			FNNAlgo::Backward(layerReLU, layer2.getOutputLength(), pInput, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[1];
			FNNAlgo::BackwardWeight(layer2, pInput, pOutput, pOutputLossGrad, inoutParameterGrads);
			FNNAlgo::BackwardLoss(layer2, parameters, pOutputLossGrad, pLossGrad);

			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[0];
			FNNAlgo::Backward(layerReLU, layer1.getOutputLength(), pInput, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inInputs;
			FNNAlgo::BackwardWeight(layer1, pInput, pOutput, pOutputLossGrad, inoutParameterGrads);
			if (outLossGrads)
			{
				FNNAlgo::BackwardLoss(layer1, parameters, pOutputLossGrad, outLossGrads);
			}
		}
	};


	struct ModelB : public NNSequenceLayer
	{
		ModelB(int const imageSize[], int numCategory)
		{
			layerReLU.setFuncionT< NNFunc::WeakReLU >();

			//28
			layer1.init(IntVector3(imageSize[0], imageSize[1], 1), 5, 6);
			addLayer(layer1);
			addLayer(layerReLU, layer1.getOutputLength());

			//24
			layer2.init(layer1.getOutputSize(), 2);
			addLayer(layer2);

			//12
			layer3.init(IntVector3(layer2.dataSize[0], layer2.dataSize[1], layer2.numNode), 5, 16);
			addLayer(layer3);
			addLayer(layerReLU, layer3.getOutputLength());

			//8
			layer4.init(layer3.getOutputSize(), 2);
			addLayer(layer4);

			//4
			layer5.init(IntVector3(layer4.dataSize[0], layer4.dataSize[1], layer2.numNode), 4, 120);
			addLayer(layer5);
			addLayer(layerReLU, layer5.getOutputLength());

			layer6.init(layer5.getOutputLength(), numCategory);
			addLayer(layer6);

			addSoftMaxLayer(layer6.numNode);
			CHECK(mNumLayer == 10);
		}

		NNConv2DLayer layer1;
		NNAveragePooling2DLayer layer2;
		NNConv2DLayer layer3;
		NNAveragePooling2DLayer layer4;
		NNConv2DLayer layer5;
		NNLinearLayer layer6;

		NNTransformLayer layerReLU;

		static void Randomize(NNConv2DLayer const& layer, TArrayView<NNScalar> parameters)
		{
			NNScalar* pWeight = parameters.data() + layer.weightOffset;
			NNScalar* pBias = parameters.data() + layer.biasOffset;

			float halfRange = sqrt(3.0f / (layer.dataSize[0] * layer.dataSize[1] * layer.inputSize.z));
			int numWeights = layer.getWeightLength();
			for (int i = 0; i < numWeights; ++i)
			{
				pWeight[i] = (static_cast<float_t>(rand()) / RAND_MAX) * (float_t(2) * halfRange) - halfRange;
			}
			for (int i = 0; i < layer.numNode; ++i)
			{
				pBias[i] = 0;
			}
		}

		static void Randomize(NNLinearLayer const& layer, TArrayView<NNScalar> parameters, int numInput)
		{
			NNScalar* pWeight = parameters.data() + layer.weightOffset;
			NNScalar* pBias = parameters.data() + layer.biasOffset;

			float_t halfRange = sqrt(float_t(3) / numInput);

			for (int i = 0, n = numInput * layer.numNode; i < n; ++i)
			{
				pWeight[i] = (static_cast<float_t>(rand()) / RAND_MAX) * (float_t(2) * halfRange) - halfRange;
			}
			for (int i = 0; i < layer.numNode; ++i)
			{
				pBias[i] = 0;
			}

		}

		void randomize(TArrayView<NNScalar> parameters)
		{
			Randomize(layer1, parameters);
			Randomize(layer3, parameters);
			Randomize(layer5, parameters);
			Randomize(layer6, parameters, layer5.getOutputLength());
		}

		int getParameterLength() const
		{
			return mParameterLength;
		}

		int getOutputLength() const
		{
			return layer6.getOutputLength();
		}
		int getPassOutputLength() const
		{
			return mPassOutputLength;
		}
		int getTempLossGradLength() const
		{
			return 2 * mMaxOutputLength;
		}

		NNScalar* forward(
			NNScalar const parameters[],
			NNScalar const inputs[],
			NNScalar outputs[])
		{
			NNScalar const* pInput = inputs;
			pInput = FNNAlgo::Forward(layer1, parameters, pInput, outputs + mOutputOffsets[0]);
			pInput = FNNAlgo::Forward(layerReLU, layer1.getOutputLength(), pInput, outputs + mOutputOffsets[1]);
			pInput = FNNAlgo::Forward(layer2, pInput, outputs + mOutputOffsets[2]);

			pInput = FNNAlgo::Forward(layer3, parameters, pInput, outputs + mOutputOffsets[3]);
			pInput = FNNAlgo::Forward(layerReLU, layer3.getOutputLength(), pInput, outputs + mOutputOffsets[4]);
			pInput = FNNAlgo::Forward(layer4, pInput, outputs + mOutputOffsets[5]);
			
			pInput = FNNAlgo::Forward(layer5, parameters, pInput, outputs + mOutputOffsets[6]);
			pInput = FNNAlgo::Forward(layerReLU, layer4.getOutputLength(), pInput, outputs + mOutputOffsets[7]);

			pInput = FNNAlgo::Forward(layer6, parameters, pInput, outputs + mOutputOffsets[8]);

			int index = FNNMath::SoftMax(layer6.getOutputLength(), pInput, outputs + mOutputOffsets[9]);
			return outputs + mOutputOffsets[9];
		}

		void backward(
			NNScalar const parameters[],
			NNScalar const inInputs[],
			NNScalar const inOutputs[],
			NNScalar const inOutputLossGrads[],
			TArrayView<NNScalar> tempLossGrads,
			NNScalar inoutParameterGrads[],
			NNScalar* outLossGrads = nullptr)
		{

			NNScalar* pLossGrad = tempLossGrads.data();
			NNScalar* pOutputLossGrad = tempLossGrads.data() + tempLossGrads.size() / 2;

			NNScalar const* pOutput = inOutputs + mOutputOffsets[9];
			NNScalar const* pInput = inOutputs + mOutputOffsets[8];

			//FNNAlgo::SoftMaxBackward(layer7.getOutputLength(), pInput, pOutput, inOutputLossGrads, pLossGrad);

			//std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[7];
			FNNAlgo::BackwardWeight(layer6, pInput, inOutputLossGrads, inoutParameterGrads);
			FNNAlgo::BackwardLoss(layer6, parameters, inOutputLossGrads, pLossGrad);

			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[6];
			FNNAlgo::Backward(layerReLU, layer4.getOutputLength(), pInput, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[5];
			FNNAlgo::BackwardWeight(layer5, pInput, pOutput, pOutputLossGrad, inoutParameterGrads);
			FNNAlgo::BackwardLoss(layer5, parameters, pOutputLossGrad, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[4];
			FNNAlgo::Backward(layer4, pOutputLossGrad, pLossGrad);

			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[3];
			FNNAlgo::Backward(layerReLU, layer3.getOutputLength(), pInput, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[2];
			FNNAlgo::BackwardWeight(layer3, pInput, pOutput, pOutputLossGrad, inoutParameterGrads);
			FNNAlgo::BackwardLoss(layer3, parameters, pOutputLossGrad, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[1];
			FNNAlgo::Backward(layer2, pOutputLossGrad, pLossGrad);

			pOutput = pInput;
			pInput = inOutputs + mOutputOffsets[0];
			FNNAlgo::Backward(layerReLU, layer1.getOutputLength(), pInput, pLossGrad);

			std::swap(pOutputLossGrad, pLossGrad);
			pOutput = pInput;
			pInput = inInputs;
			FNNAlgo::BackwardWeight(layer1, pInput, pOutput, pOutputLossGrad, inoutParameterGrads);
			if (outLossGrads)
			{
				FNNAlgo::BackwardLoss(layer1, parameters, pOutputLossGrad, outLossGrads);
			}
		}
	};

	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		typedef StageBase BaseClass;
	public:
		TestStage()
			:mModel(ImageSize, 10)
		{

		}

		virtual bool onInit()
		{
			if (!BaseClass::onInit())
				return false;

			TestANNT();
			Test();

			loadTrainSample();
			loadTestSample();
			mOrderedSamples.resize(mTrainSamples.size());
			for (int i = 0; i < mTrainSamples.size(); ++i)
			{
				mOrderedSamples[i] = mTrainSamples[i];
			}

			mMainData.init(mModel);
			initParameters();
			mOptimizer.init(mModel.getParameterLength());


			::Global::GUI().cleanupWidget();
			auto frame = WidgetUtility::CreateDevFrame();

			frame->addButton("Clear", [this](int event, GWidget*) -> bool
			{
				return false;
			});
			frame->addCheckBox("bAutoTrain", bAutoTrain);


			restart();

			return true;
		}


		struct SampleData
		{
			TArray<NNScalar> image;
			uint8 label;
		};


		static int constexpr ImageSize[2] = {28 , 28 };

		TArray< SampleData* > mTrainSamples;
		TArray< SampleData* > mTestSamples;
		TArray< SampleData* > mOrderedSamples;
		ModelB mModel;
		AdamOptimizer mOptimizer;



		static void EndianSwap(uint32& value)
		{
			uint8* ptr = (uint8*)&value;
			std::swap(ptr[0], ptr[3]);
			std::swap(ptr[1], ptr[2]);
		}

		bool loadTrainSample()
		{
			return loadSample(TrainImageFileName, TrainLabelFileName, mTrainSamples);
		}
		bool loadTestSample()
		{
			return loadSample(TestImageFileName, TestLabelFileName, mTestSamples);
		}
		bool loadSample(char const* imageFileName, char const* labelFileName, TArray<SampleData*>& outDatas)
		{
			InlineString<> imagePath;
			imagePath.format("%s/%s", SampleFileFolder, imageFileName);

			InputFileSerializer imageSerializer;
			if (!imageSerializer.openNoHeader(imagePath))
				return false;

			struct ImageHeader
			{
				uint32 magic;
				uint32 sampleNum;
				uint32 sizeX;
				uint32 sizeY;
			};

			ImageHeader imageHeader;
			imageSerializer >> imageHeader;
			if (imageHeader.magic != 0x3080000)
				return false;

			EndianSwap(imageHeader.sampleNum);
			EndianSwap(imageHeader.sizeX);
			EndianSwap(imageHeader.sizeY);

			if(imageHeader.sizeX != ImageSize[0] || imageHeader.sizeY != ImageSize[1])
				return false;

			TArray<uint8> image;
			image.resize(ImageSize[0] * ImageSize[1]);

			for (int i = 0; i < imageHeader.sampleNum; ++i)
			{
				SampleData* trainData = new SampleData;
				trainData->image.resize( ImageSize[0] * ImageSize[1] );
				imageSerializer.read( image.data(), image.size());
				for (int n = 0; n < image.size(); ++n)
				{
					trainData->image[n] = NNScalar(image[n]) / 255.0f;
				}

				outDatas.push_back(trainData);
			}

			InlineString<> labelPath;
			labelPath.format("%s/%s", SampleFileFolder, labelFileName);

			InputFileSerializer labelSerializer;
			if (!labelSerializer.openNoHeader(labelPath))
				return false;

			struct LabelHeader
			{
				uint32 magic;
				uint32 sampleNum;
			};

			LabelHeader labelHeader;
			labelSerializer >> labelHeader;
			if (labelHeader.magic != 0x1080000)
				return false;

			EndianSwap(labelHeader.sampleNum);
			if (imageHeader.sampleNum != labelHeader.sampleNum)
				return false;

			for (int i = 0; i < imageHeader.sampleNum; ++i)
			{
				SampleData* trainData = outDatas[i];
				labelSerializer.read(trainData->label);
			}

			return true;
		}

		RHITexture2DRef mTexShow;

		virtual bool setupRenderResource(ERenderSystem systemName)
		{
			mTexShow = RHICreateTexture2D(TextureDesc::Type2D(ETexture::R32F, ImageSize[0], ImageSize[1]));

			updateShowImage();
			return true;
		}

		virtual void onEnd()
		{
			BaseClass::onEnd();
		}

		struct TrainData
		{

			TArray< NNScalar > outputs;
			TArray< NNScalar > lossGrads;
			TArray< NNScalar > parameterGrads;
			TArray< NNScalar > batchNormParameters;
			NNScalar loss;
			int count;
			void init(ModelB& model, int batchSize = 1)
			{
				batchSize = 1;
				outputs.resize(batchSize * model.getPassOutputLength());
				lossGrads.resize(batchSize * model.getTempLossGradLength());
				parameterGrads.resize(model.getParameterLength());
			}

			void reset()
			{
				std::fill_n(parameterGrads.data(), parameterGrads.size(), 0.0);
				loss = 0;
				count = 0;
			}

		};
		bool bAutoTrain = true;
		TrainData mMainData;
		void initParameters()
		{
			mParameters.resize(mModel.getParameterLength());

			mModel.randomize(mParameters);
#if 0

			NNScalar v = 0.5 * Math::Sqrt(2.0 / (ImageSize[0] * ImageSize[1]));
			for (NNScalar& x : mParameters)
			{
				x = RandFloat(0, v);
			}
#endif
		}



		NNScalar fit(SampleData const& sample, TrainData& trainData)
		{
			NNScalar const* pOutput = mModel.forward(mParameters.data(), sample.image.data(), trainData.outputs.data());


			int index = FNNMath::Max(mModel.getOutputLength(), pOutput);
			if (index == sample.label)
			{
				++trainData.count;
			}


			NNScalar lossGrads[10];

			NNScalar loss = 0;
			for (int i = 0; i < mModel.getOutputLength(); ++i)
			{
				NNScalar label = (sample.label == i) ? 1.0 : 0.0;
				lossGrads[i] = LossFunc::CalcSoftMaxDevivative(pOutput[i], label);
				loss += LossFunc::Calc(pOutput[i], label);
			}

			mModel.backward(mParameters.data(), sample.image.data(), trainData.outputs.data(), lossGrads, trainData.lossGrads, trainData.parameterGrads.data());

			return loss;
		}

		using LossFunc = FCrossEntropyLoss;

		TArray<NNScalar> mParameters;

		float learnRate = 1e-3;

		int mEpoch = 0;


		NNScalar trainBatch(TArrayView<SampleData*> samples)
		{
			mMainData.reset();
			NNScalar loss = 0;
			for (int i = 0; i < samples.size(); ++i)
			{
				loss += fit(*samples[i], mMainData);
			}

			for (int i = 0; i < mMainData.parameterGrads.size(); ++i)
			{
				auto& parameterGrad = mMainData.parameterGrads[i];
				parameterGrad /= samples.size();
#if 1
				if (!FNNMath::IsValid(parameterGrad))
				{
					parameterGrad = Math::Sign(parameterGrad) * 1e15;
				}
#endif

				parameterGrad += 2 * 0.001 * mParameters[i];
			}
			loss /= (float)samples.size();
			FNNMath::ClipNormalize(mMainData.parameterGrads.size(), mMainData.parameterGrads.data(), 1.0);

			mOptimizer.update(mParameters, mMainData.parameterGrads, learnRate);

#if 1
			for (int i = 0; i < mParameters.size(); ++i)
			{
				if (!FNNMath::IsValid(mParameters[i]))
				{
					LogMsg("AAAAAAAAAA, %g", mParameters[i]);
					break;
				}
			}
#endif

			return loss;
		}

		template< typename Func >
		void train(double timeLimit, Func& timeLimitCB)
		{
			uint64 startTime;
			Profile_GetTicks(&startTime);

			int const BatchSize = 30;
			std::mt19937 g(::rand());
			for (mEpoch = 0; mEpoch < 20; ++mEpoch)
			{

				std::shuffle(mOrderedSamples.begin(), mOrderedSamples.end(), g);

				int numBatch = (mOrderedSamples.size() + BatchSize - 1) / BatchSize;

				for (int i = 0; i < numBatch; ++i)
				{
					int numSample = Math::Min<int>(mOrderedSamples.size() - i * BatchSize, BatchSize);
					float loss = trainBatch(TArrayView<SampleData*>(mOrderedSamples.data() + i * BatchSize, numSample));
					LogMsg("%d %d: Loss = %f, %d / %d", mEpoch, i, loss, mMainData.count, numSample);

					if (timeLimit > 0)
					{
						uint64 endTime;
						Profile_GetTicks(&endTime);
						if (double(endTime - startTime) / 1000.0 > timeLimit)
						{
							startTime = endTime;
							timeLimitCB();
						}
					}
				}

				testSamples();
			}
		}



		virtual void onUpdate(GameTimeSpan deltaTime)
		{
			BaseClass::onUpdate(deltaTime);

			if (bAutoTrain)
			{
				Coroutines::Resume(mTrainHandle);
				test();
			}
		}

		Coroutines::ExecutionHandle mTrainHandle;
		
		void restart()
		{
			mTrainHandle = Coroutines::Start([this]
			{
				train(15, []()
				{
					CO_YEILD(nullptr);
				});
			});
		}

		int pred = 0;


		void onRender(float dFrame) override
		{
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			auto screenSize = ::Global::GetScreenSize();


			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.0, 0.0, 0.0, 0.0), 1);
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();

			g.beginRender();
			RenderUtility::SetBrush(g, EColor::White);
			g.drawTexture(*mTexShow, Vector2(20,20), Vector2(200,200));
			RenderUtility::SetPen(g, EColor::White);
			RenderUtility::SetBrush(g, EColor::Null);
			g.drawRect(Vector2(20, 20), Vector2(200, 200));


			g.drawTextF(Vector2(10, 10), "Index = %d Number = %d Pred = %d", indexShow, mTestSamples[indexShow]->label, pred);
			g.endRender();
		}

		void testSamples()
		{
			int num = 0;
			for (int i = 0; i < mTestSamples.size(); ++i)
			{
				auto sample = mTestSamples[i];
				NNScalar* output = mModel.forward(mParameters.data(), sample->image.data(), mMainData.outputs.data());
				int pred = FNNMath::Max(10, output);
				if (pred == sample->label)
				{
					++num;
				}
			}

			LogMsg("%d: %f %%", mEpoch, float(100 * num) / mTestSamples.size());
		}


		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		void test()
		{
			indexShow = rand() % mTestSamples.size();
			auto sample = mTestSamples[indexShow];
			NNScalar* output = mModel.forward(mParameters.data(), sample->image.data(), mMainData.outputs.data());
			pred = FNNMath::Max(10, output);
			updateShowImage();
		}

		void updateShowImage()
		{
			auto sample = mTestSamples[indexShow];
			RHIUpdateTexture(*mTexShow, 0, 0, ImageSize[0], ImageSize[1], sample->image.data());
		}

		int indexShow = 0;
		MsgReply onKey(KeyMsg const& msg)
		{
			switch (msg.getCode())
			{
			case EKeyCode::X: ++indexShow; if (indexShow >= mTestSamples.size()) indexShow = 0; updateShowImage(); break;
			case EKeyCode::Z: --indexShow; if (indexShow < 0) indexShow = mTestSamples.size() - 1; updateShowImage(); break;
			default:
				break;
			}
			return MsgReply::Unhandled();
		}

		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.screenWidth = 1080;
			systemConfigs.screenHeight = 768;
		}

	};

	REGISTER_STAGE_ENTRY("Number Recognizer", TestStage, EExecGroup::Dev, "AI");
}