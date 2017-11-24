#include "ZumaPCH.h"
#include "ZUISystem.h"

#include "ResID.h"
#include "ZEvent.h"
#include "AudioPlayer.h"

namespace Zuma
{
	static ZUISystem* g_uiSystem = NULL;

	ZUISystem* getUISystem(){ return g_uiSystem; }

	ZUISystem::ZUISystem()
	{
		g_uiSystem = this;
	}

	bool ZUISystem::init( IRenderSystem& RDSystem )
	{
		mRDSystem = &RDSystem;
		return true;
	}


	bool ZUISystem::onEvent( int evtID , ZEventData const& data , ZEventHandler* sender )
	{
		return true;
	}

	ZWidget* ZUISystem::findUI( int idUI , WidgetCoreT<ZWidget>* parent )
	{
		if( parent == nullptr )
		{
			for( auto ui = createTopWidgetIterator(); ui; ui )
			{
				if( ui->getUIID() == idUI )
					return &(*ui);
			}
		}
		else
		{
			for( auto ui = parent->createChildrenIterator(); ui; ui )
			{
				if( ui->getUIID() == idUI )
					return &(*ui);
			}

		}
		return NULL;
	}

	void ZUISystem::drawUI( ZWidget* ui , int frame /*= -1*/ )
	{
		Vec2i const& pos = ui->getWorldPos();
		drawTex( ui->getTexID() , pos , frame );
	}

	ITexture2D* ZUISystem::getTexture( ResID idTex )
	{
		return mRDSystem->getTexture( idTex );
	}

	ITexture2D* ZWidget::getTexture()
	{
		return getUISystem()->getTexture( getTexID() );
	}

	void ZWidget::sendEvent( int idEvt , ZWidget* ui )
	{
		getUISystem()->processEvent( idEvt , ui );
	}

	Vec2i ZWidget::calcUISize( ResID idTex )
	{
		ITexture2D* tex = getUISystem()->getTexture( idTex );

		int x = tex->getWidth();
		if ( tex->col != -1 )
			x /= tex->col;

		int y = tex->getHeight();
		if ( tex->row != -1 )
			y /= tex->row;

		return Vec2i( x , y );
	}


	unsigned ZAdvButton::selectStageID = 0;

	ZButton::ZButton( int idUI , ResID idTex , Vec2i const& pos , ZWidget* parent )
		:GameUI::ButtonT< ZButton >( pos , Vec2i(0,0) , parent )
	{
		m_idUI  = idUI;
		m_idTex = idTex;
	}

	void ZButton::onClick()
	{
		sendEvent( EVT_UI_BUTTON_CLICK , this );
	}

	void ZButton::onChangeState( ButtonState state )
	{
		switch ( state )
		{
		case BS_PRESS:
			playSound( SOUND_BUTTON1 );
			break;
		}
	}

	ZTexButton::ZTexButton( int idUI , ResID idTex , Vec2i const& pos , ZWidget* parent )
		:ZButton( idUI , idTex , pos , parent )
	{
		setSize( calcUISize( idTex ) );
	}

	void ZTexButton::onRender()
	{
		int frame;
		switch( getButtonState() )
		{
		case BS_NORMAL:    frame = 0; break;
		case BS_HIGHLIGHT: frame = 1; break;
		case BS_PRESS:     frame = 2; break;
		}

		getUISystem()->drawUI( this , frame );
	}

	ZDialogButton::ZDialogButton( int idUI , float len , Vec2i const& pos , ZWidget* parent )
		:ZButton( idUI , IMAGE_DIALOG_BUTTON  , pos , parent)
	{
		setSize( calcUISize( len ) );
	}

	ZAdvButton::ZAdvButton( ResID idTex , Vec2i const& pos , ZWidget* parent )
		:ZButton( UI_ADV_BUTTON , idTex , pos , parent )
	{
		setSize( calcUISize( idTex ) );
	}

	void ZAdvButton::onRender()
	{
		int frame = 0;
		if ( stageID == selectStageID )
			frame = 3;
		else if ( isEnable() )
		{
			switch ( getButtonState() )
			{
			case BS_NORMAL:    frame = 1; break;
			case BS_PRESS:
			case BS_HIGHLIGHT: frame = 2; break;
			}
		}

		getUISystem()->drawUI( this , frame );
	}

	void ZAdvButton::onClick()
	{
		selectStageID = stageID;
		ZButton::onClick();
	}

	ZCheckButton::ZCheckButton( int idUI , Vec2i const& pos , ZWidget* parent )
		:ZButton( idUI , IMAGE_DIALOG_CHECKBOX , pos , parent )
	{
		Vec2i size = calcUISize( IMAGE_DIALOG_CHECKBOX );

		size.x -= ( frontGap + backGap );
		size.y -= downGap;
		setSize( size );
	}

	ZStaticText::ZStaticText( Vec2i const& pos , Vec2i const& size , std::string const& str, ZWidget* parent )
		:ZWidget( pos , size , parent )
		,title( str ),fontType( FONT_MAIN10 )
	{
		fontColor.value = 0xffffffff;
		enable( false );
		m_idUI  = UI_STATIC_TEXT;
	}

	void ZStaticText::setFontColor( uint8 r ,uint8 g , uint8 b , uint8 a )
	{
		fontColor.r = r;
		fontColor.g = g;
		fontColor.b = b;
		fontColor.a = a;
	}

	ZSlider::ZSlider( int idUI , Vec2i const& pos , ZWidget* parent )
		:GameUI::SliderT< ZSlider >( pos , 1 , calcUISize(IMAGE_SLIDER_THUMB) , true , 0 , 100 , parent )
	{
		m_idUI  = idUI;
		m_idTex = IMAGE_SLIDER_TRACK;

		setSize( calcUISize( IMAGE_SLIDER_TRACK) );
		idTexTip = IMAGE_SLIDER_THUMB;
	}

	void ZSlider::onScrollChange( int value )
	{
		sendEvent( EVT_UI_SLIDER_CHANGE , this );
	}

	ZPanel::ZPanel( int idUI , Vec2i const& pos , Vec2i const& size , ZWidget* parent )
		:GameUI::PanelT< ZPanel >(  pos , Vector2(1,1) , parent )
	{
		m_idUI  = idUI;
		m_idTex = IMAGE_DIALOG_BACK;

		setSize( size );
	}

	bool ZPanel::onMessage( MouseMsg& msg )
	{
		static Vec2i oldPos;
		if ( msg.onLeftDown() )
		{
			oldPos = msg.getPos();
		}
		if ( msg.isLeftDown() && msg.isDraging() )
		{
			Vec2i pos = getPos() + msg.getPos() - oldPos;
			setPos( pos );
			oldPos = msg.getPos();
		}
		return false;
	}

	Vec2i ZSettingPanel::UISize( 400 , 380 );

	ZSettingPanel::ZSettingPanel( int idUI , Vec2i const& pos , ZWidget* parent )
		:ZPanel( idUI , pos , UISize , parent )
	{
		int const sideGap = 35;
		int buttonLen = UISize.x - 2 * sideGap ;

		int const lineSpacing = 40;

		int startPos = 80 - lineSpacing;
		ZStaticText* text;

		text = new ZStaticText(
			Vec2i( 100 , 20 ) ,
			Vec2i( UISize.x - 2 * 100 , 50 ) , "OPTIONS" , this );
		text->setFontType( FONT_TITLE );

		text = new ZStaticText(
			Vec2i( sideGap , startPos += lineSpacing ) ,
			Vec2i( 130 , lineSpacing - 10 ) , "Music Volume" , this	);
		text->setFontColor( 255 , 255 , 0 );
		ZSlider* slider;
		musicSlider = new ZSlider( UI_MUSIC_VOLUME ,
			Vec2i( 130 + sideGap , startPos ) , this );

		text = new ZStaticText(
			Vec2i( sideGap , startPos += lineSpacing ) ,
			Vec2i( 130 , lineSpacing - 10 ) , "Sound Effect" , this );
		text->setFontColor( 255 , 255 , 0 );
		soundSlider = new ZSlider( UI_SOUND_VOLUME ,
			Vec2i( 130 + sideGap , startPos ) , this );

		ZCheckButton* cButton;
		FSCheck = new ZCheckButton( UI_FULLSCREEN ,
			Vec2i( sideGap , startPos += lineSpacing ) , this );
		FSCheck->setTitle( "Fullscreen" , buttonLen / 2 - 20 );
		CCCheck = new ZCheckButton( UI_CUSTOM_CURSORS ,
			Vec2i( sideGap + buttonLen / 2 , startPos ) , this );
		CCCheck->setTitle( "Custom Cursors" , buttonLen / 2  );
		HWCheck = new ZCheckButton( UI_3DHD_ACC ,
			Vec2i( sideGap , startPos += lineSpacing ) , this );
		HWCheck->setTitle( "3-D Hardware Acceleration" , buttonLen - 20 );

		startPos += 10;
		ZDialogButton* button;
		button = new ZDialogButton( UI_HELP , buttonLen ,
			Vec2i( sideGap , startPos += lineSpacing ) , this );
		button->setTitle( "Help" );
		button = new ZDialogButton( UI_DONE , buttonLen ,
			Vec2i( sideGap , startPos += lineSpacing ) , this );
		button->setTitle( "Done" );
	}

	void ZSettingPanel::setSystemInfo( ZSetting& info )
	{
		FSCheck->setCheck( info.fullScreen );
		CCCheck->setCheck( info.customCursor );
		HWCheck->setCheck( info.hardwareAcc );

		musicSlider->setValue( info.musicVolume * musicSlider->getMaxValue() );
		soundSlider->setValue( info.soundVolume * soundSlider->getMaxValue() );
	}

	void ZSettingPanel::getSystemInfo( ZSetting& info )
	{
		info.fullScreen   = FSCheck->isCheck();
		info.customCursor = CCCheck->isCheck();
		info.hardwareAcc  = HWCheck->isCheck();
		info.musicVolume  = float(musicSlider->getValue()) / musicSlider->getMaxValue();
		info.soundVolume  = float(soundSlider->getValue()) / soundSlider->getMaxValue();
	}

	Vec2i ZMenuPanel::UISize( 300 , 250 );
	ZMenuPanel::ZMenuPanel( Vec2i const& pos , ZWidget* parent )
		:ZPanel( UI_MENU_PANEL , pos , UISize , parent )
	{
		int const sideGap = 30;
		int startPos = 10;
		int buttonLen = UISize.x - 2 * sideGap ;
		ZDialogButton* button;
		button = new ZDialogButton(
			UI_OPTIONS_BUTTON , buttonLen ,
			Vec2i( sideGap , startPos += 50 ) , this );
		button->setTitle( "Options" );

		button = new ZDialogButton(
			UI_MAIN_MENU , buttonLen ,
			Vec2i( sideGap , startPos += 50 ) , this );
		button->setTitle( "Back Main Menu" );


	}

}//namespace Zuma
