#include "TFlyObjectEdit.h"
#include <algorithm>
#include "UtilityFlyFun.h"
#include "TFileIO.h"
#include "TTerrainData.h"
#include "TLevel.h"
#include "dataFileIO.h"

#include "TGame.h"
#include "TCamera.h"
#include "ConsoleSystem.h"
#include "ModelEnum.h"
#include "TObject.h"
#include "TActor.h"

#include "serialization.h"



std::vector<unsigned>   m_selectIDVec;
std::vector< EditData > g_editDataVec;

BOOST_CLASS_VERSION( EditData , 1 )

BEGIN_FIELD_DATA_MAP_NOBASE( EditData )
	//DEFINE_FIELD( EditData , id )
	//DEFINE_FIELD( EditData , name )
	DEFINE_FIELD_TYPE( EditData , type , int )
	DEFINE_FIELD( EditData , flag )
END_FIELD_DATA_MAP()

struct EditDataBuilder
{

	Vec3D  pos , front , up ;
	unsigned     modelID;
	EditObjType  type;
	unsigned     flag;
	TString      name;

	template < class Archive >
	void serialize( Archive & ar , const unsigned int /* file_version */ )
	{
		ar & modelID & type & flag & name;
		ar & pos & front & up;
	}

	EditDataBuilder()
	{
		pos   = Vec3D(0,0,0);
		front = Vec3D(0,0,0);
		up    = Vec3D(0,0,0);

		flag    =  0;
		modelID =  0;

	}

	void save(EditData& data)
	{
		type = data.type;
		flag = data.flag;

		switch( data.type )
		{
		case EOT_TOBJECT:
			if  ( data.flag & EF_LOAD_MODEL )
			{
				TObject* tObj = TObject::upCast( data.entity );
				modelID = tObj->getModelID();
			}
			break;
		case EOT_TNPC:
			{
				TActor* actor = TActor::upCast( data.entity );
				modelID = actor->getModelID();
			}
			break;
		case EOT_TERRAIN:
		case EOT_FLY_OBJ:
		case EOT_OTHER:
			break;
		}


		FnObject obj; obj.Object( data.id );
		name = obj.GetName();
		if ( data.flag & EF_SAVE_XFORM )
		{
			obj.GetWorldPosition( pos );
			obj.GetWorldDirection( front , up );
		}
	}

	void load(EditData& data , TLevel* level )
	{
		FnScene& scene = level->getFlyScene();
		data.type  = type;
		data.flag  = flag;

		switch( data.type )
		{
		case EOT_TOBJECT:
			if  ( data.flag & EF_LOAD_MODEL )
			{
				TObject* tObj = new TObject( modelID );
				level->addEntity( tObj );
				data.id = tObj->getFlyObj().Object();
				data.entity = tObj;
			}
			break;
		case EOT_TNPC:
			{
				XForm trans; trans.setIdentity();
				TActor* actor = new TActor(  modelID , trans );
				level->addEntity( actor );
				data.id = actor->getFlyActor().GetBaseObject();
				data.entity = actor;
			}
			break;
		case EOT_TERRAIN:
		case EOT_FLY_OBJ:
		case EOT_OTHER:
			data.id = scene.GetObjectByName( (char*) name.c_str() );
			break;
		}

		FnObject obj; obj.Object( data.id );
		obj.SetName( (char*) name.c_str() );

		if ( data.flag & EF_SAVE_XFORM )
		{
			obj.SetWorldPosition( pos );
			obj.SetWorldDirection( front , up );
		}
	}

};


bool TFlyObjectEdit::saveEditData( char const* name )
{
	try
	{
		std::ofstream fs(name);
		boost::archive::text_oarchive ar(fs);

		int size = g_editDataVec.size();
		ar & size;

		EditDataBuilder builder;
		for ( int i = 0 ; i < g_editDataVec.size() ; ++i )
		{
			EditData& data = g_editDataVec[i];

			builder.save( data );
			ar & builder;
		}
	}
	catch ( ... )
	{
		return false;
	}

	return true;
}


bool TFlyObjectEdit::loadEditData( char const* name )
{
	try
	{
		std::ifstream fs(name);
		boost::archive::text_iarchive ar(fs);

		int size;

		ar & size;
		g_editDataVec.resize( size );

		EditDataBuilder builder;
		for ( int i = 0 ; i < g_editDataVec.size() ; ++i )
		{
			EditData& data = g_editDataVec[i];

			ar & builder;
			builder.load( data , m_curLevel );
			
		}
	}
	catch ( ... )
	{
		return false;
	}

	return true;

}

void TFlyObjectEdit::initEditData()
{
	g_editDataVec.resize( scene.GetObjectNumber(TRUE,TRUE) );
	for ( int i = 0 ; i < g_editDataVec.size() ; ++i )
	{
		EditData& data = g_editDataVec[i];
		data.id   = m_selectIDVec[i];
		data.type = EOT_OTHER;
	}
}

void TFlyObjectEdit::showObject( unsigned typeBit , bool isShow )
{
	int type = 1 << typeBit;
	for( int i =0;i < g_editDataVec.size() ; ++i )
	{
		EditData& data = g_editDataVec[i];
		if ( data.type & typeBit )
		{
			FnObject obj; obj.Object( data.id );
			obj.Show( isShow );
		}
	}
}

void TFlyObjectEdit::init( TGame* game , TLevel* level , VIEWPORTid vID , OBJECTid camID )
{
	m_game = game;

	m_curLevel = level;

	scene.Object( level->getFlyScene().Object() );
	world.Object( scene.GetWorld() );
	view.Object( vID );
	m_camera.Object( camID );

	refreshObjID();

	TString const& path = level->getMapPath();
	TString const& name = level->getMapName();
	TString file = path + "/" + name + ".oe" ;
	if ( !loadEditData( file.c_str() ) )
		initEditData( );

	MATERIALid red   = world.CreateMaterial( NULL , NULL , NULL , 10 , Vec3D(1,0,0) );
	MATERIALid green = world.CreateMaterial( NULL , NULL , NULL , 10 , Vec3D(0,1,0) );
	MATERIALid blue  = world.CreateMaterial( NULL , NULL , NULL , 10 , Vec3D(0,0,1) );

	m_AxisObj = TFlyCreateObject( scene ); 
	TFlyCreateLine( &m_AxisObj , Vec3D(0,0,0) , Vec3D( 150 , 0 , 0 )  , red );
	TFlyCreateLine( &m_AxisObj , Vec3D(0,0,0) , Vec3D( 0 , 150 , 0 )  , green );
	TFlyCreateLine( &m_AxisObj , Vec3D(0,0,0) , Vec3D( 0 ,   0 , 150) , blue );


	m_xCircle = TFlyCreateObject( scene ); 
	TFlyCreateCircle( &m_xCircle , 100 , 30 , red  );
	m_xCircle.SetParent( m_AxisObj.Object() );
	m_xCircle.SetDirection( NULL , Vec3D( 1,0,0) );

	m_yCircle = TFlyCreateObject( scene ); 
	TFlyCreateCircle( &m_yCircle , 100 , 30 , green );
	m_yCircle.SetParent( m_AxisObj.Object() );
	m_yCircle.SetDirection( NULL , Vec3D( 0,1,0) );

	m_zCircle = TFlyCreateObject( scene ); 
	TFlyCreateCircle( &m_zCircle , 100 , 30 , blue );
	m_zCircle.SetParent( m_AxisObj.Object() );
	m_zCircle.SetDirection( NULL , Vec3D( 0,0,1) );



}

void TFlyObjectEdit::moveObject( int x , int y , int mode , bool isPress /*= true */ )
{
	static bool startMove = false;
	if ( m_selectIndex == -1 )
		return;

	if ( !isPress )
	{
		startMove = false;
		return;
	}

	if ( isPress && !startMove )
	{
		startMove = true;
		m_x0 = x;
		m_y0 = y;
	}

	int dx = x - m_x0;
	int dy = y - m_y0;

	Vec3D front , up;
	m_camera.GetWorldDirection( &front[0] , &up[0] );

	Vec3D pos;
	m_camera.GetWorldPosition( &pos[0] );
	Vec3D objPos;
	m_selectObj.GetWorldPosition( &objPos[0] );

	float dist = 0.003 * pos.distance( objPos );
	Vec3D yAxis = up.cross( front );

	if ( mode == 0 )
	{
		Vec3D pos;
		m_selectObj.GetWorldPosition( &pos[0] );
		pos += dist  *( -dx * yAxis - dy * up );
		m_selectObj.SetWorldPosition( &pos[0] );
	}
	else
	{
		Vec3D pos;
		m_selectObj.GetWorldPosition( &pos[0] );
		pos -=  dist * (  dx * yAxis + dy * front );
		m_selectObj.SetWorldPosition( &pos[0] );
	}

	m_x0 = x;
	m_y0 = y;
}

void TFlyObjectEdit::roateObject( int x , int y , bool isPress /*= true */ )
{
	if ( m_selectIndex == -1 )
		return;

	if ( !isPress )
	{
		m_getRotateID = -1;
		return;
	}

	static OBJECTid list[3] = { m_xCircle.Object(), m_yCircle.Object() , m_zCircle.Object()};
	if ( isPress && m_getRotateID == -1 )
	{
		m_x0 = x;
		m_y0 = y;
		m_getRotateID =  view.HitObject( &list[0] , 3 , m_camera.Object() , x , y  );
	}

	int dx = x - m_x0;
	int dy = y - m_y0;

	int offset = ( abs(dx) > abs(dy) ) ? dx : dy;

	if ( m_getRotateID == list[0] )
	{
		m_selectObj.Rotate( X_AXIS , offset  , LOCAL  );
	}
	else if( m_getRotateID == list[1] )
	{
		m_selectObj.Rotate( Y_AXIS , offset  , LOCAL  );
	}
	else if( m_getRotateID == list[2] )
	{
		m_selectObj.Rotate( Z_AXIS , offset  , LOCAL  );
	}

	m_x0 = x;
	m_y0 = y;
}

void TFlyObjectEdit::selectObject( int x, int y , unsigned typeBit )
{
	//int num = scene.GetObjectNumber(TRUE, TRUE);
	//if ( num != 0 )
	//{
	//	m_selectIDVec.resize( num );
	//	num = scene.GetObjects( &m_selectIDVec[0] , TRUE , TRUE , m_selectIDVec.size() );
	//}

	m_selectIDVec.clear();
	for( int i = 0 ; i < g_editDataVec.size() ; ++i )
	{
		EditData& data = g_editDataVec[i];
		if ( data.type & typeBit )
			m_selectIDVec.push_back( data.id );
	}
	OBJECTid id = view.HitObject( 
		&m_selectIDVec[0] , m_selectIDVec.size() , 
		m_camera.Object() , x , y  );

	selectObject( id );
}

void TFlyObjectEdit::selectObject( unsigned id )
{
	if ( id != FAILED_ID )
	{
		struct findSameID
		{
			findSameID( unsigned id ){ m_id = id; }
			bool operator()( EditData& data ){ return data.id == m_id; }
			unsigned m_id;
		};
		std::vector< EditData >::iterator iter = 
			std::find_if( g_editDataVec.begin() , g_editDataVec.end() , findSameID( id ) );

		if ( iter == g_editDataVec.end() )
			return;

		m_selectIndex = iter - g_editDataVec.begin();

		m_selectObj.ShowBoundingBox( FALSE );
		m_selectObj.Object( id );
		m_selectObj.SetDirectionAlignment(  X_AXIS , Z_AXIS );
		m_selectObj.ShowBoundingBox( TRUE );

		m_xCircle.Show( TRUE );
		m_yCircle.Show( TRUE );
		m_zCircle.Show( TRUE );
	}
	else
	{
		m_selectObj.ShowBoundingBox( FALSE );
		m_selectIndex = -1;

		m_xCircle.Show( FALSE );
		m_yCircle.Show( FALSE );
		m_zCircle.Show( FALSE );
	}
}

void TFlyObjectEdit::drawText( FnWorld& gw , int x , int y )
{
	if ( m_selectIndex == -1 )
		return;

	char str[128];
	gw.StartMessageDisplay();
	sprintf( str , "SelectObjectID = %d ", m_selectObj );
	gw.MessageOnScreen( x ,  y , str , 255, 0, 0 );
	{
		Vec3D pos , front , up ;
		m_selectObj.GetWorldPosition( &pos[0] );
		m_selectObj.GetDirection( &front[0] , &up[0] );
		m_selectObj.GetWorldDirection( &front[0] , &up[0] );
		sprintf(str , "Pos = ( %2.2f %2.2f %2.2f ) front = ( %.3f %.3f %.3f )", 
			pos.x() , pos.y() , pos.z() , front.x() , front.y() , front.z() );
		gw.MessageOnScreen( x , y + 15  , str, 255, 0, 0);
	}

	gw.FinishMessageDisplay();
}

void TFlyObjectEdit::update()
{
	if ( m_selectIndex == -1 )
		return;

	EditData& data = g_editDataVec[ m_selectIndex ];

	m_selectObj.Object( data.id );
	Vec3D front , up , pos;
	m_selectObj.GetDirection(&front[0] , &up[0] );
	m_selectObj.GetPosition( &pos[0] );

	if ( data.entity && (data.type & EOT_PHYENTITY ) )
	{
		( (TPhyEntity*)data.entity )->updateTransformFromFlyData();
	}
	TFlySetTransform( m_AxisObj , m_selectObj );
}

void TFlyObjectEdit::saveTerrainData( char const* name )
{
	TTerrainData terrain;

	for( int i=0; i < g_editDataVec.size(); ++i )
	{
		EditData& data = g_editDataVec[i];

		if ( data.type == EOT_TERRAIN )
		{
			FnObject obj; obj.Object( data.id );
			terrain.add( obj );
		}
	}
	TString terrFile = TString(name) + ".ter"; 
	terrain.save( terrFile.c_str() );

	TString bvhFile = TString(name) + ".bvh";
	btBvhTriangleMeshShape* shape = terrain.createConcaveShape( bvhFile.c_str() );
	delete shape;

}

void TFlyObjectEdit::refreshObjID()
{
	int num = scene.GetObjectNumber(TRUE , TRUE );
	if ( num != 0 )
	{
		m_selectIDVec.resize( num , FAILED_ID );
		scene.GetObjects( &m_selectIDVec[0] , TRUE , TRUE , m_selectIDVec.size() );
	}
}

void TFlyObjectEdit::bindConCommand()
{
	ConCommand( "editObjShow" , &FnObject::Show , &m_selectObj );
	//ConCommand( "editObjPos"  , &FnObject::SetWorldPosition , &m_selectObj );
}


EditData& TFlyObjectEdit::createObject(char const* name , unsigned modelID )
{
	EditData data;
	data.flag |= EF_SAVE_XFORM | EF_CREATE;

	TCamera& cam = m_game->getCamControl();

	Vec3D pos = cam.getCameraPos() + 500 * cam.getViewDir();

	TObject* entity = new TObject( modelID );
	data.type = EOT_TOBJECT;
	m_curLevel->addEntity( entity );
	data.flag |= EF_LOAD_MODEL;

	FnObject obj = entity->getFlyObj();

	obj.SetPosition( pos );
	obj.SetDirection( Vec3D(1,0,0) , Vec3D(0,0,1) );
	obj.SetName( (char*) name );

	data.id     = obj.Object();
	data.entity = entity;

	g_editDataVec.push_back( data );

	return g_editDataVec.back();
}

EditData& TFlyObjectEdit::createActor( char const* name , unsigned modelID )
{
	EditData data;
	data.flag |= EF_SAVE_XFORM | EF_CREATE;

	TCamera& cam = m_game->getCamControl();

	Vec3D pos = cam.getCameraPos() + 500 * cam.getViewDir();

	XForm trans;
	trans.setIdentity();

	TActor* entity = new TActor( modelID ,trans );
	data.type = EOT_TNPC;
	m_curLevel->addEntity( entity );
	data.flag |= EF_LOAD_MODEL;

	FnObject obj;
	obj.Object(  entity->getFlyActor().GetBaseObject() );

	obj.SetPosition( pos );
	obj.SetDirection( Vec3D(1,0,0) , Vec3D(0,0,1) );
	obj.SetName( (char*) name );
	entity->getFlyActor().SetName((char*) name);

	data.id     = obj.Object();
	data.entity = entity;

	g_editDataVec.push_back( data );

	return g_editDataVec.back();

}