#ifndef TGameUI_h__
#define TGameUI_h__

#include "CUICommon.h"
#include "CUISystem.h"

#include "Thinkable.h"

class CSceneLevel;
class CHero;

class CVitalStateUI;
class CEquipFrame;
class CFastPlayBar;
class CItemBagFrame;
class CSkillFrame;
class CMiniMapUI;
class CChestPanel;
class CBloodBar;
class CTextButton;
class CBuffBarUI;
class CTalkWidget;

struct TEvent;

class CGameUI : public Thinkable
{
public:
	CGameUI( CHero* player );
	~CGameUI();

	CVitalStateUI* getVitalStateUI(){ return vitalStateUI; }
	CFastPlayBar*  getFastPlayBar(){ return fastPlayBar; }


	enum
	{
		SB_EQUIP   ,
		SB_ITEMBAG ,
		SB_SKILL   ,
	};
	CTextButton*   showButton[3];

	void resizeScreenArea( int w , int h );
	void changeLevel( CSceneLevel* level );
	void showFrameEvent( TEvent const& event );
	void update( long time )
	{
		Thinkable::update( time );
	}

	void onHotKey( TEvent const& event );


	CVitalStateUI* vitalStateUI;
	CEquipFrame*   equipPanel;
	CFastPlayBar*  fastPlayBar;
	CItemBagFrame* itemBagPanel;
	CSkillFrame*   skillPanel;

	CMiniMapUI*    miniMap;
	CTalkWidget*   talkUI;
	CBuffBarUI*    buffBarUI;
	CChestPanel*   chestPanel;
};

class CBuffBarUI : public CPanelUI
{
	typedef CPanelUI BaseClass;
public:
	static Vec2i Size;

	CBuffBarUI( Vec2i const& pos , CHero* player , Thinkable* thinkable );
	void onStartBuff( TEvent const& event );
	void onCanelSKill( TEvent const& event );
	void onUpdateUI();

	
	float bufTime;
	TPtrHolder< CBloodBar > m_progessBar;
	CHero* m_player;
};


class CTalkWidget : public CPanelUI
{
	typedef CPanelUI BaseClass;
public:
	CTalkWidget( Vec2i const pos );
	static CFont  sTextFont;
	static Vec2i Size;
	static int const MaxLineNum = 3;
	static int const LineOffset = 30;
	static int const LineCharNum = 40;
	static int const ActorPicSize = 120;

	virtual void onRender();
	void onEvent( TEvent const& event );
	void receiveTalkStr( TEvent const& event );

	char m_showTalkStr[ MaxLineNum * 3 ][ LineCharNum * 2 + 1 ];
	int  m_numLine;
};

#endif // TGameUI_h__