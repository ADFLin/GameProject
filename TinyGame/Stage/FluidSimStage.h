#pragma once
#ifndef FluidSimStage_H_94B85E62_A880_4677_B764_39C27A8D7B77
#define FluidSimStage_H_94B85E62_A880_4677_B764_39C27A8D7B77

#include "TestStageHeader.h"

#include "Phy2D/Base.h"

namespace Phy2D
{
	//d(m)/dt + div(mv ) = 0
	//d(mv)/dt + div( v x (mv) ) + div(P) = 0
	//d(E)/dt + div(v ( E + P ) ) = 0
	struct FluildGridData
	{
		Vector2 Loc;
		float P;
		float T;
		Vector2 vel;
	};
	class FluidSimStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		FluidSimStage() {}

		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;
			::Global::GUI().cleanupWidget();
			restart();
			return true;
		}

		virtual void onEnd()
		{
			BaseClass::onEnd();
		}

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

		void restart() {}
		void tick() {}
		void updateFrame(int frame) {}
		bool onMouse(MouseMsg const& msg)
		{
			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}
		bool onKey(unsigned key, bool isDown)
		{
			if( !isDown )
				return false;
			switch( key )
			{
			case Keyboard::eR: restart(); break;
			}
			return false;
		}
	protected:
	};







}



#endif // FluidSimStage_H_94B85E62_A880_4677_B764_39C27A8D7B77

