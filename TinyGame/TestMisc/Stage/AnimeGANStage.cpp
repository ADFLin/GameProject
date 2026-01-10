#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHIGraphics2D.h"

#include "Core/IntegerType.h"
#include "MacroCommon.h"

#include "AI/NeuralNetwork.h"

#include <fstream>
#include "DataCacheInterface.h"
#include "RandomUtility.h"
#include "ProfileSystem.h"
#include <random>

using namespace Render;

#define WEIGHT_VERSION 3

struct NNBatchNormlizeLayer : NeuralLayer
{

	int getPassOutputLength() const
	{
		return 3 * length * numNode;
	}
	int getOutputLength() const
	{
		return length * numNode;
	}


	int length;
	int batchLength = 1;
	int meanOffset;
	int varianceOffset;

	int setParameterOffset(int offset)
	{
		weightOffset = offset;
		offset += numNode;
		biasOffset = offset;
		offset += numNode;
		meanOffset = offset;
		offset += numNode;
		varianceOffset = offset;
		offset += numNode;
		return offset;
	}

	static NNScalar* Inference(
		NNBatchNormlizeLayer const& layer, NNScalar const* parameters,
		NNScalar const inputs[],
		NNScalar outputs[])
	{
		NNScalar const* pWeight = parameters + layer.weightOffset;
		NNScalar const* pBias = parameters + layer.biasOffset;
		NNScalar const* pMean = parameters + layer.meanOffset;
		NNScalar const* pVariance = parameters + layer.varianceOffset;

		for (int indexInput = 0; indexInput < layer.numNode; ++indexInput)
		{
			NNScalar mean = *pMean;
			NNScalar variance = *pVariance;

			int index = indexInput * layer.length;

			NNScalar invVar = 1.0 / Math::Sqrt(variance + 1e-5);
			NNScalar alpha = invVar * (*pWeight);
			NNScalar beta = *pBias - mean * alpha;
			for (int i = 0; i < layer.length; ++i)
			{
				outputs[index] = alpha * inputs[index] + beta;
				++index;
			}

			++pMean;
			++pVariance;
			++pWeight;
			++pBias;
		}

		return outputs;
	}

	static NNScalar* Forward(
		NNBatchNormlizeLayer const& layer, NNScalar const* parameters,
		NNScalar const inputs[],
		NNScalar outputs[])
	{

		NNScalar const* pWeight = parameters + layer.weightOffset;
		NNScalar const* pBias = parameters + layer.biasOffset;

		NNScalar const* pInput = inputs;
		NNScalar* pOutput = outputs;
		NNScalar* pMean = pOutput + layer.numNode;
		NNScalar* pVariance = pMean + layer.numNode;
		for (int indexInput = 0; indexInput < layer.numNode; ++indexInput)
		{
			NNScalar mean, variance;
			FNNMath::GetNormalizeParams(layer.batchLength * layer.length, pInput, layer.numNode, mean, variance);
			*pMean = mean;
			++pMean;
			*pVariance = variance;
			++pVariance;

			for (int i = 0; i < layer.batchLength; ++i)
			{
				int index = i * layer.numNode;
				for (int j = 0; j < layer.length; ++j)
				{
					outputs[index + j] = (pInput[index + j] - mean) / Math::Sqrt(variance + 1e-5) * (*pWeight) + (*pBias);
				}

			}

			++pWeight;
			++pBias;
			pInput += layer.length;
			pOutput += layer.length;
		}

		return outputs;
	}

	static void BackwardLoss(
		NNBatchNormlizeLayer const& layer, NNScalar const* parameters,
		NNScalar const inInputs[],
		NNScalar const inOutputs[],
		NNScalar const inLossGrads[],
		NNScalar outLossGrads[])
	{
		NNScalar const* pWeight = parameters + layer.weightOffset;
		NNScalar const* pInput = inInputs;
		NNScalar const* pInLossGrad = inLossGrads;
		NNScalar* pLossGrad = outLossGrads;

		for (int indexInput = 0; indexInput < layer.numNode; ++indexInput)
		{
			NNScalar mean, variance;
			FNNMath::GetNormalizeParams(layer.batchLength, pInput, layer.numNode, mean, variance);
			for (int i = 0; i < layer.batchLength; ++i)
			{
				int index = i * layer.numNode;
				//mean = IiEi / N
				//variance = sqrt( (Ii - mean)(Ii - mean) / N )
				//Oi = (Ii - mean) / variance * w + b;

				pLossGrad[index] = 0;
				for (int j = 0; j < layer.batchLength; ++j)
				{
					int indexJ = j * layer.numNode;
					pLossGrad[index] += (((i == j) ? 1 : 0 - 1.0 / layer.numNode) / variance) * (1 - Math::Square((pInput[indexJ] - mean) / variance) / layer.numNode) * *pWeight;
				}
			}

			++pLossGrad;
			++pInLossGrad;
			++pWeight;
		}

	}

	static void BackwardWeight(
		NNBatchNormlizeLayer const& layer,
		NNScalar const inInputs[],
		NNScalar const inLossGrads[],
		NNScalar inoutParameterGrads[])
	{
		NNScalar* pWeightGrads = inoutParameterGrads + layer.weightOffset;
		NNScalar* pBiasGrads = inoutParameterGrads + layer.biasOffset;

		NNScalar const* pInput = inInputs;

		NNScalar const* pInLossGrad = inLossGrads;
		for (int indexInput = 0; indexInput < layer.numNode; ++indexInput)
		{
			NNScalar mean, variance;
			FNNMath::GetNormalizeParams(layer.batchLength, pInput, layer.numNode, mean, variance);
			for (int i = 0; i < layer.batchLength; ++i)
			{
				int index = i * layer.numNode;
				*pBiasGrads += pInLossGrad[index];
				*pWeightGrads += pInLossGrad[index] * (pInput[index] - mean) / variance;
			}
			++pInLossGrad;

			++pWeightGrads;
			++pBiasGrads;
			++pInput;
		}

	}
};;

struct NNConvTranspose2DLayer : NeuralLayer
{
	IntVector3 inputSize;
	IntVector2 dataSize;


	int stride;
	int padding;
	int convSize;

	void init(IntVector3 const& inInputSize, int inNumNode, int inConvSize, int inPadding = 0, int inStride = 1)
	{
		inputSize = inInputSize;
		convSize = inConvSize;
		numNode = inNumNode;
		padding = inPadding;
		stride = inStride;

		int dilation = 1;
		dataSize[0] = (inputSize[0] - 1) * stride - 2 * padding + dilation * (convSize - 1) + 1;
		dataSize[1] = (inputSize[1] - 1) * stride - 2 * padding + dilation * (convSize - 1) + 1;
	}


	static NNScalar* Forward(
		NNConvTranspose2DLayer const& layer, NNScalar const parameters[],
		NNScalar const inputs[],
		NNScalar outputs[])
	{
		int numSliceInput = layer.inputSize[2];

		int const nx = layer.dataSize[0];
		int const ny = layer.dataSize[1];

		NNScalar const* pWeight = parameters + layer.weightOffset;
		NNScalar const* pBias = parameters + layer.biasOffset;

		int const sliceInputLength = layer.inputSize[0] * layer.inputSize[1];
		int const convLength = layer.convSize * layer.convSize;
		int const nodeOutputLength = nx * ny;

		int const nodeWeightSize = numSliceInput * convLength;


		NNScalar const* pNodeWeight = pWeight;


		if (layer.stride > 0)
		{
			NNScalar* pNodeOutput = outputs;
			for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
			{
				FNNMath::Fill(nodeOutputLength, pNodeOutput, pBias[idxNode]);
				pNodeOutput += nodeOutputLength;
			}

			NNScalar const* pSliceInput = inputs;
			for (int idxSlice = 0; idxSlice < numSliceInput; ++idxSlice)
			{
				pNodeOutput = outputs;
				for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
				{
					FNNMath::DeConv(layer.inputSize[0], layer.inputSize[1], pSliceInput, layer.convSize, layer.convSize, pNodeWeight, layer.stride, layer.padding, pNodeOutput);

					pNodeOutput += nodeOutputLength;
					pNodeWeight += convLength;
				}

				pSliceInput += sliceInputLength;
			}
		}
		else
		{

#define USE_ROTATE_180_FUNC 1
#if USE_ROTATE_180_FUNC

#else
			NNScalar* weightRotated = (NNScalar*)alloca(sizeof(NNScalar) * convLength);
#endif

			NNScalar* pNodeOutput = outputs;
			for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
			{
				FNNMath::Fill(nodeOutputLength, pNodeOutput, pBias[idxNode]);
				pNodeOutput += nodeOutputLength;
			}

			NNScalar const* pSliceInput = inputs;
			for (int idxSlice = 0; idxSlice < numSliceInput; ++idxSlice)
			{
				pNodeOutput = outputs;
				for (int idxNode = 0; idxNode < layer.numNode; ++idxNode)
				{
#if USE_ROTATE_180_FUNC
					FNNMath::FullConvRotate180(layer.inputSize[0], layer.inputSize[1], pSliceInput, layer.convSize, layer.convSize, pNodeWeight, pNodeOutput, -layer.padding);
#else
					FNNMath::Rotate180(layer.convSize, layer.convSize, pNodeWeight, weightRotated);
					FNNMath::FullConv(layer.inputSize[0], layer.inputSize[1], pSliceInput, layer.convSize, layer.convSize, weightRotated, layer.stride, pNodeOutput, -layer.padding);
#endif
					pNodeOutput += nodeOutputLength;
					pNodeWeight += convLength;
				}

				pSliceInput += sliceInputLength;
			}	 
		}
		return outputs;
	}

	int getParameterLength() const
	{
		return numNode * (inputSize.z * convSize * convSize + 1);
	}

	int getWeightLength() const
	{
		return numNode * (inputSize.z * convSize * convSize);
	}

	int getPassOutputLength() const
	{
		return dataSize[0] * dataSize[1] * numNode;
	}

	int getOutputLength() const
	{
		return dataSize[0] * dataSize[1] * numNode;
	}

	IntVector3 getOutputSize() const
	{
		return IntVector3(dataSize.x, dataSize.y, numNode);
	}

	int setParameterOffset(int offset)
	{
		weightOffset = offset;
		biasOffset = weightOffset + getWeightLength();
		return offset + getParameterLength();
	}
};


template< class T1, class ...Args >
typename std::common_type< T1, Args... >::type
Max(T1&& t1, Args&&... args)
{
	auto temp = Max(std::forward<Args>(args)...);
	return t1 > temp ? t1 : temp;
}

template< class T1, class T2 >
auto Max(T1&& t1, T2&& t2)
{
	return t1 > t2 ? t1 : t2;
}


struct Model
{

	struct LeakyReLU
	{
#if WEIGHT_VERSION == 2
		static NNScalar constexpr NSlope = 0.001;
#else
		static NNScalar constexpr NSlope = 0.01;
#endif
		static NNScalar Value(NNScalar value)
		{
			return (value > 0) ? value : NSlope * value;
		}

		static NNScalar Derivative(NNScalar value)
		{
			return (value > 0) ? NNScalar(1) : NSlope;
		}

		static NNScalar LossGrad(NNScalar input, NNScalar output, NNScalar lossGrad)
		{
			return  (input > 0) ? lossGrad : NSlope * lossGrad;
		}
	};

	Model()
	{
#if WEIGHT_VERSION == 1
		layerReLU.setFuncionT<NNFunc::ReLU>();
#else
		layerReLU.setFuncionT<LeakyReLU>();
#endif
		layerTanh.setFuncionT<NNFunc::Tanh>();

		int offset = 0;

		layer1.init(IntVector3(1, 1, 128), 512, 4, 0, 1);
		offset = layer1.setParameterOffset(offset);

		layer2.length = layer1.dataSize.x * layer1.dataSize.y;
		layer2.numNode = layer1.numNode;
		offset = layer2.setParameterOffset(offset);

		layer3.init(IntVector3(layer1.dataSize[0], layer1.dataSize[1], 512), 256, 4, 1, 2);
		offset = layer3.setParameterOffset(offset);

		layer4.length = layer3.dataSize.x * layer3.dataSize.y;
		layer4.numNode = layer3.numNode;
		offset = layer4.setParameterOffset(offset);

		layer5.init(IntVector3(layer3.dataSize[0], layer3.dataSize[1], 256), 128, 4, 1, 2);
		offset = layer5.setParameterOffset(offset);
		
		layer6.length = layer5.dataSize.x * layer5.dataSize.y;
		layer6.numNode = layer5.numNode;
		offset = layer6.setParameterOffset(offset);

		layer7.init(IntVector3(layer5.dataSize[0], layer5.dataSize[1], 128), 64, 4, 1, 2);
		offset = layer7.setParameterOffset(offset);

		layer8.length = layer7.dataSize.x * layer7.dataSize.y;
		layer8.numNode = layer7.numNode;
		offset = layer8.setParameterOffset(offset);

		layer9.init(IntVector3(layer7.dataSize[0], layer7.dataSize[1], 64), 3, 4, 1, 2);
		offset = layer9.setParameterOffset(offset);
	}

	void inference(
		NNScalar const parameters[],
		NNScalar const inputs[],
		NNScalar outputs[])
	{
		int maxOutputLength = getMaxOutputLength();
		NNScalar* pInput = (NNScalar*)alloca(2 * maxOutputLength * sizeof(NNScalar));
		NNScalar* pOutput = pInput + maxOutputLength;

		NNConvTranspose2DLayer::Forward(layer1, parameters, inputs, pOutput);

		std::swap(pInput, pOutput);
		NNBatchNormlizeLayer::Inference(layer2, parameters, pInput, pOutput);
		FNNAlgo::Forward(layerReLU, layer2.getOutputLength(), pOutput);

		std::swap(pInput, pOutput);
		NNConvTranspose2DLayer::Forward(layer3, parameters, pInput, pOutput);

		std::swap(pInput, pOutput);
		NNBatchNormlizeLayer::Inference(layer4, parameters, pInput, pOutput);
		FNNAlgo::Forward(layerReLU, layer4.getOutputLength(), pOutput);

		std::swap(pInput, pOutput);
		NNConvTranspose2DLayer::Forward(layer5, parameters, pInput, pOutput);

		std::swap(pInput, pOutput);
		NNBatchNormlizeLayer::Inference(layer6, parameters, pInput, pOutput);
		FNNAlgo::Forward(layerReLU, layer6.getOutputLength(), pOutput);

		std::swap(pInput, pOutput);
		NNConvTranspose2DLayer::Forward(layer7, parameters, pInput, pOutput);

		std::swap(pInput, pOutput);
		NNBatchNormlizeLayer::Inference(layer8, parameters, pInput, pOutput);
		FNNAlgo::Forward(layerReLU, layer8.getOutputLength(), pOutput);

		std::swap(pInput, pOutput);
		NNConvTranspose2DLayer::Forward(layer9, parameters, pInput, outputs);
		FNNAlgo::Forward(layerTanh, layer9.getOutputLength(), outputs);
	}

	int getMaxOutputLength() const
	{
		return Max(
			layer1.getOutputLength(), 
			layer2.getOutputLength(), 
			layer3.getOutputLength(),
			layer4.getOutputLength(),
			layer5.getOutputLength(),
			layer6.getOutputLength(),
			layer7.getOutputLength(),
			layer8.getOutputLength(),
			layer9.getOutputLength()
		);
	}

	int getOutputLength() const
	{
		return layer9.getOutputLength();
	}

	NNTransformLayer layerReLU;
	NNTransformLayer layerTanh;

	NNConvTranspose2DLayer layer1;
	NNBatchNormlizeLayer layer2;
	NNConvTranspose2DLayer layer3;
	NNBatchNormlizeLayer layer4;
	NNConvTranspose2DLayer layer5;
	NNBatchNormlizeLayer layer6;
	NNConvTranspose2DLayer layer7;
	NNBatchNormlizeLayer layer8;
	NNConvTranspose2DLayer layer9;
};


class AnimeGANStage : public StageBase
	                , public IGameRenderSetup
{
	using BaseClass = StageBase;
public:
	AnimeGANStage() {}

	bool onInit() override
	{
		if (!BaseClass::onInit())
			return false;
		::Global::GUI().cleanupWidget();

		if (!loadWeight())
			return false;

		FNNMath::Fill(128, mInputs, 0.5);

		auto debugFrame = WidgetUtility::CreateDevFrame();
		int length = 140;
		GFrame* frame = new GFrame(UI_ANY, Vec2i(400, 200), Vec2i(4 * length + 5 * 10, 680), nullptr);
		debugFrame->addButton("Rand", [this, frame](int, GWidget*)
		{
			std::random_device rd{};
			std::mt19937 gen{ rd() };
			std::normal_distribution d{ 0.0, 1.0 };

			for (int i = 0; i < ARRAY_SIZE(mInputs); ++i)
			{
				mInputs[i] = d(gen);
			}
			frame->refresh();
			updateImage(mInputs);
			return true;
		});

		for( int i = 0 ; i < 128; ++i )
		{
			GSlider* slider = new GSlider(UI_ANY, Vec2i(10 + (length + 10) * (i / 32), 30 + 20 * (i % 32) ), length, true, frame);
			FWidgetProperty::Bind(slider, mInputs[i], -1.0, 1.0, [this](float value)
			{
				updateImage(mInputs);
			});
		}
		::Global::GUI().addWidget(frame);

		restart();
		return true;
	}

	bool loadWeight()
	{
#if WEIGHT_VERSION == 1
		char const* path = "GAN/AnimeWeights1.txt";
#elif WEIGHT_VERSION == 2
		char const* path = "GAN/AnimeWeights2.txt";
#else
		char const* path = "GAN/AnimeWeights5.txt";
#endif

		auto WeightLoad = [this](IStreamSerializer& serializer) -> bool
		{
			serializer >> mParametersCounts;
			serializer >> mParameters;

			return true;
		};

		auto WeightSave = [this](IStreamSerializer& serializer) -> bool
		{
			serializer << mParametersCounts;
			serializer << mParameters;

			return true;
		};

		DataCacheKey key;
		key.typeName = "NNPARAMS";
		key.version = "B3A381DE-31E3-4E3F-9954-447526616027";
		key.keySuffix.add(path);
		key.keySuffix.addFileAttribute(path);

		if (!::Global::DataCache().loadDelegate(key, WeightLoad))
		{
			if (!loadWeightActual(path))
				return false;

			if (!::Global::DataCache().saveDelegate(key, WeightSave))
			{

			}
		}

		return true;
	}


	Model mModel;
	TArray<int> mParametersCounts;
	TArray<NNScalar> mParameters;
	bool loadWeightActual(char const* path)
	{
		std::ifstream fs(path);

		if (!fs.is_open())
			return false;

		mParameters.clear();
		while (!fs.eof())
		{
			int numParam;
			fs >> numParam;
			mParametersCounts.push_back(numParam);
			int offest = mParameters.size();
			mParameters.resize(mParameters.size() + numParam);

			NNScalar* pParam = mParameters.data() + offest;
			for (int i = 0; i < numParam; ++i)
			{
				fs >> *pParam;
				++pParam;
			}
		}

		return true;
	}

	void onEnd() override
	{
		BaseClass::onEnd();
	}

	void restart() {}

	void onUpdate(GameTimeSpan deltaTime) override
	{
		BaseClass::onUpdate(deltaTime);
	}

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
		g.drawTexture(*mTexture, Vector2(100, 100), Vector2(200, 200));
		g.endRender();
	}

	MsgReply onMouse(MouseMsg const& msg) override
	{
		return BaseClass::onMouse(msg);
	}

	MsgReply onKey(KeyMsg const& msg) override
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			}
		}
		return BaseClass::onKey(msg);
	}

	bool onWidgetEvent(int event, int id, GWidget* ui) override
	{
		switch (id)
		{
		default:
			break;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	
	}


	RHITexture2DRef mTexture;

	void updateImage(NNScalar inputs[])
	{
		TArray<NNScalar> outputs;
		outputs.resize(mModel.getOutputLength());
		{
			TIME_SCOPE("Model Inference");
			mModel.inference(mParameters.data(), inputs, outputs.data());
		}

		TArray<float> imageData;
		imageData.resize(outputs.size());

		auto Conv = [](float* color)
		{
			Vector3 hsv = FColorConv::RGBToHSV(color);

			hsv.z = Math::Min<float>( 1.5 * hsv.z, 1.0 );
			Color3f colorConv = FColorConv::HSVToRGB(hsv);

			color[0] = colorConv.r;
			color[1] = colorConv.g;
			color[2] = colorConv.b;
		};

		for (int i = 0; i < 64 * 64; ++i)
		{
			float* color = imageData.data() + 3 * i;
			color[0] = outputs[64 * 64 * 0 + i];
			color[1] = outputs[64 * 64 * 1 + i];
			color[2] = outputs[64 * 64 * 2 + i];

			Conv(color);
		}
		RHIUpdateTexture(*mTexture, 0, 0, mTexture->getSizeX(), mTexture->getSizeY(), imageData.data());

	}

	NNScalar mInputs[128];

	virtual bool setupRenderResource(ERenderSystem systemName)
	{
		auto CreateLayerTexture = [](int w, int h, void* data = nullptr)
		{
			return RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA32F, w, h));
		};

		mTexture = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGB32F, 64, 64));


		updateImage(mInputs);
		return true;
	}

	void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
	{
		systemConfigs.screenWidth = 1080;
		systemConfigs.screenHeight = 768;
	}

protected:
};

REGISTER_STAGE_ENTRY("Anime GAN", AnimeGANStage, EExecGroup::Dev, "AI");