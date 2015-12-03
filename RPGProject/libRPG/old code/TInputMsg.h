#ifndef TInputMsg_h__
#define TInputMsg_h__


#include "common.h"
#include "TVec2D.h"

enum MsgType
{
	MSG_CHAR  ,
	MSG_MOUSE ,
	MSG_KEY   ,
};

enum StageType
{
	GS_MAIN ,
	GS_START_MENU ,
	GS_CONSOLE    ,
	GS_PROFILE    ,

	GS_EMPTY ,
};

class TMessage
{	
public:
	TMessage( MsgType type) : m_type( type ){}
	MsgType getType(){ return m_type; }
private:
	MsgType m_type;
};

class KeyMsg : public TMessage
{
public:
	KeyMsg( char key , bool beP )
		:TMessage( MSG_KEY )
		,m_key( key )
		,isDonw( beP ){}
	char getKey(){ return m_key; }
	bool isDown(){ return isDonw; }
private:
	bool isDonw;
	char m_key; 
};

class CharMsg : public TMessage
{
public:
	CharMsg( char c )
		:TMessage( MSG_CHAR )
		,m_char( c ){}
	char getChar(){ return m_char; }
private:
	char m_char; 
};

enum MouseState
{
	MBS_LEFT         = BIT(0),
	MBS_MIDDLE       = BIT(1),
	MBS_RIGHT        = BIT(2),
	MBS_DRAGING      = BIT(3),
	MBS_DOUBLE_CLICK = BIT(4),
	MBS_DOWN         = BIT(5),
	MBS_MOVING       = BIT(6),
	MBS_WHEEL        = BIT(7),
};

class MouseMsg : public TMessage
{
public:
	MouseMsg()
		: TMessage( MSG_MOUSE )
		, msg(0),state(0)
	{

	}
	MouseMsg( TVec2D const& pos , unsigned m , unsigned s )
		: TMessage( MSG_MOUSE )
		, pos( pos ), msg( m ), state(s )
	{
	}
	MouseMsg( int x , int y , unsigned m , unsigned s )
		: TMessage( MSG_MOUSE )
		,pos(x,y),msg( m ) ,state(s)
	{
	}
	int    x() const { return pos.x; }
	int    y() const { return pos.y; }
	TVec2D const& getPos(){ return pos; }
	void setPos( TVec2D const& p ){ pos = p; }

	bool isDown()       { return ( msg & MBS_DOWN ) != 0; }
	bool isLeftDown()   { return ( state & MBS_LEFT )!= 0; }
	bool isMiddleDown() { return ( state & MBS_MIDDLE )!= 0; }
	bool isRightDown()  { return ( state & MBS_RIGHT ) != 0; }

	bool OnMoving()     { return ( msg & MBS_MOVING ) != 0; }
	bool OnDraging()    { return ( msg & MBS_DRAGING ) != 0; }

	bool OnWheelFront() { return msg ==  MBS_WHEEL ;}
	bool OnWheelBack() { return msg ==  ( MBS_WHEEL | MBS_DOWN );}

	bool OnLeftUp()     { return msg == MBS_LEFT; }
	bool OnMiddleUp()   { return msg == MBS_MIDDLE; }
	bool OnRightUp()    { return msg == MBS_RIGHT; }
	bool OnLeftDClick() { return msg ==( MBS_LEFT | MBS_DOUBLE_CLICK); }
	bool OnRightDClick() { return msg ==( MBS_RIGHT | MBS_DOUBLE_CLICK); }
	bool OnLeftDown()   { return msg ==( MBS_LEFT | MBS_DOWN ); }
	bool OnRightDown()  { return msg ==( MBS_RIGHT | MBS_DOWN ); }
	bool OnMiddleDown() { return msg ==( MBS_MIDDLE | MBS_DOWN ); }

private:
	TVec2D      pos;
	unsigned    msg;
	unsigned    state;
};

#endif // TInputMsg_h__
