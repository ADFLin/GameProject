#include "TestStageHeader.h"
#include "NesEmulator.h"

#include "InputManager.h"
#include "Widget/WidgetUtility.h"



class SimpleDevice : public Nes::IIODevice
{

public:

	int const NesScreenWidth = 256;
	int const NesScreenHeight = 240;
	SimpleDevice()
	{
		mIndexCurBuffer = 0;
	}

	bool init(Graphics2D& g)
	{
		for( int i = 0; i < 2; ++i )
		{
			if( !mBuffers[i].create(g.getRenderDC(), NesScreenWidth, NesScreenHeight, (void**)&mpBufferData[i]) )
			{
				return false;
			}
		}
		return true;
	}

	virtual void setPixel(int x, int y, uint32 value) override
	{
		if ( 0 <= x && x < NesScreenWidth && 0 <= y && y < NesScreenHeight )
		{
			int index = NesScreenWidth * (NesScreenHeight - y - 1) + x;
			mpBufferData[mIndexCurBuffer][index] = value;
		}
	}

	virtual void flush() override
	{
		mIndexCurBuffer = 1 - mIndexCurBuffer;
	}
	void render(Graphics2D& g)
	{
		GdiTexture& texture = mBuffers[1 - mIndexCurBuffer];
		g.drawTexture(texture , Vec2i(100, 100) , 3 * texture.getSize() / 2 );
	}

	GdiTexture mBuffers[2];
	uint32*  mpBufferData[2];
	int      mIndexCurBuffer;

	union ButtonState
	{
		uint32 value;
		struct  
		{
			uint32 left : 1;
			uint32 right : 1;
			uint32 up : 1;
			uint32 down : 1;
			uint32 a : 1;
			uint32 b : 1;
			uint32 select : 1;
			uint32 start : 1;

		};
	};
	
	virtual void scanInput(uint32 input[2]) override
	{
		ButtonState p1;
		p1.value = 0;
		p1.left = InputManager::Get().isKeyDown(Keyboard::eD);
		p1.right = InputManager::Get().isKeyDown(Keyboard::eA);
		p1.up = InputManager::Get().isKeyDown(Keyboard::eW);
		p1.down = InputManager::Get().isKeyDown(Keyboard::eS);
		p1.a = InputManager::Get().isKeyDown(Keyboard::eK);
		p1.b = InputManager::Get().isKeyDown(Keyboard::eL);
		p1.start = InputManager::Get().isKeyDown(Keyboard::eN);
		p1.select = InputManager::Get().isKeyDown(Keyboard::eB);

		input[0] = p1.value;
		input[1] = 0;
		//input[1] = buttonState.value;
	}

};


class NesEmulatorTestStage : public StageBase
{
	typedef StageBase BaseClass;
public:


	enum
	{
		UI_LOAD_ROM = BaseClass::NEXT_UI_ID ,

		UI_ROM_FILE_LIST ,
	};
	NesEmulatorTestStage() 
	{
	
	}

	~NesEmulatorTestStage()
	{

	}

	Nes::IMechine* mMechine = nullptr;
	



	virtual bool onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		if( !mDevice.init(::Global::GetGraphics2D()) )
			return false;

		mMechine = Nes::IMechine::Create();
		mMechine->setIODevice(mDevice);
		char const* path = "Nes/SMB3_JP.nes";
		if( !mMechine->loadRom(path) )
			return false;

		::Global::GUI().cleanupWidget();

		DevFrame* frame = WidgetUtility::CreateDevFrame();
		frame->addButton(UI_LOAD_ROM, "Load Rom");
		restart();
		return true;
	}

	virtual void onEnd()
	{
		if( mMechine )
			mMechine->release();

		BaseClass::onEnd();
	}

	void restart() 
	{
		mMechine->reset();
		
	}
	void tick() 
	{
		int cycle = 178977.25 / gDefaultTickTime;
		mMechine->run(cycle);
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

	void onRender(float dFrame)
	{
		Graphics2D& g = Global::GetGraphics2D();

		mDevice.render(g);
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

	SimpleDevice mDevice;

	virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
	{
		switch( id )
		{
		case UI_LOAD_ROM:
			{
				GFileListCtrl* ctrl = new GFileListCtrl( UI_ROM_FILE_LIST , Vec2i(100 , 100 ) , Vec2i( 400 , 400 ) , nullptr );
				ctrl->mSubFileName = ".nes";
				ctrl->setDir("Nes");
				::Global::GUI().addWidget(ctrl);
			}
			return false;
		case UI_ROM_FILE_LIST:
			if ( event == EVT_LISTCTRL_DCLICK )
			{
				String path = GUI::CastFast<GFileListCtrl>(ui)->getSelectedFilePath();
				mMechine->loadRom(path.c_str());
				mMechine->reset();

				ui->destroy();
			}
			return false;
		}
		return BaseClass::onWidgetEvent(event, id, ui);
	}

};

REGISTER_STAGE("Nes Emulator Test", NesEmulatorTestStage, EStageGroup::GraphicsTest);


