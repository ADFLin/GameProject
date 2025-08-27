
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

namespace NR
{
	char const* TrainFileFolder = "mnist";
	char const* TrainFileImageName = "train-images.idx3-ubyte";
	char const* TrainFileLabelName = "train-labels.idx1-ubyte";
	using namespace Render;

	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		typedef StageBase BaseClass;
	public:
		TestStage() {}

		virtual bool onInit()
		{
			if (!BaseClass::onInit())
				return false;

			loadTrainData();

			::Global::GUI().cleanupWidget();
			auto frame = WidgetUtility::CreateDevFrame();

			frame->addButton("Clear", [this](int event, GWidget*) -> bool
			{
				return false;
			});

			return true;
		}


		struct TrainData
		{
			TArray<uint8> image;
			uint8 label;
		};


		static int constexpr ImageSize[2] = {28 , 28 };

		TArray< TrainData* > mTrainDatas;


		struct Model
		{
			Model()
			{
				layer1.numNode = 8;
				layer1.convSize = 3;
				layer1.dataSize[0] = ImageSize[0] - layer1.convSize + 1;
				layer1.dataSize[1] = ImageSize[1] - layer1.convSize + 1;
				layer1.setFuncionT< NNFunc::ReLU >();
				addLayer(layer1, 1);

				layer2.numNode = 8;
				layer2.convSize = 3;
				layer2.dataSize[0] = layer1.dataSize[0] - layer2.convSize + 1;
				layer2.dataSize[1] = layer1.dataSize[1] - layer2.convSize + 1;
				layer2.setFuncionT< NNFunc::ReLU >();
				addLayer(layer2, layer1.numNode);

				layer3.numNode = layer2.numNode;
				layer3.poolSize = 2;
				layer3.dataSize[0] = layer2.dataSize[0] / layer3.poolSize;
				layer3.dataSize[1] = layer2.dataSize[1] / layer3.poolSize;

				layer4.numNode = 10;
				layer4.convSize = 3;
				layer4.dataSize[0] = layer3.dataSize[0] - layer4.convSize + 1;
				layer4.dataSize[1] = layer3.dataSize[1] - layer4.convSize + 1;
				layer4.setFuncionT< NNFunc::ReLU >();
				addLayer(layer4, layer3.numNode);

				layer5.numNode = 10;
				layer5.convSize = 3;
				layer5.dataSize[0] = layer4.dataSize[0] - layer5.convSize + 1;
				layer5.dataSize[1] = layer4.dataSize[1] - layer5.convSize + 1;
				layer5.setFuncionT< NNFunc::ReLU >();
				addLayer(layer5, layer4.numNode);

				layer6.numNode = layer5.numNode;
				layer6.poolSize = 2;
				layer6.dataSize[0] = layer5.dataSize[0] / layer6.poolSize;
				layer6.dataSize[1] = layer5.dataSize[1] / layer6.poolSize;

				layer7.numNode = 10;
				layer7.setFuncionT< NNFunc::Linear >();
				addLayer(layer7, layer6.getOutputLength());
			}

			void addLayer(NeuralConv2DLayer& inoutlayer, int numSliceInput)
			{
				int const convLen = inoutlayer.convSize * inoutlayer.convSize;
				int const nodeWeightLen = numSliceInput * convLen;
				int numParameters = (nodeWeightLen + 1) * inoutlayer.numNode;

				inoutlayer.weightOffset = mParametersLength;
				inoutlayer.biasOffset = mParametersLength + nodeWeightLen * inoutlayer.numNode;
				inoutlayer.fastMethod = NeuralConv2DLayer::eNone;

				mOutputOffsets[mNumLayer] = mOutputLength;
				++mNumLayer;
				mOutputLength += 2 * inoutlayer.getOutputLength();
				mLossGradLength += inoutlayer.getOutputLength();

				mParametersLength += numParameters;
			}

			void addLayer(NeuralFullConLayer& inoutlayer, int numInput)
			{
				int numParameters = (numInput + 1) * inoutlayer.numNode;

				inoutlayer.weightOffset = mParametersLength;
				inoutlayer.biasOffset = mParametersLength + numInput * inoutlayer.numNode;

				mOutputOffsets[mNumLayer] = mOutputLength;
				++mNumLayer;
				mOutputLength += 2 * inoutlayer.getOutputLength();
				mLossGradLength += inoutlayer.getOutputLength();

				mParametersLength += numParameters;
			}

			void addLayer(NeuralMaxPooling2DLayer& inoutlayer, int numInput)
			{
				int numParameters = (numInput + 1) * inoutlayer.numNode;

				mOutputOffsets[mNumLayer] = mOutputLength;
				++mNumLayer;
				mOutputLength += 2 * inoutlayer.getOutputLength();
				mLossGradLength += inoutlayer.getOutputLength();

				mParametersLength += numParameters;
			}

			int mParametersLength = 0;
			int mOutputLength = 0;
			int mOutputOffsets[7];
			int mNumLayer;
			int mLossGradLength = 0;

			NeuralConv2DLayer layer1;
			NeuralConv2DLayer layer2;
			NeuralMaxPooling2DLayer layer3;
			NeuralConv2DLayer layer4;
			NeuralConv2DLayer layer5;
			NeuralMaxPooling2DLayer layer6;
			NeuralFullConLayer layer7;

			void forwardPass(NNScalar const parameters[], NNScalar const inputs[], NNScalar outOutputs[])
			{
				NNScalar const* pInput = inputs;
				pInput = FNNAlgo::ForwardFeedback(layer1, parameters, 1, ImageSize, pInput, outOutputs + mOutputOffsets[0], true);
				pInput = FNNAlgo::ForwardFeedback(layer2, parameters, layer1.numNode, layer1.dataSize, pInput, outOutputs + mOutputOffsets[1], true);
				pInput = FNNAlgo::ForwardFeedback(layer3, layer2.dataSize, pInput, outOutputs + mOutputOffsets[2]);
				pInput = FNNAlgo::ForwardFeedback(layer4, parameters, layer3.numNode, layer3.dataSize, pInput, outOutputs + mOutputOffsets[3], true);
				pInput = FNNAlgo::ForwardFeedback(layer5, parameters, layer4.numNode, layer4.dataSize, pInput, outOutputs + mOutputOffsets[4], true);
				pInput = FNNAlgo::ForwardFeedback(layer6, layer5.dataSize, pInput, outOutputs + mOutputOffsets[5]);
				pInput = FNNAlgo::ForwardFeedback(layer7, parameters, layer6.getOutputLength(), pInput, outOutputs + mOutputOffsets[6], true);

				//int index = FNNMath::SoftMax(layer7.getOutputLength(), output + mOutputOffsets[6], mResult.data());
			}

			void backwardPass(NNScalar const parameters[], NNScalar const inLossDerivatives[], NNScalar const inputs[], NNScalar const inOutputs[], NNScalar outLossGrads[], NNScalar outDeltaWeights[])
			{



			}

		};

		static void EndianSwap(uint32& value)
		{
			uint8* ptr = (uint8*)&value;
			std::swap(ptr[0], ptr[3]);
			std::swap(ptr[1], ptr[2]);
		}

		bool loadTrainData()
		{
			InlineString<> imagePath;
			imagePath.format("%s/%s", TrainFileFolder, TrainFileImageName);


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

			for (int i = 0; i < imageHeader.sampleNum; ++i)
			{

				TrainData* trainData = new TrainData;
				trainData->image.resize( ImageSize[0] * ImageSize[1] );
				imageSerializer.read( trainData->image.data(), trainData->image.size() );

				mTrainDatas.push_back(trainData);
			}


			InlineString<> labelPath;
			labelPath.format("%s/%s", TrainFileFolder, TrainFileLabelName);

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
				TrainData* trainData = mTrainDatas[i];
				labelSerializer.read(trainData->label);
			}

			return true;
		}



		RHITexture2DRef mTexShow;

		virtual bool setupRenderResource(ERenderSystem systemName)
		{
			mTexShow = RHICreateTexture2D(TextureDesc::Type2D(ETexture::R8, ImageSize[0], ImageSize[1]));

			updateShowImage();
			return true;
		}

		virtual void onEnd()
		{
			BaseClass::onEnd();
		}

		void restart()
		{

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
			g.drawTexture(*mTexShow, Vector2(20,20), Vector2(200,200));
			RenderUtility::SetPen(g, EColor::White);
			RenderUtility::SetBrush(g, EColor::Null);
			g.drawRect(Vector2(20, 20), Vector2(200, 200));
			g.endRender();
		}


		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		void updateShowImage()
		{
			auto dataShow = mTrainDatas[indexShow];
			RHIUpdateTexture(*mTexShow, 0, 0, ImageSize[0], ImageSize[1], dataShow->image.data());
		}

		int indexShow = 0;
		MsgReply onKey(KeyMsg const& msg)
		{
			switch (msg.getCode())
			{
			case EKeyCode::X: ++indexShow; if (indexShow > mTrainDatas.size()) indexShow = 0; updateShowImage(); break;
			case EKeyCode::Z: --indexShow; if (indexShow < 0) indexShow = mTrainDatas.size() - 1; updateShowImage(); break;
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