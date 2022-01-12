
#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "AI/NeuralNetwork.h"


float TestFunc(float x)
{
	return sin(Math::PI * x);
}
class FuncFitTestStage : public StageBase
	                   , public IGameRenderSetup
{
	typedef StageBase BaseClass;
public:
	FuncFitTestStage() {}


	FCNNLayout      mLayout;
	FCNeuralNetwork mFNN;
	std::vector< NNScalar > mWeights;
	std::vector< NNScalar > mSingnals;
	std::vector< NNScalar > mNetworkInputs;
	std::vector< NNScalar > mSensivityValues;


	virtual bool onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		::Global::GUI().cleanupWidget();
		uint32 topology[] = { 1 , 4 , 4 , 1 };
		mLayout.init(topology, ARRAY_SIZE(topology));
		mFNN.init(mLayout);
		mWeights.resize(mLayout.getWeightNum());
		mSingnals.resize(mLayout.getInputNum() + mLayout.getHiddenNodeNum() + mLayout.getOutputNum());
		mNetworkInputs.resize(mLayout.getHiddenNodeNum() + mLayout.getOutputNum());
		mSensivityValues.resize(mLayout.getHiddenNodeNum() + mLayout.getOutputNum());
		mFNN.setWeights(mWeights);
		restart();
		return true;
	}
	static NNScalar RandFloat(NNScalar min, NNScalar max)
	{
		return min + (max - min) * NNScalar(::rand()) / RAND_MAX;
	}
	static void Randomize(std::vector< NNScalar >& v)
	{
		for( NNScalar& x : v )
		{
			x = RandFloat(-1, 1);
		}
	}

	virtual void onEnd()
	{
		BaseClass::onEnd();
	}

	void restart() 
	{
		Randomize(mWeights);
	}

	void Step()
	{
		NNScalar input = RandFloat(-1, 1);
		mFNN.calcForwardFeedbackSignal( &input , mSingnals.data() , mNetworkInputs.data());

		// E = sum( 0.5 *( tj - a[l|j] )^2 )
		// dE/da[l|j] = -(tj - a[l|j]) 
		// z[l|j] = sum( w[l|j,k]*a[l-1|k] ) + b[l|j]

		// p[l|j] = f'(z[l|j])*dE/da[l|j]
		// p[l-1|k] = f'(z[l-1|k])*sum( p[l|j] * w[l|j,k] )

		// dE/dw[l|j,k] = a[l-1|k] * p[l|j]
		// dE/db[l|j] = p[l|j]
		// w'[l|j,k] = w[l|j,k] + delta * dE/dw[l|j,k]
		// b'[l|j] = b[l|j] + delta * dE/db[l|j] 
		
#if 0
		NNScale delta = 0.5;
		NNScale* pSensivityValueCur = mSensivityValues.data();
		NNScale* pLayerSensivityValueStart = nullptr;
		for( int idxLayer = mLayout.getHiddenLayerNum() ; idxLayer <= 0 ; --idxLayer )
		{
			pLayerSensivityValueStart = pSensivityValueCur;

			auto const& layer = mLayout.getLayer(idxLayer);
			NNScale* pNetworkInputs = mNetworkInputs.data() + mLayout.getNetworkInputOffset(idxLayer);
			
			for( int idxNode = 0; idxNode < mLayout.getOutputNum(); ++idxNode )
			{
				NNScale z_lj = pNetworkInputs[idxNode];
				NNScale p_lj = layer.funcDif( z_lj );
				if( idxLayer == mLayout.getHiddenLayerNum() )
				{
					p_lj *= -(TestFunc(input) - outputs[idxNode]);
				}
				else
				{
					auto const& prevLayer = mLayout.getLayer(idxLayer + 1);
					NNScale const* pWeight = mWeights.data() + prevLayer.weightOffset + idxNode;
					int weightStride = prevLayer.numNode + 1;
					NNScale* pSensivityValuePrev = pLayerSensivityValueStart - prevLayer.numNode;
					NNScale v = 0;
					for( int i = 0; i < prevLayer.numNode; ++i )
					{
						v += *pWeight * (*pSensivityValuePrev);
						++pSensivityValuePrev;
						pWeight += weightStride;
					}

					p_lj *= v;
				}

				NNScale dEdw = p_lj * 


			}
		}
#endif

	}
	void tick() {}
	void updateFrame(int frame) {}

	virtual void onUpdate(long time)
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		for( int i = 0; i < frame; ++i )
			tick();

		updateFrame(frame);
	}

	void onRender(float dFrame)
	{
		Graphics2D& g = Global::GetGraphics2D();
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
			}
		}
		return BaseClass::onKey(msg);
	}

	virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
	{
		switch( id )
		{
		default:
			break;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	}
protected:
};