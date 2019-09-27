#include "Stage/TestStageHeader.h"

namespace PW
{
	Vec2i BasePos = Vec2i(100, 100);
	int NumberOffset = 50;
	int NumberWidth = 40;

	int const IncAmounts[] = { 7 , 3 , 5 , 4 };
	int const IncIndices[][2] = { { 0,3 } ,{ 0,1 } ,{ 1,2 } ,{ 2,3 } };
	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage() {}

		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;

			::Global::GUI().cleanupWidget();
			for( int i = 0; i < 4; ++i )
			{
				GButton* button = new GButton(UI_ANY, BasePos + Vec2i(NumberOffset * i, 80), Vec2i(NumberWidth, 20), nullptr);
				button->onEvent = [this, i](int event, GWidget* ui) -> bool
				{
					handleButtonClick(i);
					return false;
				};
				button->setTitle(FStringConv::From(IncAmounts[i]));
				::Global::GUI().addWidget(button);
				mPasswords[i] = Global::Random() % 10;
			}

			restart();
			return true;
		}

		virtual void onEnd()
		{
			BaseClass::onEnd();
		}

		void restart() {}
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

			RenderUtility::SetPen(g, EColor::White);
			RenderUtility::SetBrush(g, EColor::Gray);
			g.drawRect(BasePos - Vec2i(10, 10), Vec2i(NumberOffset * 4, 120) + Vec2i(10, 10));
		
			RenderUtility::SetFont(g, FONT_S24);
			for( int i = 0; i < 4; ++i )
			{
				g.drawText(BasePos + Vec2i(NumberOffset * i, 0), Vec2i(NumberWidth, 40), FStringConv::From(mPasswords[i]));
			}
		}

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

		virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch( id )
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}

		void handleButtonClick(int index)
		{
			for( int i = 0; i < 2; ++i )
			{
				int idxPW = IncIndices[index][i];
				mPasswords[idxPW] += IncAmounts[index];
				mPasswords[idxPW] %= 10;
			}
		}
	protected:

		int mPasswords[4];
	};




	REGISTER_STAGE("PW Test", TestStage, EStageGroup::Test);
}