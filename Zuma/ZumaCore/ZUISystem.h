#ifndef ZUISystem_h__
#define ZUISystem_h__

#include "WidgetCommon.h"
#include "ZEventHandler.h"
#include "IRenderSystem.h"

namespace Zuma
{
	class ZUISystem;
	ZUISystem* getUISystem();

	enum  ResID;
	class ZWidget;
	struct ZSetting;

	enum
	{
		UI_ADVENTLIRE ,
		UI_GAUNTLET   ,

		UI_MORE_GAMES ,
		UI_QUIT_GAME  ,

		UI_FULLSCREEN ,
		UI_CUSTOM_CURSORS ,
		UI_3DHD_ACC ,

		UI_PLAY_STAGE ,
		UI_ADV_BUTTON ,
		UI_MENU_BUTTON ,
		UI_MENU_PANEL ,
		UI_OPTIONS_BUTTON ,
		UI_OPTIONS_PANEL ,
		UI_STATIC_TEXT ,

		UI_MAIN_MENU  ,
		UI_SOUND_VOLUME ,
		UI_MUSIC_VOLUME ,
		UI_HELP ,
		UI_DONE ,
	};


	struct ZSetting
	{
		void setDefault()
		{
			soundVolume = 0.6f;
			musicVolume = 0.6f;
			fullScreen  = false;
			customCursor= true;
			hardwareAcc = true;
		}

		float soundVolume;
		float musicVolume;
		bool  fullScreen;
		bool  customCursor;
		bool  hardwareAcc;
	};


	class ZWidget : public WidgetCoreT< ZWidget >
	{
	public:
		ZWidget( Vec2i const& pos , Vec2i const& size , ZWidget* parent )
			:WidgetCoreT< ZWidget >( pos , size , parent ){}
		int    getUIID() const { return m_idUI; }
		ResID  getTexID() const { return m_idTex; }

		ITexture2D* getTexture();

		static Vec2i calcUISize( ResID idTex );
		void   sendEvent( int idEvt , ZWidget* ui );

	public:
		virtual void  onRender(){}
	protected:

		ResID  m_idTex;
		int    m_idUI;
	};

	typedef TWidgetLibrary< ZWidget > GameUI;
	
	class ZDialogButton;

	class ZUISystem : public ZEventHandler
		            , public GameUI::Manager
	{
	public:
		ZUISystem();

		bool            init( IRenderSystem& RDSystem );
		ITexture2D*     getTexture( ResID idTex );
		void            drawUI( ZWidget* ui , int frame = -1);
		void            drawTex( ResID idTex , Vec2D const& pos , int frame = -1);
		ZWidget*        findUI( int idUI , WidgetCoreT<ZWidget>* parent = NULL );
		IRenderSystem&  getRenderSystem(){ return *mRDSystem; }

	protected:
		bool     onEvent( int evtID , ZEventData const& data , ZEventHandler* sender );
		IRenderSystem* mRDSystem;
	};

	class ZStaticText : public ZWidget
	{
	public:
		ZStaticText( Vec2i const& pos , Vec2i const& size , std::string const& str, ZWidget* parent );

		void  setFontType( ResID type ){ fontType = type;}
		void  setTitle( std::string const& str ){ title = str; }
		void  onRender();
		void  setFontColor( uint8 r ,uint8 g , uint8 b , uint8 a = 255 );

		ColorKey fontColor;
		ResID    fontType;
		std::string  title;
	};


	class ZButton : public GameUI::ButtonT< ZButton >
	{
	public:
		ZButton( int idUI , ResID idTex , Vec2i const& pos , ZWidget* parent );
		virtual void onClick();
		virtual void onChangeState( ButtonState state );
	};


	class ZTexButton : public ZButton
	{
	public:
		ZTexButton( int idUI , ResID idTex , Vec2i const& pos , ZWidget* parent );

		virtual void onRender();
	};

	class ZAdvButton : public ZButton
	{
	public:
		ZAdvButton( ResID idTex , Vec2i const& pos , ZWidget* parent );
		virtual void onRender();
		virtual void onClick();

		static   unsigned selectStageID;

		unsigned stageID;
	};

	class ZDialogButton : public ZButton
	{
	public:
		ZDialogButton( int idUI , float len , Vec2i const& pos , ZWidget* parent );

		Vec2i calcUISize( int len );
		virtual void onRender();

		void  setTitle( std::string const& str ){ title = str; }

		std::string  title;
		static int const frontGap = 11;
		static int const backGap = 2;
		static int const downGap = 9;
		static int const headSize = 35;

	};

	class ZCheckButton : public ZButton
	{
	public:
		ZCheckButton( int idUI , Vec2i const& pos , ZWidget* parent );

		void  setTitle( std::string const& str , unsigned len )
		{  title = str;  titleLen = len;  }
		bool isCheck(){ return checked; }
		void setCheck( bool beC ){ checked = beC; }
		virtual void onClick(){  checked = !checked;  }
		virtual void onRender();


		static int const frontGap = 7;
		static int const backGap  = 5;
		static int const downGap  = 8;

		unsigned titleLen;
		bool checked;
		std::string  title;
	};

	class ZSlider : public GameUI::SliderT< ZSlider >
	{
	public:
		ZSlider( int idUI , Vec2i const& pos , ZWidget* parent );

		virtual void onScrollChange( int value );
		virtual void doRenderTip( TipUI* ui )
		{
			getUISystem()->drawTex( idTexTip , ui->getWorldPos() );
		}
		virtual void doRenderBackground()
		{
			getUISystem()->drawUI( this );
		}
		ResID idTexTip;
	};

	class ZPanel : public GameUI::PanelT< ZPanel >
	{
	public:
		ZPanel( int idUI , Vec2i const& pos ,  Vec2i const& size , ZWidget* parent );

		virtual void onRender();
		virtual bool onMessage( MouseMsg& msg );
	};


	class ZSettingPanel : public ZPanel
	{
	public:
		ZSettingPanel( int idUI , Vec2i const& pos  , ZWidget* parent );
		static Vec2i UISize;

		void  getSystemInfo( ZSetting& info );
		void  setSystemInfo( ZSetting& info );

		ZSlider* musicSlider;
		ZSlider* soundSlider;
		ZCheckButton* FSCheck;
		ZCheckButton* CCCheck;
		ZCheckButton* HWCheck;
	};

	class ZMenuPanel : public ZPanel
	{
	public:
		ZMenuPanel( Vec2i const& pos , ZWidget* parent );
		static Vec2i UISize;
	};

	//class ZMessagePanel : public ZPanel
	//{
	//
	//};

}//namespace Zuma

#endif // ZUISystem_h__
