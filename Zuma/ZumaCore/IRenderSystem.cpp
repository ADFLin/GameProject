#include "ZumaPCH.h"
#include "IRenderSystem.h"

#include "ZLevel.h"
#include "ZLevelManager.h"
#include "ZFont.h"
#include "ResID.h"

#include "GIFLoader.h"

namespace Zuma
{
	IRenderSystem::IRenderSystem()
	{
		mSrcBlend = BLEND_ONE;
		mDstBlend = BLEND_ONE;
		mBlendEnable = false;
	}

	IRenderSystem::RenderNode IRenderSystem::addRenderable( IRenderable* obj )
	{
		RNODE node;
		node.obj    = obj;
		node.effect = NULL;

		RenderNode iter = std::lower_bound( mRenderList.begin() , mRenderList.end() , node , NodeCmp() );
		mRenderList.insert( iter , node );
#ifdef _DEBUG
		obj->mbManaged = true;
		return --iter;
#endif
		return --iter;
	}

	bool IRenderSystem::removeObj( IRenderable* obj )
	{
		RenderNode node;

		if ( findRenderNode( obj , node ) )
		{
			mRenderList.erase( node );
			return true;
		}
		return false;
	}



	bool IRenderSystem::findRenderNode( IRenderable* obj , RenderNode& node )
	{
		node =  std::find_if( mRenderList.begin() , mRenderList.end() , FindObj(obj) );

		if ( node != mRenderList.end() )
			return true;

		return false;
	}

	IRenderable* IRenderSystem::getRenderable( RenderNode node )
	{
		if ( node == mRenderList.end() )
			return NULL;
		return node->obj;
	}

	void IRenderSystem::removeEffect( RenderNode node ,IRenderEffect* effect /*= NULL */ )
	{
		if ( node == mRenderList.end() )
			return;

		IRenderEffect* e = node->effect;

		while ( e )
		{
			if ( effect == NULL || effect == e )
			{
				e = e->nextEffect;
				node->effect = e;
			}
		}
		if ( e == NULL )
			return ;

		IRenderEffect* next =  e->nextEffect;
		while ( next )
		{
			if ( effect == NULL || effect == next )
			{
				next =  next->nextEffect;
				e->nextEffect = next;
			}
			else
			{
				e = next;
				next = next->nextEffect;
			}
		}
	}

	void IRenderSystem::renderNode( RNODE& node )
	{
		IRenderEffect* effect = node.effect;
		while( effect )
		{
			effect->preRenderEffect( *this );
			effect = effect->nextEffect;
		}
		node.obj->onRender( *this );

		effect = node.effect;
		while( effect )
		{
			effect->postRenderEffect( *this );
			effect = effect->nextEffect;
		}
	}

	bool IRenderSystem::beginRender()
	{
		if ( !prevRender() ) 
			return false;

		mNextRenderNode = mRenderList.begin();
		mCurOrder = RO_MIN_ORDER - 1;
		setCurOrder( RO_MIN_ORDER );

		return true;
	}

	void IRenderSystem::endRender()
	{
		for ( ; mNextRenderNode != mRenderList.end(); ++mNextRenderNode )
		{
			RNODE& node = *mNextRenderNode;
			renderNode( node );
		}

		postRender();
	}

	void IRenderSystem::setCurOrder( int order )
	{
		assert( order > mCurOrder );

		for ( ; mNextRenderNode != mRenderList.end(); ++mNextRenderNode )
		{
			RNODE& node = *mNextRenderNode;

			if ( node.obj->getOrder() > order )
				break;

			renderNode( node );
		}

		mCurOrder = order;
	}

	bool IRenderSystem::setupBlend( bool beOwned , unsigned flag )
	{
		bool beE = ( flag & TBF_ENABLE_ALPHA_BLEND ) ||
			( beOwned && ( flag & TBF_DONT_USE_TEX_ALPHA ) == 0 );

		enableBlendImpl( beE );

		if ( beE )
		{
			if ( ( flag & TBF_USE_BLEND_PARAM ) )
				setupBlendFun( mSrcBlend , mDstBlend );
			else
				setupBlendFun( BLEND_SRCALPHA , BLEND_INV_SRCALPHA );
		}
		return beE;
	}

	void IRenderSystem::setBlendFun( BlendEnum src , BlendEnum dst )
	{
		mSrcBlend = src;
		mDstBlend = dst;

		if ( mBlendEnable )
		{
			setupBlendFun( mSrcBlend , mDstBlend );
		}
	}

	void IRenderSystem::enableBlend( bool beE )
	{
		if ( mBlendEnable == beE )
			return;

		mBlendEnable = beE;
		enableBlendImpl( beE );

		if ( beE )
		{
			setupBlendFun( mSrcBlend , mDstBlend );
		}
	}

	ZFont* IRenderSystem::createFontRes( FontResInfo& info )
	{
		ZFontLoader loader;
		if ( !loader.load( info.path.c_str() ) )
			return NULL;

		ZFont* font = new ZFont( loader );
		return font;
	}

	ResBase* IRenderSystem::createResource( ResID id , ResInfo& info )
	{
		switch ( info.type )
		{
		case RES_IMAGE: 
			return createTextureRes( static_cast< ImageResInfo& >( info ) );
		case RES_FONT:
			return createFontRes( static_cast< FontResInfo& >( info ));
		}
		return NULL;
	}


	ITexture2D* IRenderSystem::getTexture( ResID id )
	{
		ResBase* res = getResource( id );
		assert( res == NULL || res->getType() == RES_IMAGE );
		return static_cast< ITexture2D* >( res );
	}

	ZFont* IRenderSystem::getFontRes( ResID id )
	{
		ResBase* res = getResource( id );
		assert( res == NULL || res->getType() == RES_FONT );
		return static_cast< ZFont* >( res );
	}

	bool TempRawData::GIFCreateAlpha( GIFImageInfo const& info , void* userData )
	{
		TempRawData& data = *reinterpret_cast< TempRawData* >( userData );

		int len = info.imgWidth * info.imgHeight;

		unsigned char* buffer = new unsigned char[ 4 * len ];
		unsigned char* temp = buffer;

		for( int j = 0 ; j < info.imgHeight ; ++j )
		{
			int idx = info.bytesPerRow * j;

			for( int i = 0 ; i < info.imgWidth  ; ++i )
			{
				COLOR& color = info.palette[ (unsigned char )info.raster[idx] ];
				*(temp++) = 255;
				*(temp++) = 255;
				*(temp++) = 255;
				*(temp++) = color.r;
				++idx;
			}
		}

		data.w = info.imgWidth;
		data.h = info.imgHeight;
		data.buffer = buffer;

		return false;
	}

	bool TempRawData::GIFFillAlpha( GIFImageInfo const& info , void* userData )
	{
		TempRawData& data = *reinterpret_cast< TempRawData* >( userData );

		assert( info.frameWidth == data.w );

		unsigned char* temp = data.buffer;
		for( int j = 0 ; j < data.h ; ++j )
		{
			int idx = info.bytesPerRow * ( j % info.imgHeight );
			for( int i = 0 ; i < data.w  ; ++i )
			{
				COLOR& color = info.palette[ (unsigned char )info.raster[idx] ];
				temp[3] = color.r;
				++idx;
				temp += 4;
			}
		}
		return false;
	}

	bool TempRawData::GIFCreateRGB( GIFImageInfo const& info , void* userData )
	{
		TempRawData& data = *reinterpret_cast< TempRawData* >( userData );

		int len = info.imgWidth * info.imgHeight;

		unsigned char* buffer = new unsigned char[ 4 * len ];
		unsigned char* temp = buffer;

		for( int j = 0 ; j < info.imgHeight ; ++j )
		{
			int idx = info.bytesPerRow * j;

			for( int i = 0 ; i < info.imgWidth  ; ++i )
			{
				COLOR& color = info.palette[ (unsigned char)info.raster[idx] ];
				*(temp++) = color.r;
				*(temp++) = color.g;
				*(temp++) = color.b;
				*(temp++) = 255;
				++idx;
			}
		}

		data.w = info.imgWidth;
		data.h = info.imgHeight;
		data.buffer = buffer;

		return false;
	}

}//namespace Zuma
