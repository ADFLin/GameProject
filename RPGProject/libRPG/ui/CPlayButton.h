#ifndef TPlayButton_h__
#define TPlayButton_h__

#include "CUICommon.h"


class VertexBuffer;
class CActor;


enum CBType
{
	CBT_UNKNOWN   ,
	CBT_ITEMBAG_PANEL  ,
	CBT_SKILL_PANEL ,
	CBT_EQUIP_PANEL ,
	CBT_FAST_PLAY_BAR ,
	CBT_CHEST_PANEL ,
	CBT_STORE_PANEL ,
};

class TItemBase;
class CToolTipUI;
void SetItemHelpTip(CToolTipUI& helpTip , TItemBase* item );

class CDMask
{
public:
	CDMask( Vec2i const& size , CFScene* scene );
	~CDMask();

	void restore( float time );
	void setState( float time , float chCDTime );
	void update();
	void restoreMaskVertex();
	
	void setConnectEvent( CActor* actor , char const* name );
	void startCD( TEvent const& event );
	static int createMask( CFObject* obj , Material* mat );
	
	char const* mName;
	Vec2i size;
	float  dx;
	float  dy;
	float  mTotalTime;
	float  reTime;
	int    mIdxProcess;
	bool   needShow;
	CFObject*  spr;
private:
	int    mShapeID;
	CFly::VertexBuffer* mGoemBuf;
	unsigned      mGoemOffset;
};

class CPlayButton : public CButtonUI     
{
	typedef CButtonUI BassClass;
public:
	static Vec2i const Size;
	CPlayButton( CBType type ,  Vec2i const& pos , Vec2i const& size , CWidget* parent );
	static Sprite*  sDragIcon;

	virtual void onInputItem( CPlayButton* button ){}

	void play()   {  m_playInfo.doPlay(); }
	void onClick(){ BassClass::onClick(); play(); }
	MsgReply onMouseMsg( MouseMsg const& msg );
	void onMouse( bool beIn );

	void onShow( bool beS );

	void onUpdateUI()
	{
		m_mask.update();

	}

	void checkColdDown();


	void startCD()
	{

	}

	virtual void onSetHelpTip( CToolTipUI& helpTip );

	enum PlayType
	{
		PT_NULL  ,
		PT_SKILL ,
		PT_ITEM  ,
		PT_EQUIP ,
	};

	static PlayType getPlayType( char const* name );

	void setFunction( CActor* actor , char const* name );
	//void setColor( Vec3D const& color )
	//{
	//	m_spr.SetRectColor( (float*) &color );
	//}

	char const* getPlayName(){ return m_playInfo.name; }

	int setupPlayIcon( Sprite* spr );
	struct PlayInfo
	{
		PlayInfo()
		{
			type   = PT_NULL;
			caster = NULL;
			name   = NULL;
		}
		PlayType    type;
		CActor*  caster;
		char const* name;
		void doPlay();
	};

	unsigned    mIdSprRect;

	CDMask      m_mask;
	PlayInfo    m_playInfo;
	CBType      m_cbType;
};


#endif // TPlayButton_h__