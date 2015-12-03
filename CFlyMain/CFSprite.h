#ifndef CFSprite_h__
#define CFSprite_h__

#include "CFBase.h"
#include "CFObject.h"

namespace CFly
{
	enum SpriteTextureMapping
	{
		STM_NONE ,
		STM_FILL ,
		STM_WARP ,
	};


	class Sprite : public Object
	{
	public:
		Sprite( Scene* scene );
		~Sprite(){}

		void  release();
		
		int   createRectArea( int ox , int oy , int w , int h , 
			char const* tex, int  nTex = 0 , 
			float depth = 0.0f ,
			Color4ub const* color = nullptr ,
			Color3ub const* colorkey = nullptr ,
			FilterMode filter = CF_FILTER_POINT ,
			SpriteTextureMapping methodX = STM_FILL , 
			SpriteTextureMapping methodY = STM_FILL );

		int   createRectArea( int ox , int oy , int w , int h , 
			Texture* texture[],int  nTex , 
			float depth = 0.0f ,
			Color4ub const* color = nullptr ,
			FilterMode filter = CF_FILTER_POINT ,
			SpriteTextureMapping methodX = STM_FILL , 
			SpriteTextureMapping methodY = STM_FILL );

		void  setRectColor      ( unsigned id , Color4ub const& color );
		void  setRectTextureSlot( unsigned id , unsigned slot );
		void  removeRectArea    ( unsigned id );


	protected:
		void  renderRectUnits( Matrix4 const& trans );

		struct SprVertex
		{
			Vector3  pos;
			float    tex[2];
			enum     { FVF = D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0) , };
		};

		struct RectUnit 
		{
			bool             beUsed;
			int              width;
			int              height;
			Color4ub         color;
			int              curSlot;
			FilterMode       filter;
			std::vector< Texture::RefPtr >  texSlot;
			VertexBuffer::RefPtr buffer;
		};

		void  renderImpl( Matrix4 const& trans );
		int   fetchEmptyRectUnit();
		bool  setupRectUnit( RectUnit& unit , 
			int ox , int oy , int w, int h , float depth ,
			Texture* texture[] , int nTex , 
			Color4ub const* color ,FilterMode filter, 
			SpriteTextureMapping methodX , SpriteTextureMapping methodY);

		typedef std::vector< RectUnit > RectUnitVector;
		RectUnitVector     mRectUnits;

	};

	DEFINE_ENTITY_TYPE( Sprite  , ET_SPRITE )

};//namespace CFly
#endif // CFSprite_h__
