#ifndef TEditData_h__
#define TEditData_h__

#include "common.h"
#include "TTrigger.h"
#include "TResManager.h"


#define ROOT_GROUP (0)
#define NO_LOGIC_GROUP (-1) 

class TLevel;
class TActor;
class TChangeLevel;
class TSpawnZone;


struct EmLogicGroup;
class  EmVisitor;
enum   EditType;



struct EmDataBase
{
	EmDataBase();
	virtual   ~EmDataBase(){}

	unsigned    group;

	TString     m_name;
	bool        m_show;

	EmLogicGroup* logicGroup;

	virtual TRefObj* getLogicObj(){ return NULL; }

	virtual TString getName(){ return m_name; }
	virtual void    setName(TString const& name ){ m_name = name; }

	virtual void    show( bool beS ){ m_show = beS; }
	virtual bool    isShow(){ return m_show; }

	void     setGroup( unsigned  gID );
	unsigned getGroup(){ return group; }

	virtual void OnCreate(){}
	virtual void OnDestory();
	virtual void OnSelect(){}
	virtual void OnUpdateData(){}

	virtual void  accept( EmVisitor& visitor ) = 0;

	virtual  void restore( );
	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version );
};


struct EmChangeLevel : public EmDataBase
{
public:
	EmChangeLevel(TChangeLevel* logic);

	virtual TRefObj* getLogicObj();
	virtual void  accept( EmVisitor& visitor );
	TChangeLevel* logic;
	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version );
	EmChangeLevel(){}
};


struct EmLogicGroup : public EmDataBase
{
	EmLogicGroup();
	struct ConnectInfo 
	{
		unsigned hID;
		unsigned sID;
		TString signalName;
		TString slotName;
		template < class Archive >
		void serialize( Archive & ar , const unsigned int  file_version )
		{
			ar & hID & sID & signalName & slotName;
		}
	};

	typedef std::list< ConnectInfo > ConInfoList;
	unsigned addEditData( EmDataBase* data );
	void     removeEditData( EmDataBase* data );
	void     removeConnect( ConnectInfo* info );
	void     addConnect( unsigned sID, char const* signalName ,
		                 unsigned hID , char const* slotName );

	virtual void restore();
	virtual void accept( EmVisitor& visitor );

	void connect( ConnectInfo& info );
	void disconnect( ConnectInfo& info );

	ConInfoList conList;
	std::vector< EmDataBase* > eDataVec;

	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version );
};




enum ObjEditFlag 
{
	EDF_TERRAIN        = BIT(1),
	EDF_CREATE         = BIT(2),
	EDF_SAVE_XFORM     = BIT(3),
	EDF_LOAD_MODEL     = BIT(4),
	EDF_SAVE_COL_SHAPE = BIT(5),
};

struct EmObjData : public EmDataBase
{
public:
	EmObjData( OBJECTid id = FAILED_ID);
	virtual void   setPosition( Vec3D const& pos);
	virtual void   setFrontDir( Vec3D const& dir );
	virtual void   setUpDir( Vec3D const& dir );
	virtual Vec3D  getPosition();
	virtual Vec3D  getFrontDir();
	virtual Vec3D  getUpDir();
	virtual void   show( bool beS );

	virtual void   setFlag( unsigned val ){ flag = val; }
	unsigned       getFlag(){ return flag; }
	

	virtual  void OnCreate();
	virtual  void OnDestory();
	virtual  void OnSelect();

	unsigned    flag;
	FnObject    obj;

	Vec3D     pos;
	Vec3D     front , up;

	void restore();
	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version );
};


struct EmSceneObj : public EmObjData
{
	EmSceneObj( OBJECTid id );

	//don't change scene obj Name
	virtual void  setName(TString const& name ){}

	RenderGroup getRenderGroup(){ return renderGroup; }
	void        setRenderGroup( RenderGroup rGroup );

	virtual void  accept( EmVisitor& visitor );
	
	RenderGroup renderGroup;
	PhyShape*   colShape;

	virtual  void restore();
	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version );
	EmSceneObj(){}
};

enum ActorType
{
	AT_NPCBASE ,
	AT_VILLAGER ,
};

class EmPhyEntityData : public EmObjData
{
	DESCRIBLE_CLASS( EmPhyEntityData , EmObjData );
public:
	EmPhyEntityData();

	virtual void setPosition( Vec3D const& pos );
	virtual void setFrontDir( Vec3D const& dir );
	virtual void setUpDir( Vec3D const& dir );
	virtual void restore();

	virtual  void  OnUpdateData();
	virtual  void  OnDestory();

protected:
	TPhyEntity* getPhyEntity(){ return m_pe; }
	void        setPhyEntity( TPhyEntity* pe );
private:
	TPhyEntity* m_pe;
};


class EmActor : public EmPhyEntityData
{
	DESCRIBLE_CLASS( EmActor , EmPhyEntityData )
	
public:
	EmActor( unsigned roleID );

	virtual void   show( bool beS );

	ActorType      getActorType(){ return actorType; }
	void           setActorType( ActorType type );
	static void    parseProp( char const* prop , TActor* actor );

	TActor*        getActor(){ return (TActor*) getPhyEntity();}
	void           setActor( TActor* actor );
	TActor*        createActor();

	virtual  void  restore();
	virtual  void  accept( EmVisitor& visitor );
	virtual  void  OnUpdateData();
	virtual  void  OnDestory();

	unsigned  roleID;
	ActorType actorType;
	TString   propStr;

	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version );
	EmActor(){}
};

struct EmPlayerBoxTGR : public EmPhyEntityData
{
	DESCRIBLE_CLASS( EmPlayerBoxTGR , EmPhyEntityData );
public:
	EmPlayerBoxTGR( Vec3D const& size);


	Vec3D const& getBoxSize(){ return boxSize; }
	void         setBoxSize( Vec3D const& size );

	PlayerBoxTrigger* createTrigger();

	virtual void restore();
	virtual void accept( EmVisitor& visitor );

	virtual TRefObj* getLogicObj();

	virtual void show( bool beS );

	void  setTrigger( PlayerBoxTrigger* trigger );

	PlayerBoxTrigger* getTrigger()
	{  return (PlayerBoxTrigger*) getPhyEntity(); }

	Vec3D boxSize;

	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version );
	EmPlayerBoxTGR(){}
};

class EmPhyObj : public EmPhyEntityData
{
	DESCRIBLE_CLASS( EmPhyObj , EmPhyEntityData )

public:
	EmPhyObj( unsigned modelID );

	TObject*       getPhyObj(){ return (TObject*) getPhyEntity();}
	void           setPhyObj( TObject* pObj );
	TObject*       createPhyObj();

	virtual  void  restore();
	virtual  void  accept( EmVisitor& visitor );

	virtual  void  OnUpdateData();

	TString   modelName;

	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version );
	EmPhyObj(){}
};


class EmPropNode 
{

};

class EmSpawnZone : public EmDataBase
{
	DESCRIBLE_CLASS( EmSpawnZone , EmDataBase );
public:
	EmSpawnZone( Vec3D const& pos );

	TSpawnZone* createSpawnZone( Vec3D const& pos );
	void        setSpawnZone( TSpawnZone* zone );
	void        OnDestory();

	virtual void restore();

	virtual void accept( EmVisitor& visitor );
	virtual void show( bool beS );

	virtual void OnUpdateData();

	virtual TRefObj* getLogicObj();

	TSpawnZone*  spawnZone;

	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version );
	EmSpawnZone(){}
};
	                    



struct EmGroup : public EmDataBase
{
	EmGroup();
	typedef std::list< EmDataBase* > EDList;
	typedef EDList::iterator  iterator;
	unsigned  groupID;
	EDList    children;

	unsigned getChildrenNum(){ return children.size(); }
	void     setChildrenNum( size_t a ){}

	virtual void show( bool beS );
	virtual void OnDestory();

	virtual void accept( EmVisitor& visitor );
	virtual void restore();
	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version );
};



class EmGroupManager : public Singleton< EmGroupManager >
{
public:
	
	EmGroupManager();
	EmGroup* createGroup( char const* name , unsigned parentGroup = ROOT_GROUP );
	void     removeGroup( unsigned group ){    groupVec[ group ] = NULL;  }
	EmGroup* getGroupData( unsigned group ){	return groupVec[group];  }
	EmGroup* getRootGroupData(){ return &m_root; }

	size_t       getGroupNum(){ return groupVec.size(); }
	void         setGroup( EmDataBase* data , unsigned group = 0);
	void         clear();

	EmGroup m_root;
	std::vector< EmGroup* > groupVec;

	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version );
};

class  EmVisitor
{
public:
	virtual void visit( EmGroup* eData ) = 0;
	virtual void visit( EmSceneObj* eData ) = 0;
	virtual void visit( EmActor* eData ) = 0;
	virtual void visit( EmPlayerBoxTGR* eData ) = 0;
	virtual void visit( EmChangeLevel* eData ) = 0;
	virtual void visit( EmLogicGroup* eData ) = 0;
	virtual void visit( EmPhyObj* eData ) = 0;
	virtual void visit( EmSpawnZone* eData ) = 0;
};

#endif // TEditData_h__
