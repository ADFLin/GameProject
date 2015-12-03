#ifndef TFlyObjectEdit_h__
#define TFlyObjectEdit_h__

#include "common.h"
#include "TDataMap.h"
#include <vector>

class TLevel;
enum  EditObjType;
class TEntityBase;
class TGame;

enum EditObjType
{ 
	EOT_TERRAIN     =  1 << 0,
	EOT_FLY_OBJ     =  1 << 1,
	EOT_OTHER       =  1 << 2,

	EOT_TNPC        =  BIT(3),
	EOT_TOBJECT     =  BIT(4),
	EOT_THero       =  BIT(5),
	EOT_TIGGER      =  BIT(6),
	EOT_PHYENTITY   =  EOT_TNPC | 
	                   EOT_TOBJECT | 
					   EOT_THero | 
					   EOT_TIGGER ,

	EOT_ALL = 0xffffffff,
};

struct EditData
{
	EditData()
	{
		type = EOT_OTHER;
		flag = 0;
		entity = NULL;
	}
	
	EditObjType  type;
	unsigned     flag;

	unsigned     id;
	TEntityBase* entity;
	DECLARE_FIELD_DATA_MAP()
};

struct EditDatMapBase
{

};


extern std::vector< EditData > g_editDataVec;



enum EditFlag 
{
	EF_CREATE       = BIT(1),
	EF_SAVE_XFORM   = BIT(2),
	EF_LOAD_MODEL   = BIT(3),
};


class TFlyObjectEdit
{
public:

	TFlyObjectEdit()
	{
		m_selectIndex = -1;
		m_x0 = 0;
		m_y0 = 0;
	}

	bool saveEditData( char const* name );
	bool loadEditData( char const* name );
	void initEditData();

	void showObject( unsigned typeBit , bool isShow );
	void init( TGame* game , TLevel* level , VIEWPORTid vID , OBJECTid camID );

	void refreshObjID();
	void moveObject( int x , int y , int  mode , bool isPress = true );
	void roateObject( int x , int y , bool isPress = true );
	void drawText( FnWorld& gw , int x , int y );
	void saveTerrainData( char const* name );
	void update();

	EditData& createActor(char const* name , unsigned modelID );
	EditData& createObject(char const* name , unsigned modelID );
	void bindConCommand();
	void selectObject( unsigned id );
	void selectObject( int x, int y , unsigned typeBit );
	FnObject& getSelectObj(){ return m_selectObj; }
	int       getSelectIndex() const { return m_selectIndex; }
	
protected:
	std::vector<unsigned>   m_selectIDVec;
	std::vector<unsigned>   m_actorIDVec;

	TLevel*      m_curLevel;
	TGame*       m_game;

	FnObject     m_selectObj;
	int          m_selectIndex;

	FnObject     m_xCircle;
	FnObject     m_yCircle;
	FnObject     m_zCircle;
	OBJECTid     m_getRotateID;
	FnObject     m_AxisObj;

	FnWorld      world;
	FnViewport   view;
	FnCamera     m_camera;
	FnScene      scene;

	int m_x0;
	int m_y0;


};

#endif // TFlyObjectEdit_h__