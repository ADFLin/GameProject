#pragma once
#ifndef FluidSimStage_H_94B85E62_A880_4677_B764_39C27A8D7B77
#define FluidSimStage_H_94B85E62_A880_4677_B764_39C27A8D7B77

#include "Stage/TestStageHeader.h"

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

		bool onInit() override
		{
			if( !BaseClass::onInit() )
				return false;
			::Global::GUI().cleanupWidget();
			restart();
			return true;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
		}

		void onRender(float dFrame) override
		{
			Graphics2D& g = Global::GetGraphics2D();
		}

		void restart() {}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}
		MsgReply onKey(KeyMsg const& msg) override
		{
			if( msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				}
			}
			return BaseClass::onKey(msg);
		}
	protected:
	};







}



#endif // FluidSimStage_H_94B85E62_A880_4677_B764_39C27A8D7B77

