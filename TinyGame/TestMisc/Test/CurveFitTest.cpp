
#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "AI/NeuralNetwork.h"

#include "RHI/RHICommand.h"
#include "GameGlobal.h"
#include "RHI/RHIGraphics2D.h"
#include "ProfileSystem.h"
#include "Async/AsyncWork.h"

#include <random>

float TestFunc(float x)
{
#if 1
	return x * x * x - x * x - exp(2 * Math::PI * x);
#else
	if (x < -0.5)
		return -1;
	if (x < 0)
		return  1;

	if (x < 0.5)
		return -1;
	return  1;
#endif
}

class CurveFitTestStage : public StageBase
	                    , public IGameRenderSetup
{
	typedef StageBase BaseClass;
public:
	CurveFitTestStage() {}


	FCNNLayout      mLayout;
	FCNeuralNetwork mFNN;
	TArray< NNScalar > mParameters;

	struct TrainData
	{
		TArray< NNScalar > signals;
		TArray< NNScalar > netInputs;
		TArray< NNScalar > lossGrads;
		TArray< NNScalar > deltaParameters;
		TArray< NNScalar > batchNormParameters;
		NNScalar loss;

		void init(FCNNLayout& layout, int batchSize = 1)
		{
			batchSize = 1;
			signals.resize(batchSize * layout.getSignalNum());
			netInputs.resize(batchSize * (layout.getHiddenNodeNum() + layout.getOutputNum()));
			lossGrads.resize(batchSize * (layout.getHiddenNodeNum() + layout.getOutputNum()));
			
			deltaParameters.resize(layout.getParameterNum());
		}

		void reset()
		{
			std::fill_n(deltaParameters.data(), deltaParameters.size(), 0);
			loss = 0;
		}
	};


	TArray< std::unique_ptr< TrainData > > mThreadTrainDatas;

	bool bUseParallelComputing = false;
	bool bAutoTrain = true;
	//bool bShowLog = true;

	QueueThreadPool mPool;

	
	virtual bool onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		::Global::GUI().cleanupWidget();

		int size = 24;
		uint32 topology[] = { 1, size, size, size, size, 1 };
		mLayout.init(topology, ARRAY_SIZE(topology));
		mLayout.setHiddenLayerFunction<ENNFunc::ReLU>();
		mLayout.getLayer(mLayout.getHiddenLayerNum()).setFuncionT<ENNFunc::Linear>();

		mFNN.init(mLayout);
		mParameters.resize(mLayout.getParameterNum());
		mFNN.setParamsters(mParameters);
		mRMSPropSqare.resize(mLayout.getParameterNum(), 0);

		int workerNum = 10;
		for (int i = 0; i < workerNum; ++i)
		{
			mThreadTrainDatas.push_back(std::make_unique<TrainData>());
			mThreadTrainDatas.back()->init(mLayout , i == 0 ? mBatchSize : 1);
		}
		mPool.init(workerNum);

		DevFrame* frame = WidgetUtility::CreateDevFrame();
		frame->addCheckBox("bAutoTrain", bAutoTrain);
		frame->addCheckBox("bUseParallel", bUseParallelComputing);

		restart();
		generateFuncCurve();
		return true;
	}
	static NNScalar RandFloat(NNScalar min, NNScalar max)
	{
		return min + (max - min) * NNScalar(::rand()) / RAND_MAX;
	}
	static void Randomize(TArray< NNScalar >& v)
	{
		for( NNScalar& x : v )
		{
			x = 0.5 * RandFloat(-1, 1);
		}
	}

	virtual void onEnd()
	{
		BaseClass::onEnd();
	}

	void restart() 
	{
		Randomize(mParameters);
		importSample();
	}


	struct SampleData
	{
		NNScalar input;
		NNScalar label;
	};


	TArray<SampleData> mSamples;
	TArray<SampleData> mTestSamples;

	TArray<SampleData*> mOrderedSample;
	void importSample()
	{
		int sampleNum = 50000;
		mSamples.resize(sampleNum);
		mTestSamples.resize(10000);

		NNScalar delta = 2 / float(sampleNum);
		NNScalar start = -1;

		for (auto& sample : mSamples)
		{
			sample.input = start;
			sample.label = TestFunc(sample.input);
			start += delta;
		}

		NNScalar startTest = -1 + 0.5 * delta;
		for (auto& sample : mTestSamples)
		{
			sample.input = startTest;
			sample.label = TestFunc(sample.input);
			startTest += delta;
		}

		mOrderedSample.resize(sampleNum);
		for (int i = 0; i < sampleNum; ++i)
		{
			mOrderedSample[i] = &mSamples[i];
		}
	}


	//RMSProp : thi[t,i] = sqrt( alpha * thi[t-1,i]^2 + ( 1- alpha )( g[t,i] ) );
	//Adam : RMSprop + Momentum
	//Warm Up: 
	
	TArray<NNScalar> mRMSPropSqare;
	TArray<NNScalar> mMomentum;


	class FRMSELoss
	{
	public:
		//MSE £U(r-s)^2
		static NNScalar CalcDevivative(NNScalar prediction, NNScalar label)
		{
			NNScalar delta = label - prediction;
			return -delta;
		}

		static NNScalar Calc(NNScalar prediction, NNScalar label)
		{
			NNScalar delta = label - prediction;
			return  0.5 * delta * delta;;
		}
	};
	
	class FCrossEntropyLoss
	{
		//Cross-Entropy -£Urln(s)


	};
	struct TrainResult
	{
		NNScalar loss;
	};



	using LossFunc = FRMSELoss;

	NNScalar fit(SampleData const& sample, TrainData& trainData)
	{
		NNScalar const* pOutputSignals = trainData.signals.data() + (mLayout.getInputNum() + mLayout.getHiddenNodeNum());

		trainData.signals[0] = sample.input;
		mFNN.calcForwardPass(trainData.signals.data(), trainData.signals.data() + 1, trainData.netInputs.data());

		NNScalar delta = sample.label - pOutputSignals[0];

		NNScalar lossDerivatives = LossFunc::CalcDevivative(pOutputSignals[0], sample.label);
		NNScalar loss = LossFunc::Calc(pOutputSignals[0], sample.label);

		mFNN.calcBackwardPass(&lossDerivatives, trainData.signals.data(), trainData.netInputs.data(), trainData.lossGrads.data(), trainData.deltaParameters.data());
		return loss;
	}

	TrainResult trainStep(TArrayView< SampleData* > const& samples, NNScalar learnRate)
	{
		NNScalar loss = 0.0;
		if (bUseParallelComputing)
		{
			int numWorker = mPool.getAllThreadNum();
			int maxWorkerSampleNum = (samples.size() + numWorker - 1) / numWorker;
			int numSampleCheck = 0;
			for (int i = 0; i < numWorker; ++i)
			{
				int indexSampleStart = i * maxWorkerSampleNum;
				int numSample = (i == numWorker - 1) ?  samples.size() - indexSampleStart : maxWorkerSampleNum;
				TrainData& trainData = *mThreadTrainDatas[i];
				mPool.addFunctionWork(
					[this, &samples, indexSampleStart, numSample, &trainData]()
					{
						trainData.reset();
						for (int i = 0; i < numSample; ++i)
						{
							SampleData* sample = samples[indexSampleStart + i];
							trainData.loss += fit(*sample, trainData);
						}
					}
				);
				numSampleCheck += numSample;
			}

			CHECK(numSampleCheck == samples.size());
			mPool.waitAllWorkComplete();
			TrainData& masterTrainData = *mThreadTrainDatas[0];
			for (int i = 1; i < numWorker; ++i)
			{
				TrainData& trainData = *mThreadTrainDatas[i];

				masterTrainData.loss += trainData.loss;
				FNNCalc::VectorAdd(
					masterTrainData.deltaParameters.size(),
					masterTrainData.deltaParameters.data(),
					trainData.deltaParameters.data());
			}
			
		}
		else
		{
			TrainData& trainData = *mThreadTrainDatas[0];
			trainData.reset();
			for (auto const& sample : samples)
			{
				trainData.loss += fit(*sample, trainData);
			}
		}

		TrainData& mainTrainData = *mThreadTrainDatas[0];
		{
			for (int i = 0; i < mParameters.size(); ++i)
			{
				NNScalar g = mainTrainData.deltaParameters[i] / NNScalar(samples.size());
				NNScalar alpha = 0.9;
				NNScalar thiSquare = /*mRMSPRrop[i] == 0 ? g : */alpha * mRMSPropSqare[i] + (1 - alpha) * Math::Square(g);
				mParameters[i] -= learnRate / Math::Sqrt(thiSquare + 1e-6) * g;
				mRMSPropSqare[i] = thiSquare;
			}
		}

		TrainResult trainResult;
		trainResult.loss = mainTrainData.loss;
		return trainResult;
	}

	int mBatchSize = 1000;
	int mEpoch = 0;
	TArray< Vector2 > mLossPoints;
	TArray< Vector2 > mTestLossPoints;
	void Train()
	{
		TIME_SCOPE("Train");



		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(mOrderedSample.begin(), mOrderedSample.end(), g);

		int iterCount = ( mSamples.size() + mBatchSize - 1 ) / mBatchSize;
		NNScalar learnRate = 0.001;
		NNScalar loss = 0;
		int numSampleCheck = 0;
		for (int iter = 0; iter < iterCount; ++iter)
		{
			int offset = mBatchSize * iter;
			int numSampleData = (iter == iterCount - 1 ) ? mOrderedSample.size() - offset : mBatchSize;
			TArrayView< SampleData* > samples = TArrayView< SampleData* >(mOrderedSample.data() + offset, numSampleData);
			TrainResult trainResult = trainStep(samples, learnRate);

			loss += trainResult.loss;
			numSampleCheck += numSampleData;
		}
		CHECK(numSampleCheck == mSamples.size());

		generateNNCurve();
		mLossPoints.emplace_back(mEpoch, log10(loss / mSamples.size()));
		++mEpoch;

		NNScalar lossTest = 0.0;
		for (auto const& sample : mTestSamples)
		{
			NNScalar output;
			mFNN.calcForwardFeedback(&sample.input, &output);
			lossTest += LossFunc::Calc(output, sample.label);
		}
		lossTest /= mTestSamples.size();
		mTestLossPoints.emplace_back(mEpoch, log10(lossTest));
	}


	void tick() 
	{
		if (bAutoTrain)
		{
			Train();
		}
	}

	void updateFrame(int frame) {}

	virtual void onUpdate(long time)
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		for( int i = 0; i < frame; ++i )
			tick();

		updateFrame(frame);
	}


	TArray< Vector2 > mFuncCurvePoints;
	TArray< Vector2 > mNNCurvePoints;

	void generateFuncCurve()
	{
		float min = -1;
		float max = 1;
		int num = 50;
		mFuncCurvePoints.resize(num);
		for (int i = 0; i < num; ++i)
		{
			Vector2& pt = mFuncCurvePoints[i];
			pt.x = min + (max - min) * i / float(num - 1);
			pt.y = TestFunc(pt.x);
		}
	}

	void generateNNCurve()
	{
		float min = -1;
		float max = 1;
		int num = 50;
		mNNCurvePoints.resize(num);
		for (int i = 0; i < num; ++i)
		{
			NNScalar inputs[1];
			inputs[0] = min + (max - min) * i / float(num - 1);
			NNScalar outputs[1];
			mFNN.calcForwardFeedback(inputs, outputs);
			Vector2& pt = mNNCurvePoints[i];
			pt.x = inputs[0];
			pt.y = outputs[0];
		}
	}

	void onRender(float dFrame)
	{
		using namespace Render;

		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		RHICommandList& commandList = g.getCommandList();

		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 1), 1);

		struct Diagram
		{

			Vector2 min = Vector2(-2, -2);
			Vector2 max = Vector2( 2, 2);
			Vector2 border = Vector2(0.1, 0.1);

			void setup(RHICommandList& commandList, Vector2 renderPos, Vector2 renderSize)
			{
				Vec2i screenSize = ::Global::GetScreenSize();
				RHISetViewport(commandList, renderPos.x, screenSize.y - (renderPos.y + renderSize.y), renderSize.x, renderSize.y);
				Matrix4 matProj = AdjProjectionMatrixForRHI(OrthoMatrix(min.x - border.x, max.x + border.x, min.y - border.y, max.y + border.y, -1, 1));
				RHISetFixedShaderPipelineState(commandList, matProj);
			}

			void drawGird(RHICommandList& commandList, Vector2 basePoint, Vector2 size)
			{
				static TArray<Vector2> buffer;
				buffer.clear();
				float offset = 0.2;

				float v;
				v = basePoint.x;
				while (v >= min.x )
				{
					buffer.emplace_back(v, min.y);
					buffer.emplace_back(v, max.y);
					v -= size.x;
				}

				v = basePoint.x;
				while (v <= max.x)
				{
					buffer.emplace_back(v, min.y);
					buffer.emplace_back(v, max.y);
					v += size.x;
				}

				v = basePoint.y;
				while (v >= min.y)
				{
					buffer.emplace_back(min.x, v);
					buffer.emplace_back(max.x, v);
					v -= size.y;
				}

				v = basePoint.y;
				while (v <= max.y)
				{
					buffer.emplace_back(min.x, v);
					buffer.emplace_back(max.x, v);
					v += size.y;
				}

				if (!buffer.empty())
				{
					RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA >::GetRHI());
					TRenderRT< RTVF_XY >::Draw(commandList, EPrimitive::LineList, &buffer[0], buffer.size(), LinearColor(0, 0, 1));
				}

				{
					Vector2 const lines[] =
					{
						Vector2(min.x,min.y), Vector2(min.x,max.y),
						Vector2(max.x,min.y), Vector2(max.x,max.y),
						Vector2(min.x,min.y) , Vector2(max.x,min.y),
						Vector2(min.x,max.y) , Vector2(max.x,max.y),
					};
					RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
					TRenderRT< RTVF_XY >::Draw(commandList, EPrimitive::LineList, &lines[0], ARRAY_SIZE(lines), LinearColor(1, 0, 0));
				}

			}

			void drawCurve(RHICommandList& commandList, TArray< Vector2 > const& points, LinearColor const& color)
			{
				TRenderRT< RTVF_XY >::Draw(commandList, EPrimitive::LineStrip, points.data(), points.size(), color);
			}

		};

		Diagram diagram;

		diagram.setup(commandList, Vector2(100, 100), Vector2(400, 400));
		diagram.drawGird(commandList, Vector2(0,0) , Vector2(0.2,0.2));

		RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::One, EBlend::One, EBlend::Add >::GetRHI());
		diagram.drawCurve(commandList, mFuncCurvePoints, LinearColor(1, 0, 0, 1));
		diagram.drawCurve(commandList, mNNCurvePoints, LinearColor(0, 1, 1, 1));

		diagram.min.y = -6;
		diagram.max.y = 0.5;
		diagram.min.x = 0;
		diagram.max.x = mEpoch + 1;
		diagram.setup(commandList, Vector2(550, 100), Vector2(200, 400));
		diagram.drawGird(commandList, Vector2(0, 0), Vector2(100, 1));

		RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::One, EBlend::One, EBlend::Add >::GetRHI());
		diagram.drawCurve(commandList, mLossPoints, LinearColor(1, 1, 0, 1));
		diagram.drawCurve(commandList, mTestLossPoints, LinearColor(0, 1, 1, 1));
		g.beginRender();

		g.endRender();
	}

	MsgReply onMouse(MouseMsg const& msg) override
	{
		return BaseClass::onMouse(msg);
	}

	MsgReply onKey(KeyMsg const& msg) override
	{
		if(msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::Z: 
				{
					Train();

				}
				break;
			}
		}
		return BaseClass::onKey(msg);
	}


	virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
	{
		switch (id)
		{
		default:
			break;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	}

	ERenderSystem getDefaultRenderSystem() override { return ERenderSystem::D3D11; }

protected:
};


REGISTER_STAGE_ENTRY("Curve Fit", CurveFitTestStage, EExecGroup::Test, "AI");