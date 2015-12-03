#ifndef TUISystem_h__
#define TUISystem_h__

#include "common.h"
#include "TInputMsg.h"
#include "TEntityBase.h"
#include "singleton.hpp"
#include "TVec2D.h"

#define UI_NO_PARENT (void*)0xffffffff

void  MessgeUI( BYTE r, BYTE g , BYTE b , const char* str , ... );

enum UIFlag
{
	UF_STAY_TOP = BIT(1),
};
struct TFont
{
public:
	void init( char const* fontClass , int size , bool beB , bool beI );

	void start();
	void setColor( BYTE r, BYTE g , BYTE b , BYTE a = 255 )
	{
		cr = r ; cg = g; cb = b ; ca = a;  
	}
	int write( int x , int y , char const* format , ... )
		{
	int write( int x , int y , int maxPixel , char const* format , ... );
	void finish(){  shufa.End();  }

	BYTE    cr,cg,cb,ca;
	FnShuFa shufa;
	char    str[128];
	int     width;
	int     height;
};

extern TFont   g_TextFont;
extern int     g_ShufaFontSize;

class MouseMsg;
class TSpriteUI;

class TSprite : public TEntityBase
{
public:
	TSprite( FnSprite& obj );
	~TSprite();

	Vec3D const& getPosition() const { return pos; }
	Vec3D const& getVelocity() const { return v; }

	void  setPosition(Vec3D const& val , bool be3D = false );
	void  setVelocity(Vec3D const& vel ){ v = vel; }
	void  use3DPos( bool be3D ){ is3DPos = be3D; }
	void  updateFrame();
	void  setDestoryTime( float time );
	void  setOpacityChange( float os , float oe , float dt );

protected:
	float    offsetOpacity;
	float    endOpacity;

	bool     is3DPos;
	FnSprite m_obj;
	Vec3D    v;
	Vec3D    pos;

	void changeOpacityThink();
	void destoryThink();

};


class TNumUICreator
{
public:
	TNumUICreator(FnWorld& world);
	~TNumUICreator();

	static int const DigiWidth  = 26;
	static int const DigiHeight = 36;
	void     init( FnWorld& world );
	FnSprite create( int val , int color = 0);
protected:
	void     createNumGeom( FnObject* obj, Vec3D const& pos , int digi , int numColor );

	FnObject   saveMatObj;
	MATERIALid numMatID;
	FnScene    bgScene;
};




class TRect
{
public:
	TRect( TVec2D const& _pos , TVec2D const& _size )
		:min( _pos )
		,max( _pos + _size )
	{

	}

	TVec2D getSize(){ return max - min; }
	TVec2D max;
	TVec2D min;
};

inline
bool testPointInRect( TVec2D const& pos , TRect const& rect )
{
	return  rect.min.x <= pos.x && pos.x <= rect.max.x &&
		    rect.min.y <= pos.y && pos.y <= rect.max.y ;
}


class TUIBase
{
public:
	TUIBase();

	TUIBase(TVec2D const& pos , TVec2D const& size , TUIBase* parent );

	virtual ~TUIBase();

	virtual bool  OnMessage(MouseMsg& msg){ return true; }
	virtual void  OnUpdateUI(){}
	virtual void  OnRender(){}
	virtual void  OnFocus( bool beF ){}
	virtual void  OnShow( bool beS );

	virtual void  OnMouse( bool beIn ){}
	virtual void  OnChangePos( TVec2D const& pos , bool beLocal ){}

	TUIBase*      getChildren( TUIBase* startUi = NULL );
	TUIBase*      getParent(){ return m_parent; }
	TUIBase*      getPrev();
	TUIBase*      getNext(){ return m_next; }

	TVec2D        getWorldPos();
	TVec2D const& getPos() const { return m_rect.min; }
	TVec2D        getSize(){ return m_rect.max - m_rect.min; }
	void          setPos( TVec2D const& pos );

	void          show( bool beS );
	bool          isFocus();
	void          enable( bool beE = true ){ m_enable = beE; }
	bool          isEnable() const { return m_enable; }
	bool          isShow() const { return m_show; }
	void          addChild( TUIBase* ui );

	void          setFlag( unsigned flag ){	m_flag = flag;	}
	unsigned      getFlag(){ return m_flag; }

protected:

	TUIBase* m_parent;
	TUIBase* m_next;
	TUIBase* m_child;

	static int const MaxLevelDepth;
	static int const MaxChildNum;

	int       getLevel();
	int       getLayer();
	int       getChildrenNum(){ return m_numChild; }

	float     computeDepth();

	void      removeChild( TUIBase* ui );
	
	TUIBase*  hitTestChildren( TVec2D const& testPos );

	friend class TUISystem;
	friend class TSpriteUI;

	unsigned m_flag;
	int      m_numChild;
	bool     m_show;
	bool     m_enable;
	TRect    m_rect;
};

class TUISystem : public TEntityBase
	            , public Singleton< TUISystem >
{
public:
	TUISystem();

	void init( FnWorld& world );
	void addUI( TUIBase* ui )
	{
		m_root.addChild( ui );
	}

	bool     OnMessage( MouseMsg& msg );
	FnScene& getBackScene(){ return backScene; }
	FnScene& getFrontScene(){ return frontScene; }

	TUIBase* getRoot(){ return &m_root; }
	TUIBase* hitTest( TVec2D const& testPos );
	void     updateUI();
	void     render( FnViewport& viewport );

	void     destoryUI( TUIBase* ui );
	void     destoryAllUI();

	void     changetScreenSize( int x , int y );

	
	MouseMsg& getLastMouseMsg(){ return m_mouseMsg; }
	TUIBase*  getFocusUI(){ return m_focusUI; }
	static  void   setTextureDir( char const* path );
	static  OBJECTid  createSprite();
	static  void  destorySprite( FnSprite& spr );
	
	SCENEid createNewUIScene();
	void    sortUIRenderIndex();

	TSprite* createScreenMask( Vec3D const& color );

	void    showDamageNum();
	void    showLevelUp();

private:

	void updateUI( TUIBase* ui );
	void render( TUIBase* ui );

	SCENEid sceneID[32];
	int     numRootChlid;

	MouseMsg m_mouseMsg;
	TUIBase* m_focusUI;
	TUIBase* m_MouseUI;
	TUIBase  m_root;

	TNumUICreator* numCreator;
	FnScene  backScene;
	FnScene  frontScene;
};



#endif // TUISystem_h__