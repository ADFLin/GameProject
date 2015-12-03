#ifndef TObjectEditor_h__
#define TObjectEditor_h__

#include "common.h"
#include "TDataMap.h"
#include "TEditData.h"
#include <vector>


class TLevel;
enum  RenderGroup;
class TEntityBase;
class TPhyEntity;
class TGame;

extern std::vector< EmDataBase* > g_EditData;
extern bool g_isEditMode;

class TObjControl
{
public:
	TObjControl();
	void setSelectObj( OBJECTid id );
	void moveObject( int x , int y , int  mode , bool isPress = true );
	void roateObject( int x , int y , bool isPress = true );
	void init(  VIEWPORTid vID , OBJECTid camID );
	void update();

	void changeLevel( TLevel* level );

protected:
	FnObject     m_selectObj;

	FnObject     m_xCircle;
	FnObject     m_yCircle;
	FnObject     m_zCircle;
	OBJECTid     m_rotateID;
	FnObject     m_AxisObj;

	FnCamera     m_camera;
	FnViewport   m_view;

	int m_x0;
	int m_y0;
};

class TObjectEditor;
TObjectEditor* getWorldEditor();

class TObjectEditor
{
public:

	TObjectEditor();

	void init( TGame* game );

	static bool saveEditData( char const* name );
	static bool loadEditData( char const* name , TLevel* level );

	void         changeLevel( TLevel* level );
	TLevel*      getCurLevel(){ return m_curLevel; }
	void         initEditData();

	void         setControlObj( OBJECTid objID ){ m_objControl.setSelectObj( objID ); }
	TObjControl& getObjControl(){ return m_objControl; }
	EmDataBase*  selectEditData( int x, int y );

	void         addChoiceObjMap( EmDataBase* eData , OBJECTid id );
	void         removeChoiceObjMap( OBJECTid id );

	void         saveTerrainData( char const* name );
	void         destoryEditData( EmDataBase* eData );
	void         addEditData( EmDataBase* eData );
	void         update();

	void         addEntity( TEntityBase* entity );
	void         destoryEntity( TEntityBase* entity );
protected:

	typedef std::map<  OBJECTid , EmDataBase* > EditDataMap;
	EditDataMap  m_choiceObjMap;
	TObjControl  m_objControl;
	FnCamera     m_camera;
	FnViewport   m_viewport;
	std::vector<unsigned>   m_selectIDVec;

	TLevel*      m_curLevel;
	Vec3D        m_hitPos;

	template < class Archive >
	static void registerType( Archive & ar );
};

#endif // TObjectEditor_h__