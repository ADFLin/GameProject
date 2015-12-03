#ifndef TGameStage_h__
#define TGameStage_h__

#include "common.h"
#include "TInputMsg.h"
#include "TEntityBase.h"

class TLevel;

class TGameStage : public TEntityBase
{
public:
	TGameStage( StageType state )
		:m_state( state ){ setGlobal( true ); }

	StageType getStateType(){ return m_state; }

	virtual ~TGameStage(){}
	virtual void  reset(){}
	virtual void  reader();
	virtual void  clear(){}
	virtual void  update(){}
	virtual bool  OnMessage( unsigned key , bool isDown ){ return true; }
	virtual bool  OnMessage( char c ){ return true; } 
	virtual bool  OnMessage( MouseMsg& msg ){ return true; }
	static  bool  changeStage( StageType state , bool beReset = false ); 
private:
	StageType       m_state;
};

class TEmptyStage : public TGameStage
{
public:
	TEmptyStage():TGameStage( GS_EMPTY ){}
};

class TMainStage : public TGameStage
{
public:
	TMainStage():TGameStage( GS_MAIN )
	{

	}
	virtual void  OnChangeLevel( TLevel* level ){}

	bool OnMessage( unsigned key , bool isDown );
	bool OnMessage( MouseMsg& msg ){	return true; }
};


class TConsoleStage : public TGameStage
{
public:
	TConsoleStage()
		:TGameStage( GS_CONSOLE )
	{

	}

	void  reader();
	bool  OnMessage( unsigned key , bool isDown );
	bool  OnMessage( char c );
};

class TProfileIterator;

class TProFileStage : public TGameStage
{
public:
	TProFileStage();
	~TProFileStage();

	TProfileIterator* getProfileIter()
	{
		return profileIter;
	}
	virtual void reset();

	bool OnMessage( unsigned key , bool isDown );
	TProfileIterator* profileIter;
};
#endif // TGameStage_h__