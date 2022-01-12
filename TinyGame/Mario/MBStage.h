#include "Mario/MBLevel.h"



namespace Mario
{

	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;


		World  mWorld;
		Player player;

	public:
		TestStage() {}

		virtual bool onInit()
		{
			::Global::GUI().cleanupWidget();

			Block::InitializeMap();

			restart();
			return true;
		}

		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			updateFrame(frame);
		}

		void restart();


		void tick();

		void updateFrame(int frame)
		{

		}

		void onRender(float dFrame);

		void drawTriangle(Graphics2D& g, Vector2 const& p1, Vector2 const& p2, Vector2 const& p3)
		{
			int frameHeight = ::Global::GetScreenSize().y;

			Vec2i v[3] = { p1 , p2 , p3 };
			v[0].y = frameHeight - v[0].y;
			v[1].y = frameHeight - v[1].y;
			v[2].y = frameHeight - v[2].y;
			g.drawPolygon(v, 3);
		}

		void drawRect(Graphics2D& g, Vector2 const& pos, Vector2 const& size)
		{
			int frameHeight = ::Global::GetScreenSize().y;
			Vector2 rPos;
			rPos.x = pos.x;
			rPos.y = frameHeight - pos.y - size.y;
			g.drawRect(rPos, size);
		}


		MsgReply onMouse(MouseMsg const& msg)
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg)
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
	protected:

	};



}
