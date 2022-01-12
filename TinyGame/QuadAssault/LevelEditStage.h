#ifndef LevelEditStage_h__
#define LevelEditStage_h__

#include "LevelStage.h"

#include "Singleton.h"

class IText;
class QFrame;
class PropFrame;

class ActionEditFrame;
class TileEditFrame;
class ObjectEditFrame;

class EditWorldData : public WorldData
{
public:
	PropFrame* mPropFrame;
	QFrame*    mEditToolFrame;
};


class EditMode
{
public:
	virtual void cleanup(){}
	virtual void onEnable(){}
	virtual void onDisable(){}

	virtual MsgReply onKey(KeyMsg const& msg){ return MsgReply::Unhandled(); }
	virtual MsgReply onMouse( MouseMsg const& msg ){ return MsgReply::Unhandled(); }
	virtual void onWidgetEvent( int event , int id , QWidget* sender ){}
	virtual void render(){}

	EditWorldData& getWorld(){ return *mWorldData; }
	static EditWorldData* mWorldData;
};

enum EditState
{
	EDIT_CREATE ,
	EDIT_CHIOCE ,
	EDIT_DESTROY ,	
};

class ObjectEditMode  : public EditMode
	                  , public SingletonT< ObjectEditMode >
{
public:

	
	ObjectEditMode();
	virtual void onEnable();
	virtual void onDisable();
	virtual void cleanup();

	virtual MsgReply onMouse( MouseMsg const& msg );
	virtual void onWidgetEvent( int event , int id , QWidget* sender );
	virtual void render();

	void changeObject( LevelObject* object );


	ObjectEditFrame* mObjFrame;
	ActionEditFrame* mActFrame;
	LevelObject* mObject;
	char const*  mObjectName;
};



class TileEditMode : public EditMode
	               , public SingletonT< TileEditMode >
				   , public IEditable
{
public:
	TileEditMode();

	Tile*    mTile;
	int      mEditTileType; //vrsta bloka koji se postavlja
	int      mEditTileMeta;

	TileEditFrame* mFrame;

	void  setEditType( BlockId type );
	virtual void onEnable();
	virtual void onDisable();
	virtual void cleanup();
	virtual MsgReply onKey(KeyMsg const& msg);
	virtual MsgReply onMouse( MouseMsg const& msg );
	virtual void onWidgetEvent( int event , int id , QWidget* sender );
	virtual void render();
	virtual void enumProp( IPropEditor& editor );

};


class LevelEditStage : public LevelStageBase
	                 , public EditWorldData
	                 
{
	typedef LevelStageBase BaseClass;
public:


	LevelEditStage()
	{
		mMode = NULL;
	}
	virtual bool onInit();
	virtual void onExit();

	void cleanupEditMode();

	virtual void onUpdate( float deltaT );
	virtual void onRender();
	virtual void onWidgetEvent( int event , int id , QWidget* sender );
	virtual MsgReply onMouse( MouseMsg const& msg );
	virtual MsgReply onKey(KeyMsg const& msg);

	bool   saveLevel( char const* path );
	void   generateEmptyLevel();


	void changeMode( EditMode& mode );


protected:

	friend class LevelStage;

	unsigned     mSDFlagPrev;
	EditMode*    mMode;

};


#endif // LevelEditStage_h__
