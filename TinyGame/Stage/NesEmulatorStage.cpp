#include "TestStageHeader.h"
#include "NesEmulator.h"

#include "InputManager.h"
#include "Widget/WidgetUtility.h"



class SimpleDevice : public NesSpace::IIODevice
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
		p1.left = InputManager::Get().isKeyDown(EKeyCode::D);
		p1.right = InputManager::Get().isKeyDown(EKeyCode::A);
		p1.up = InputManager::Get().isKeyDown(EKeyCode::W);
		p1.down = InputManager::Get().isKeyDown(EKeyCode::S);
		p1.a = InputManager::Get().isKeyDown(EKeyCode::K);
		p1.b = InputManager::Get().isKeyDown(EKeyCode::L);
		p1.start = InputManager::Get().isKeyDown(EKeyCode::N);
		p1.select = InputManager::Get().isKeyDown(EKeyCode::B);

		input[0] = p1.value;
		input[1] = 0;
		//input[1] = buttonState.value;
	}

};


#include "NesEmu/Nes.h"
#include "NesEmu/Renderer.h"
#include "NesEmu/Base.h"
#include "NesEmu/Input.h"

class NesEmuRenderer : public Renderer
{

public:
	virtual void SetWindowTitle(const char* title) override
	{
		
	}


	virtual void Create(size_t screenWidth, size_t screenHeight) override
	{
		Graphics2D& g = ::Global::GetGraphics2D();
		for (int i = 0; i < 2; ++i)
		{
			if (!mBuffers[i].create(g.getRenderDC(), screenHeight, screenHeight, (void**)&mpBufferData[i]))
			{

			}
		}
	}


	virtual void Destroy() override
	{


	}


	virtual void Clear(const Color4& color = Color4::Black()) override
	{
		BitmapDC& context = mBuffers[mIndexCurBuffer].getContext();
		HBRUSH hBrush = ::CreateSolidBrush(RGB(color.R(), color.G(), color.B()));
		::FillRect(context.getHandle(), NULL, hBrush);
		::DeleteObject(hBrush);
	}


	virtual void DrawPixel(int32 x, int32 y, const Color4& color) override
	{
		BitmapDC& context = mBuffers[mIndexCurBuffer].getContext();
		if (0 <= x && x < context.getWidth() && 0 <= y && y < context.getHeight())
		{
			int index = context.getWidth() * (context.getHeight() - y - 1) + x;
			mpBufferData[mIndexCurBuffer][index] = color.argb;
		}

	}

	virtual void Present() override
	{
		mIndexCurBuffer = 1 - mIndexCurBuffer;
	}

	void render(Graphics2D& g)
	{
		GdiTexture& texture = mBuffers[1 - mIndexCurBuffer];
		g.drawTexture(texture, Vec2i(100, 100), 3 * texture.getSize() / 2);
	}


	GdiTexture mBuffers[2];
	uint32*    mpBufferData[2];
	int        mIndexCurBuffer = 0;

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

	NesSpace::IMechine* mMechine = nullptr;
	

	NesEmuRenderer mRenderer;
	std::shared_ptr<Nes> mNes;

	bool bUseNewEmu = false;
	virtual bool onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		if( !mDevice.init(::Global::GetGraphics2D()) )
			return false;

		mMechine = NesSpace::IMechine::Create();
		mMechine->setIODevice(mDevice);
		char const* path = "Nes/SMB3_JP.nes";
		if( !mMechine->loadRom(path) )
			return false;

		::Global::GUI().cleanupWidget();

		DevFrame* frame = WidgetUtility::CreateDevFrame();
		frame->addButton(UI_LOAD_ROM, "Load Rom");

		mNes = std::make_shared<Nes>();
		mNes->Initialize(mRenderer);

		mNes->LoadRom("Nes/SMB3_JP.nes");
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
		if (bUseNewEmu)
		{
			mNes->Reset();
		}
		else
		{
			mMechine->reset();
		}	
	}
	void onUpdate(GameTimeSpan deltaTime) override
	{
		BaseClass::onUpdate(deltaTime);

		int numFrame = long(deltaTime) / gDefaultTickTime;
		for( int i = 0; i < numFrame; ++i)
		{
			if (bUseNewEmu)
			{
				bool const bPaused = false;
				Input::Update();
				//Debugger::Update();
				mNes->ExecuteFrame(bPaused);
			}
			else
			{
				int cycle = Math::FloorToInt(178977.25 / gDefaultTickTime);
				mMechine->run(cycle);
			}
		}
	}

	void onRender(float dFrame)
	{
		Graphics2D& g = Global::GetGraphics2D();
		if (bUseNewEmu)
		{
			mRenderer.render(g);
		}
		else
		{
			mDevice.render(g);
		}

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

REGISTER_STAGE_ENTRY("Nes Emulator Test", NesEmulatorTestStage, EExecGroup::GraphicsTest);


