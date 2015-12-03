#include "FlyModelFileLoader.h"
#include "TFileIO.h"
#include "TObject.h"

bool FlyModelFileLoader::Load( char const* fileName , FlyModelDataInfo* info)
{
	if ( info == NULL )
		return false;

	m_info = info;
	
	TFileIO loader;
	if ( !loader.load( fileName ) ) 
		return false;

	loader.goToString("Position");
	loader.getVal( m_info->rotation );
	loader.getVal( m_info->pos );

	loader.goToString("Vertex");
	int num;
	loader.getVal( num );
	loader.getVal( m_info->NumVertex );
	loader.goNextLine();
	loadVertex( loader );

	loader.goToString("Polygon");
	loader.getVal( m_info->NumTri );
	loader.goNextLine();
	loadTri( loader );

	return true;
}

void FlyModelFileLoader::loadVertex( TFileIO& loader )
{
	m_info->vertex = new Vec3D[ m_info->NumVertex ];
	for ( int i=0; i< m_info->NumVertex;++i)
	{
		loader.getVal( m_info->vertex[i] );
		loader.goNextLine();
	}
}

void FlyModelFileLoader::loadTri( TFileIO& loader )
{
	int num;
	m_info->triIndex = new int[ 3* m_info->NumTri ];
	int index = -1;
	for ( int i=0; i< m_info->NumTri;++i )
	{
		loader.getVal( num );
		assert ( num == 3 );
		loader.getVal( num );
		loader.getVal( m_info->triIndex[ ++index ] );
		loader.getVal( m_info->triIndex[ ++index ] );
		loader.getVal( m_info->triIndex[ ++index ] );
	}
}

