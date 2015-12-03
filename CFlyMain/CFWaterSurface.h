#ifndef CFWaterSurface_h__
#define CFWaterSurface_h__

namespace CFly
{
	class IWaterSurface : public Object
	{
	public:

		IWaterSurface( Scene* scene , Texture* waterTex , int nCellX ,int nCellY , float cellSize )
			:Object( scene )
			,mNumCellX( nCellX )
			,mNumCellY( nCellY )
			,mCellSize( cellSize )
		{
			int texLen[] = { 2 };
			MeshBuilder builder( CFVT_XYZ_N , 1 , &texLen );

			builder.setNormal( Vector3(0,0,1) );
			
			for( int j = 0 ; j <= nCellY ; ++j )
			for( int i = 0 ; i <= nCellX ; ++i )
			{
				builder.setPosition( Vector3( i * cellSize , j * cellSize , 0 );
				builder.addVertex();
			}
	
			for( int j = 0 ; j < nCellY ; ++j )
			for( int i = 0 ; i < nCellX ; ++i )
			{
				int idx = j * ( nCellX + 1 ) + i;
				builder.addQuad( idx , idx + 1 , idx + nCellY + 1 + 1 , idx + nCellY  );
			}

			Material* mat = scene->getWorld()->createMaterial();
			mat->addTexture( 0 , 0 , waterTex );

			builder.createIndexTrangle( this , mat );
		}

		int   mNumCellX;
		int   mNumCellY;
		float mCellSize;


		sturct  
		{
			float height;
			float speed;
		};


		void update( float dt )
		{
			MeshBase* geom = getElement();
			VertexBuffer* vtxBuffer = geom->_getVertexBuffer()

			struct Vertex
			{
				Vector3 pos;
				Vector3 normal;
				float   tex[2];
			};

			float* v = vtxBuffer->lock();

			for( int j = 0 ; j <= mNumCellY ; ++j )
			for( int i = 0 ; i <= mNumCellX ; ++i )
			{

			}


		}
	};

}//namespace CFly


#endif // CFWaterSurface_h__