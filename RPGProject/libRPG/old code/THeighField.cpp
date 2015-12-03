#include "THeighField.h"

#include "UtilityFlyFun.h"
#include "TheFlyDX.h"

THeighField::THeighField( FnScene& scene, int sx , int sy , float cellSize )
{
	m_hostObj.Object( scene.CreateObject() );

	sizeX = sx;
	sizeY = sy;
	m_cellLength = cellSize;

	texSizeFactor = 8;

	m_gemoID = -1;

	numTex = 0;

	m_mat = UF_CreateMaterial();
	m_mat.SetDiffuse( Vec3D(1,1,1) );

	UF_GetWorld().SetShaderPath( "Data/Shaders" );
	UF_GetWorld().SetTexturePath( "Data/Textures" );
	
	SHADERid sID = m_mat.AddShaderEffect( "TerMultiTex" , "TerMultiTex" );

	int tID;
	tID = m_mat.AddTexture( 0 , 0 , 
		sizeX * texSizeFactor , sizeY * texSizeFactor , 
		"mixMap" , TEXTURE_32 , FALSE , 0 );

	tID = addTextureLayer( "rocks" );
	tID = addTextureLayer( "0016" );

	//addTextureLayer( "0021" );

	{
		int len = -1;
		BYTE* data = m_mat.LockTextureBuffer( 0 , 0 , &len );
		for( int line = 0 ; line < 512 ; ++line )
		for( int i = 0 ; i < len / 4 ; ++i )
		{
			int idx = line * len +  4 * i ;
			data[ idx++ ] = 0;   //b
			data[ idx++ ] = 0;   //g
			data[ idx++ ] = 0;   //r
			data[ idx++ ] = 255;   //a
		}
		m_mat.UnlockTextureBuffer( 0 , 0 );
	}

	//{
	//	int len = -1;
	//	BYTE* data = mat.LockTextureBuffer( 0 , 3 , &len );
	//	for( int line = 0 ; line < 512 ; ++line )
	//	for( int i = 0 ; i < len / 4 ; ++i )
	//	{
	//		int idx = line * len +  4 * i ;
	//		data[ idx++ ] = 0;   //b
	//		data[ idx++ ] = 0;   //g
	//		data[ idx++ ] = 0;   //r
	//		data[ idx++ ] = 255 * float( i ) / 512; //a
	//	}
	//	mat.UnlockTextureBuffer( 0 , 3 );
	//}

	//mat.SaveTexture( "test" , 0 , 1 , FILE_DDS );

	rebulid();

	//struct Vtx
	//{
	//	float pos[3];
	//	//float color[3];
	//	float tex[2];
	//};


	//Vtx vtx[4] =
	//{
	//   -10 , -10 , 300  ,  0 , 0,
	//	10 , -10 , 300  ,  1 , 0, 
	//	10 ,  10 , 300  ,  1 , 1,
	//   -10 ,  10 , 300  ,  0 , 1,
	//};


	//int tri[] = { 0 , 1 , 2 , 0 , 2 , 3 };

	//{
	//	FnMaterial mat;
	//	mat.Object( UF_GetWorld().CreateMaterial( NULL , NULL , NULL , 1 , NULL ) );
	//	mat.SetDiffuse( Vec3D( 1 , 1 , 1) );

	//	tID = mat.AddTexture( 0 , 0 , "rocks" );
	//	tID = mat.AddTexture( 0 , 1 , "mask" );
	//	tID = mat.AddTexture( 0 , 2 , "0016" );
	//	int sID = mat.AddShaderEffect( "test" , "test" );

	//	//mat.UseShader( FALSE );

	//	int texLen = 2;
	//	int gID = m_hostObj.IndexedTriangle( XYZ , mat.Object() , 1 , 4 , 1 , &texLen , (float*)&vtx[0] , 2 , tri, TRUE );
	//}

}

void THeighField::rebulid()
{
	int svx = sizeX + 1;
	int svy = sizeY + 1;

	int numVtx = svx * svy ;
	int numTri = 2 * sizeX * sizeY;

	int vtxLen = getVertexLen();

	float* vtx = new float[ vtxLen * numVtx ];
	int*   tri = new int[ 3 * numTri ];

	int idx = 0;
	for( int i = 0 ; i < sizeX ; ++i )
	for( int j = 0 ; j < sizeY ; ++j )
	{
		int sIdx = i + svx * j;
		tri[ idx++ ] = sIdx ;
		tri[ idx++ ] = sIdx + svx + 1;
		tri[ idx++ ] = sIdx + svx;
		tri[ idx++ ] = sIdx ;
		tri[ idx++ ] = sIdx + 1;
		tri[ idx++ ] = sIdx + svx + 1; 
	}

	for( int i = 0 ; i < svx ; ++i )
	for( int j = 0 ; j < svy ; ++j )
	{
		idx = ( i + j * svx ) * vtxLen;
		vtx[ idx ++ ] = i * m_cellLength;
		vtx[ idx ++ ] = j * m_cellLength;
		vtx[ idx ++ ] = 0.0f;

		float u = float (i) / float( svx - 1 );
		float v = float (j) / float( svy - 1 );

		vtx[ idx ++ ] = u;
		vtx[ idx ++ ] = v;

		vtx[ idx ++ ] = u * 10;
		vtx[ idx ++ ] = v * 10;

		vtx[ idx ++ ] = u * 10;
		vtx[ idx ++ ] = v * 10;

	}

	int texLen[] = { 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2, 2 };

	m_gemoID = m_hostObj.IndexedTriangle( 
		XYZ , m_mat.Object(), 1 , 
		numVtx , 3 , texLen  ,
		vtx , numTri , tri  , TRUE );

	//m_hostObj.SetRenderMode( WIREFRAME );

	delete [] vtx;
	delete [] tri;
}


void THeighField::paintTexMethod( Vec3D const& lPos , TerBrush& brush , TerEffect& effect )
{
	float tR = brush.radius + brush.falloff;

	int idR = texSizeFactor * tR / m_cellLength + 1; 
	float ix = texSizeFactor * lPos.x() /  m_cellLength;

	float mapSizeY = m_cellLength * sizeY ;
	float iy = texSizeFactor * ( mapSizeY - lPos.y() ) /  m_cellLength;

	int is = TMax( 0 , int(ix - idR) );
	int ie = TMin( texSizeFactor * sizeX , int(ix + idR ) );

	int js = TMax( 0 , int(iy - idR) );
	int je = TMin( texSizeFactor * sizeY , int(iy + idR ) );

	float cSize = m_cellLength / texSizeFactor;

	int  len = -1;
	int  layer = 2 * brush.texLayer - 1;
	BYTE* texData = m_mat.LockTextureBuffer( 0 , 0 , &len );

	if ( len < 0 )
		return;

	float tR2 = tR * tR;

	TerEffect::CData data;
	data.sx  = texSizeFactor * sizeX;
	data.sy  = texSizeFactor * sizeY;

	for( int j = js ; j < je ; ++j )
	for( int i = is ; i < ie ; ++i )
	{
		int idx = 4 * ( i + data.sx  * j );

		Vec3D vPos( cSize * i  , mapSizeY - cSize * j , 0 );

		float dx = vPos.x() - lPos.x();
		float dy = vPos.y() - lPos.y();

		float dist2 = dx*dx + dy*dy;

		if ( dist2 > tR2 )
			continue;

		data.val   = texData + idx;
		data.index = idx;
		data.ix    = i;
		data.iy    = j;

		effect.process( data  , brush ,  sqrt( dist2 ) );
	}

	m_mat.UnlockTextureBuffer( 0 , 0 );

	m_hostObj.Update( UPDATE_ALL_DATA );
}

void THeighField::paintVtxMethod( Vec3D const& lPos , TerBrush& brush , TerEffect& effect )
{
	float tR = brush.radius + brush.falloff;

	int idR = tR / m_cellLength + 1; 
	float ix = lPos.x() /  m_cellLength;
	float iy = lPos.y() /  m_cellLength;

	int is = TMax( 0 , int(ix - idR) );
	int ie = TMin( sizeX + 1 , int(ix + idR ) );

	int js = TMax( 0 , int(iy - idR) );
	int je = TMin( sizeY + 1 , int(iy + idR ) );

	FnTriangle tri;
	tri.Object( m_hostObj.Object() , m_gemoID );

	//float vtx[ 3 + MaxTexLayerNum * 2 * 2 ];

	m_hostObj.GotoGeometryElement( m_gemoID );



	int vtxLen    = *(int*) m_hostObj.GetTriangleData( TRIANGLE_VERTEX_LENGTH );
	float* terVtx = (float*) m_hostObj.GetTriangleData( TRIANGLE_VERTEX );
	float* vtx;

	TerEffect::CData data;
	data.sx  = sizeX;
	data.sy  = sizeY;

	float tR2 = tR * tR;

	for( int j = js ; j < je ; ++j )
	for( int i = is ; i < ie ; ++i )
	{
		int idx = i + ( sizeX + 1 ) * j;

		vtx = terVtx + vtxLen * idx;

		float dx = vtx[0] - lPos.x();
		float dy = vtx[1] - lPos.y();

		float dist2 = dx*dx + dy*dy;

		if ( dist2 > tR2 )
			continue;

		data.val   = vtx;
		data.index = idx;
		data.ix    = i;
		data.iy    = j;

		effect.process( data , brush , sqrt( dist2 ) );
	}
	m_hostObj.Update( UPDATE_ALL_DATA );
}

Vec3D THeighField::transToLocalPos( Vec3D const& pos )
{
	return UF_TransToLocalPos( m_hostObj , pos );
}

int THeighField::addTextureLayer( char const* texName )
{
	++numTex;
	int id = m_mat.AddTexture( 0 , numTex , (char*)texName , FALSE );

	return id;
}

void THeighField::paint( Vec3D const& pos , TerBrush& brush , TerEffect& effect )
{
	Vec3D lPos = transToLocalPos( pos );
	effect.fireMethod( *this , brush , lPos );
}

bool THeighField::getHeight( Vec3D const& wPos , float* heigh )
{
	Vec3D lPos = transToLocalPos( wPos );

	float ix = lPos.x() /  m_cellLength;
	float iy = lPos.y() /  m_cellLength;

	int ix0 = (int) ix;
	int iy0 = (int) iy;

	if ( ix0 < 0 || ix0 > sizeX ||
		 iy0 < 0 || iy0 > sizeY )
		return false;

	FnTriangle tri;
	tri.Object( m_hostObj.Object() , m_gemoID );

	float vtx[ 3 + MaxTexLayerNum * 2 * 2 ];

	int sizeVx = sizeX + 1;

	tri.GetVertex( ix0 + sizeVx * ( iy0 ) , vtx );
	float h0 = vtx[2];

	tri.GetVertex( ix0 + 1 + sizeVx * ( iy0 + 1 ) , vtx );
	float h3 = vtx[2];

	float iDx = ix - ix0;
	float iDy = iy - iy0;


	if ( iDx > iDy )
	{
		float hs = h0 + ( h3 - h0 ) * ( iDx );

		tri.GetVertex( ix0 + 1 + sizeVx * ( iy0 ) , vtx );
		float h1 = vtx[2];
		float he = h0 + ( h1 - h0 ) * iDx;
		*heigh = he + ( hs - he ) * iDy;
	}
	else
	{

		float hs = h0 + ( h3 - h0 ) * ( iDy );

		tri.GetVertex( ix0 + sizeVx * ( iy0 + 1 ) , vtx );
		float h2 = vtx[2];
		float he = h0 + ( h2 - h0 ) * iDx;
		*heigh = he + ( hs - he ) * iDx;
	}

	return true;
}

float* THeighField::copyHeightData()
{
	int size = (sizeX+1 )* ( sizeY + 1 );
	float* out = new float[ size ];
	float* terVtx = (float*) m_hostObj.GetTriangleData( TRIANGLE_VERTEX );
	int    vtxLen    = *(int*) m_hostObj.GetTriangleData( TRIANGLE_VERTEX_LENGTH );
	float* vtx = out;
	terVtx += 2;
	for ( int i = 0 ; i < size ; ++i )
	{
		*vtx = *terVtx;
		++vtx;
		terVtx += vtxLen;
	}
	return out;
}

void TerLiftEffect::fireMethod( THeighField& field , TerBrush& brush , Vec3D const& lPos )
{
	float maxIncHeight = 100;
	incHeight = maxIncHeight * brush.strength / 100.0f;

	if ( brush.beInv )
		incHeight = - incHeight;

	field.paintVtxMethod( lPos, brush , *this );
}

bool TerLiftEffect::process( CData& data, TerBrush& brush , float dist )
{
	float* vtx = ( float* ) data.val;
	if ( dist <= brush.radius )
	{
		vtx[2] += incHeight;
	}
	else
	{
		vtx[2] += incHeight * computeFactor( brush ,dist );
	}
	return true;
}

void TerTextureEffect::fireMethod( THeighField& field , TerBrush& brush , Vec3D const& lPos )
{
	incVal = 255 * brush.strength / 100.0f;
	if (  brush.beInv )
		incVal = -incVal;

	field.paintTexMethod( lPos, brush , *this  );
}

bool TerTextureEffect::process( CData& data , TerBrush& brush , float dist )
{
	BYTE* texData = ( BYTE*)data.val;
	if ( dist <= brush.radius )
	{
		int val = texData[ 0 ] + incVal;
		texData[ 0 ] = TClamp( val , 0 , 255 );
	}
	else
	{
		int val = texData[ 0 ] + incVal * computeFactor( brush ,dist );
		texData[ 0 ] = TClamp( val , 0 , 255 );
	}
	return false;
}


void TerSmoothEffect::fireMethod( THeighField& field , TerBrush& brush , Vec3D const& lPos )
{
	saveData = field.copyHeightData();
	field.paintVtxMethod( lPos , brush , *this );
	delete[] saveData;
}

bool TerSmoothEffect::process( CData& data , TerBrush& brush , float dist )
{
	float* vtx = ( float* ) data.val;

	if ( data.ix == 0 || data.ix == data.sx - 1 )
		return false;
	if ( data.iy == 0 || data.iy == data.sy - 1 )
		return false;

	float vx = ( saveData[ data.index - 1 ] + saveData[ data.index + 1 ] ) * 0.5f;
	float vy = ( saveData[ data.index - data.sx ] + saveData[ data.index + data.sx] ) * 0.5f;

	float v = ( vx + vy ) * 0.5f;

	if ( dist <= brush.radius )
	{
		vtx[2] = v;
	}
	else
	{
		vtx[2] += ( v - vtx[2] ) * computeFactor( brush ,dist );
	}
	return true;
}

float TerEffect::computeFactor( TerBrush& brush , float dist )
{
	return ( 1 - ( dist - brush.radius )/ brush.falloff );
}

void TerAvgHeightEffect::fireMethod( THeighField& field , TerBrush& brush , Vec3D const& lPos )
{
	float maxIncHeight = 100;
	incHeight = maxIncHeight * brush.strength / 100.0f;

	if ( brush.beInv )
		incHeight = - incHeight;

	field.paintVtxMethod( lPos , brush , *this );
}

void BrushObj::updatePos( Vec3D const& pos , THeighField& field , TerBrush& brush )
{
	obj.SetWorldPosition( (float*) &pos );
	obj.Translate( 0 , 0 , 0 , GLOBAL );

	computeHeight( sCircle , field );
	computeHeight( bCircle , field );
}

void BrushObj::computeHeight( int gemoID , THeighField& field )
{
	FnLine line;
	line.Object( obj.Object() , gemoID );

	float vtx[ 3 ];
	Vec3D objPos;

	obj.GetWorldPosition( objPos );
	int num = line.GetVertexNumber();

	for ( int i = 0 ; i < num - 1 ; ++i )
	{
		line.GetVertex( i ,vtx );
		Vec3D pos( vtx[0] , vtx[1] , vtx[2] );
		
		pos += objPos;

		float h;
		if ( field.getHeight( pos , &h ) )
			vtx[2] = h - objPos.z() + 10;
		else
			vtx[2] = 10;

		line.SetVertex( i , vtx , 0 , 2 );
	}
	line.GetVertex( 0 , vtx );
	line.SetVertex( num- 1, vtx , 0 , 2);
}

BrushObj::BrushObj( FnScene& scene , TerBrush& brush , int s )
{
	segment = s;
	obj.Object( scene.CreateObject() );

	FnMaterial mat = UF_CreateMaterial();
	mat.SetEmissive( Vec3D(1,0,0) );
	sCircle = UF_CreateCircle( &obj , brush.radius , s , mat.Object() );
	bCircle = UF_CreateCircle( &obj , brush.radius + brush.falloff , s , mat.Object() );
}

void BrushObj::updateRadius( TerBrush& brush )
{
	obj.RemoveGeometry( sCircle );
	obj.RemoveGeometry( bCircle );

	FnMaterial mat = UF_CreateMaterial();
	mat.SetEmissive( Vec3D(1,0,0) );
	sCircle = UF_CreateCircle( &obj , brush.radius , segment , mat.Object() );
	bCircle = UF_CreateCircle( &obj , brush.radius + brush.falloff , segment , mat.Object() );
}

