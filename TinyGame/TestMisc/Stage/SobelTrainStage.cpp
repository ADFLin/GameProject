
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
#include "RHI/RHIUtility.h"
#include "Image/ImageData.h"
#include "Image/ImageProcessing.h"
#include "RandomUtility.h"
#include "AI/NNTrain.h"

namespace SobelTrain
{
	using namespace Render;

	float const Gx[] = { 1 , 0 , -1 , 2 , 0 , -2 ,  1, 0 , -1 };
	//float const Gy[] = { 1 , 2 ,  1 , 0 , 0 , 0 ,  -1, -2 , -1 };

	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		typedef StageBase BaseClass;
	public:
		TestStage() {}


		TArray< NNScalar > mSobelParams =
		{ 
			0, 0,
			1, 0, -1, 2, 0,-2, 1, 0, -1, 
			1, 2, 1, 0, 0, 0, -1, -2, -1, 
		};

		NNConv2DLayer layer;

		static int constexpr ImageSize[] = { 28 , 28 };


		virtual bool onInit()
		{
			if (!BaseClass::onInit())
				return false;

			::Global::GUI().cleanupWidget();
			auto frame = WidgetUtility::CreateDevFrame();

			frame->addButton("Step", [this](int event, GWidget* widget)
			{
				trainStep();
				return false;
			});
			return true;
		}


		RHITexture2DRef mTexImage;
		RHITexture2DRef mTexInput;
		RHITexture2DRef mTexTarget;
		RHITexture2DRef mTexOutput;

		TArray<NNScalar> mParameters;

		TArray<NNScalar> mInput;
		TArray<NNScalar> mTarget;

		IntVector2 mImageSize;

		virtual bool setupRenderResource(ERenderSystem systemName)
		{
			ImageData imageData;
			imageData.load("Texture/SobelTrain.png", ImageLoadOption().UpThreeComponentToFour());
			mImageSize = IntVector2(imageData.width, imageData.height);

			mTexImage = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA8, imageData.width, imageData.height), imageData.data);
			TImageView<Color4ub> imageView((Color4ub*)imageData.data, imageData.width, imageData.height);
			mInput.resize(imageData.width * imageData.height);
			GrayScale(imageView, TImageView<NNScalar>(mInput.data(), imageData.width, imageData.height));
			mTexInput = RHICreateTexture2D(TextureDesc::Type2D(ETexture::R32F, imageData.width, imageData.height), mInput.data());

			layer.init(IntVector3(imageData.width, imageData.height, 1), 3, 2);
			layer.biasOffset = 0;
			layer.weightOffset = 2;

			mTarget.resize(layer.getOutputLength() + layer.getOutputLength() / 2, -10000);
			mOutput.resize(layer.getOutputLength() + layer.getOutputLength() / 2, -10000);
			mLossGrads.resize(layer.getOutputLength() + layer.getOutputLength() / 2);


			NNScalar* pInput;
			pInput = forwardPass(mSobelParams.data(), mTarget.data());
			mTexTarget = RHICreateTexture2D(TextureDesc::Type2D(ETexture::R32F, layer.dataSize[0], layer.dataSize[1]), pInput);

			mParameters.resize(layer.getParameterLength());
			mDeltaParameters.resize(layer.getParameterLength());


			int len = layer.dataSize[0] * layer.dataSize[1];
			for (auto& v : mParameters)
			{
				v = RandFloat(-1, 1);
			}

			pInput = forwardPass(mParameters.data(), mOutput.data());
			mTexOutput = RHICreateTexture2D(TextureDesc::Type2D(ETexture::R32F, layer.dataSize[0], layer.dataSize[1]), pInput);

			mOptimizer.init(mParameters.size());
			return true;
		}

		NNScalar* forwardPass(NNScalar const* parameters, NNScalar* outputs)
		{
			NNScalar* pInput = FNNAlgo::Forward(layer, parameters, mInput.data(), outputs);
			outputs += layer.getPassOutputLength();

			int len = layer.dataSize[0] * layer.dataSize[1];
			for (int i = 0; i < len; ++i)
			{
				NNScalar x = pInput[i];
				NNScalar y = pInput[i + len];
				outputs[i] = Math::Sqrt(x*x + y*y);
			}
			return outputs;
		}

		TArray<NNScalar> mOutput;
		TArray<NNScalar> mLossGrads;
		TArray<NNScalar> mDeltaParameters;
		NNScalar mLoss;
		AdamOptimizer mOptimizer;

		using LossFunc = FRMSELoss;
		void trainStep()
		{
			mLoss = 0;

			NNScalar* pLossGrads = mLossGrads.data() + mLossGrads.size();
			NNScalar* pTarget = mTarget.data() + mTarget.size();
			NNScalar* pOutput = mOutput.data() + mOutput.size();

			int len = layer.dataSize[0] * layer.dataSize[1];
			pLossGrads -= len;
			pTarget -= len;
			pOutput -= len;
			for (int i = 0; i < len; ++i)
			{
				pLossGrads[i] = LossFunc::CalcDevivative(pOutput[i], pTarget[i]) / len;
				mLoss += LossFunc::Calc(pOutput[i], pTarget[i]);
			}
			mLoss /= mOutput.size();

			NNScalar* pLossGradsInput = pLossGrads;

			pLossGrads -= 2 * len;
			NNScalar* pInput = pOutput - 2 * len;
			for (int i = 0; i < len; ++i)
			{
				pLossGrads[i] = 2 * pInput[i] / ( pOutput[i] + 1e-6) * pLossGradsInput[i];
				pLossGrads[i + len] = 2 * pInput[i+len] / (pOutput[i] + 1e-6) * pLossGradsInput[i];
			}
	
			FNNMath::Fill(mDeltaParameters.size(), mDeltaParameters.data(), 0);

			FNNAlgo::BackwardWeight(layer, mInput.data(), mOutput.data(), pLossGrads, mDeltaParameters.data());
			FNNMath::ClipNormalize(mDeltaParameters.size(), mDeltaParameters.data(), 1.0f);

			mOptimizer.update(mParameters, mDeltaParameters, 1e-3);

			pInput = forwardPass(mParameters.data(), mOutput.data());
			RHIUpdateTexture(*mTexOutput, 0, 0, layer.dataSize[0], layer.dataSize[1], pInput);
		}



		virtual void onEnd()
		{
			BaseClass::onEnd();
		}

		bool bRunTrain = true;
		float trainTimeRate = 30;
		float accTrainCount = 0.0;
		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);

			if (bRunTrain)
			{
				accTrainCount += trainTimeRate * deltaTime;
				while (accTrainCount > 0)
				{
					trainStep();
					accTrainCount -= 1;
				}
			}
			else
			{
				accTrainCount = 0;
			}
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

			auto DrawGrayTexture = [&g](RHITexture2D& texture, Vector2 const& pos, Vector2 const& size)
			{
				g.drawCustomFunc([&texture, pos, size](RHICommandList& commandList, Matrix4 const& baseTransform, RenderBatchedElement& element)
				{
					RHISetBlendState(commandList, StaticTranslucentBlendState::GetRHI());
					DrawUtility::DrawTexture(commandList, baseTransform, texture, TStaticSamplerState<>::GetRHI(), pos, size, &LinearColor(1,0,0,0));
				});
			};

			g.beginRender();
			RenderUtility::SetBrush(g, EColor::White);
			g.drawTexture(*mTexImage, Vector2(200, 50), Vector2(200, 200));
			DrawGrayTexture(*mTexInput, Vector2(450, 50), Vector2(200, 200));
			DrawGrayTexture(*mTexTarget, Vector2(100, 300), Vector2(400, 400));
			DrawGrayTexture(*mTexOutput, Vector2(550, 300), Vector2(400, 400));


			g.drawTextF(Vector2(10, 10), "Loss = %g", mLoss);
			g.endRender();
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{

			if (msg.onLeftDown())
			{
			}
			else if (msg.onLeftUp())
			{

			}
			else if (msg.isLeftDown() && msg.onMoving())
			{
			}
			return BaseClass::onMouse(msg);
		}

		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.screenWidth = 1080;
			systemConfigs.screenHeight = 768;
		}

	};

	REGISTER_STAGE_ENTRY("Sobel Train", TestStage, EExecGroup::Dev, "AI");
}