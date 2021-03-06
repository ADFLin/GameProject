#include "RenderEngine.h"

#include "RenderSystem.h"
#include "Level.h"
#include "Light.h"
#include "ObjectRenderer.h"
#include "Block.h"

#include "RHI/ShaderManager.h"
#include "RHI/RHICommand.h"

#include "RHI/OpenGLCommon.h"
using namespace Render;

#include <algorithm>

bool gUseGroupRender = true;
bool gUseMRT = true;


IMPLEMENT_SHADER_PROGRAM(QBasePassBaseProgram);

RenderEngine::RenderEngine()
	:mAllocator( 512 )
{

}

bool RenderEngine::init( int width , int height )
{
	mAmbientLight = Vec3f(0.1f, 0.1f, 0.1f);
	//mAmbientLight = Vec3f( 0 , 0 , 0 );
	//mAmbientLight = Vec3f(0.3f, 0.3f, 0.3f);

	mFrameWidth  = width;
	mFrameHeight = height;

	if ( !setupFBO( width , height ) )
		return false;


	VERIFY_RETURN_FALSE( mProgBasePass = ShaderManager::Get().getGlobalShaderT< QBasePassBaseProgram >() );
	VERIFY_RETURN_FALSE( ShaderManager::Get().loadSimple( mShaderLighting ,"LightVS", "LightFS" ));
	VERIFY_RETURN_FALSE( ShaderManager::Get().loadSimple( mShaderScene[ RM_ALL ] , "SceneVS", "SceneFS" ));
	VERIFY_RETURN_FALSE( ShaderManager::Get().loadSimple( mShaderScene[ RM_GEOMETRY  ] , "SceneVS", "SceneGeometryFS" ));
	VERIFY_RETURN_FALSE( ShaderManager::Get().loadSimple( mShaderScene[ RM_LINGHTING ] , "SceneVS", "SceneLightingFS" ));

	return true;
}

void RenderEngine::cleanup()
{
	glDeleteFramebuffers(1,&mFBO);
	glDeleteFramebuffers(1,&mRBODepth );
}

bool RenderEngine::setupFBO( int width , int height )
{

#if QA_USE_RHI
	mTexLightmap  = RHICreateTexture2D(ETexture::FloatRGBA, width, height);
	mTexGeometry  = RHICreateTexture2D(ETexture::FloatRGBA, width, height);
	mTexNormalMap = RHICreateTexture2D(ETexture::RGB10A2, width, height);
	mDeferredFrameBuffer = RHICreateFrameBuffer();
	mDeferredFrameBuffer->setTexture(0, *mTexLightmap);
	mDeferredFrameBuffer->setTexture(1, *mTexGeometry);
	mDeferredFrameBuffer->setTexture(2, *mTexNormalMap);
#else
	glGenFramebuffers(1, &mFBO);


	mTexLightmap = RHICreateTexture2D(ETexture::RGBA8, width, height);
	Render::OpenGLCast::To(mTexLightmap)->bind();
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	Render::OpenGLCast::To(mTexLightmap)->unbind();


	mTexNormalMap = RHICreateTexture2D(ETexture::RGBA8, width, height);
	Render::OpenGLCast::To(mTexNormalMap)->bind();
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	Render::OpenGLCast::To(mTexNormalMap)->unbind();

	mTexGeometry = RHICreateTexture2D(ETexture::RGBA8, width, height);
	Render::OpenGLCast::To(mTexGeometry)->bind();
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	Render::OpenGLCast::To(mTexGeometry)->unbind();

	glGenRenderbuffers(1, &mRBODepth);
	glBindRenderbuffer(GL_RENDERBUFFER, mRBODepth);
	//glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8 , width , height);  
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);


#endif


	return true;
}

void RenderEngine::renderScene( RenderParam& param )
{
	mAllocator.clearFrame();

	param.renderWidth  = mFrameWidth * param.scaleFactor;
	param.renderHeight = mFrameHeight * param.scaleFactor;

#if QA_USE_RHI
	mDrawer.mBaseTransform = OrthoMatrix(0, param.renderWidth, param.renderHeight, 0, 0, 1);
#else
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0 , param.renderWidth , param.renderHeight  ,0,0,1);
	glMatrixMode( GL_MODELVIEW );
#endif
	

	TileRange& renderRange = param.terrainRange;

	renderRange.xMin = int( param.camera->getPos().x/BLOCK_SIZE )-1;
	renderRange.yMin = int( param.camera->getPos().y/BLOCK_SIZE )-1;
	renderRange.xMax = renderRange.xMin + int( param.renderWidth / BLOCK_SIZE ) + 3;
	renderRange.yMax = renderRange.yMin + int( param.renderHeight / BLOCK_SIZE ) + 3;

	TileMap& terrain = param.level->getTerrain();

	renderRange.xMin = Math::Clamp( renderRange.xMin , 0 , terrain.getSizeX() );
	renderRange.xMax = Math::Clamp( renderRange.xMax , 0 , terrain.getSizeX() );
	renderRange.yMin = Math::Clamp( renderRange.yMin , 0 , terrain.getSizeY() );
	renderRange.yMax = Math::Clamp( renderRange.yMax , 0 , terrain.getSizeY() );

	if ( gUseGroupRender )
		updateRenderGroup( param );

#if QA_USE_RHI

#else
	switch( param.mode )
	{
	case RM_ALL:
		renderGeometryFBO( param );
		renderLightingFBO( param );	
		break;
	case RM_GEOMETRY:
		renderGeometryFBO( param );
		break;
	case RM_LINGHTING:
		renderLightingFBO( param );
		break;
	case RM_NORMAL_MAP:
		renderNormalFBO( param );

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode( GL_MODELVIEW );

		glEnable(GL_TEXTURE_2D);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, OpenGLCast::GetHandle(mTexNormalMap) );

		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 1.0); glVertex2f(0.0, 0.0);
		glTexCoord2f(1.0, 1.0); glVertex2f( mFrameWidth , 0.0);
		glTexCoord2f(1.0, 0.0); glVertex2f( mFrameWidth , mFrameHeight );
		glTexCoord2f(0.0, 0.0); glVertex2f(0.0 , mFrameHeight );
		glEnd();

		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);
		return;
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );

	renderSceneFinal( param );
#endif
}

void RenderEngine::renderSceneFinal( RenderParam& param )
{
	RHICommandList& commandList = RHICommandList::GetImmediateList();
	ShaderProgram& shader = mShaderScene[ param.mode ];

	RHISetShaderProgram(commandList, shader.getRHIResource());

	glEnable(GL_TEXTURE_2D);
	shader.setTexture(commandList, SHADER_PARAM(texGeometry) , *mTexGeometry );
	shader.setTexture(commandList, SHADER_PARAM(texLightmap) , *mTexLightmap );
	shader.setParam(commandList, SHADER_PARAM(ambientLight) , mAmbientLight );

	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 1.0); glVertex2f(0.0, 0.0);
	glTexCoord2f(1.0, 1.0); glVertex2f( mFrameWidth , 0.0);
	glTexCoord2f(1.0, 0.0); glVertex2f( mFrameWidth , mFrameHeight );
	glTexCoord2f(0.0, 0.0); glVertex2f(0.0 , mFrameHeight );
	glEnd();

	RHISetShaderProgram(commandList, nullptr);

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);
}

void RenderEngine::renderTerrain( Level* level , TileRange const& range )
{
	TileMap& terrain = level->getTerrain();
	for(int i = range.xMin; i < range.xMax ; ++i )
	for(int j = range.yMin; j < range.yMax; ++j )
	{
		Tile const& tile = terrain.getData( i , j );
		Block::Get( tile.id )->render( tile );
	}
}

void RenderEngine::renderTerrainNormal( Level* level , TileRange const& range )
{
	TileMap& terrain = level->getTerrain();
	for(int i = range.xMin; i < range.xMax ; ++i )
	for(int j = range.yMin; j < range.yMax; ++j )
	{		
		Tile const& tile = terrain.getData( i , j );
		Block::Get( tile.id )->renderNormal( tile );
	}
}

void RenderEngine::renderTerrainGlow( Level* level , TileRange const& range )
{
	TileMap& terrain = level->getTerrain();
	for(int i = range.xMin; i < range.xMax ; ++i )
	for(int j = range.yMin; j < range.yMax; ++j )
	{
		Tile const& tile = terrain.getData( i , j );
		Block::Get( tile.id )->renderGlow( tile );
	}
}


void RenderEngine::renderLight( RenderParam& param , Vec2f const& lightPos , Light* light )
{
	RHICommandList& commandList = RHICommandList::GetImmediateList();

	Vec2f posLight = lightPos - param.camera->getPos();

	RHISetShaderProgram(commandList, mShaderLighting.getRHIResource());
	mShaderLighting.setTexture(commandList, SHADER_PARAM( texNormalMap ) , *mTexNormalMap );
	//mShaderLighting->setParam( SHADER_PARAM(frameHeight), mFrameHeight );
	//mShaderLighting->setParam( SHADER_PARAM(scaleFactor) , param.scaleFactor );
	mShaderLighting.setParam(commandList, SHADER_PARAM(posLight) , posLight );
	setupLightShaderParam( mShaderLighting , light );

#if 1
	Vec2f halfRange = param.scaleFactor * Vec2f( light->radius , light->radius ); 

	Vec2f minRender = posLight - halfRange;
	Vec2f maxRender = posLight + halfRange;

	Vec2f minTex , maxTex;
	minTex.x = minRender.x / param.renderWidth;
	maxTex.x = maxRender.x / param.renderWidth;
	minTex.y = 1 - minRender.y / param.renderHeight;
	maxTex.y = 1 - maxRender.y / param.renderHeight;

	glColor3f(1,1,1);

	glBegin(GL_QUADS);
	glTexCoord2f(minTex.x,minTex.y); glVertex2f( minRender.x , minRender.y );
	glTexCoord2f(maxTex.x,minTex.y); glVertex2f( maxRender.x , minRender.y );
	glTexCoord2f(maxTex.x,maxTex.y); glVertex2f( maxRender.x , maxRender.y );
	glTexCoord2f(minTex.x,maxTex.y); glVertex2f( minRender.x , maxRender.y );
	glEnd();	
#else
	glColor3f(1,1,1);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 1.0); glVertex2f( 0.0, 0.0);
	glTexCoord2f(1.0, 1.0); glVertex2f( param.renderWidth , 0.0);
	glTexCoord2f(1.0, 0.0); glVertex2f( param.renderWidth , param.renderHeight );
	glTexCoord2f(0.0, 0.0); glVertex2f( 0.0 , param.renderHeight );
	glEnd();	
#endif

	RHISetShaderProgram(commandList, nullptr);
	glActiveTexture(GL_TEXTURE0);
}

void RenderEngine::setupLightShaderParam(ShaderProgram& shader , Light* light )
{
	RHICommandList& commandList = RHICommandList::GetImmediateList();
	shader.setParam(commandList, SHADER_PARAM(colorLight) , light->color );
	shader.setParam(commandList, SHADER_PARAM(dir) , light->dir );
	shader.setParam(commandList, SHADER_PARAM(angle) , light->angle );
	shader.setParam(commandList, SHADER_PARAM(radius), light->radius );
	shader.setParam(commandList, SHADER_PARAM(intensity) ,light->intensity );
	shader.setParam(commandList, SHADER_PARAM(isExplosion) , ( light->isExplosion ) ? 1 : 0 );
}

void RenderEngine::renderGeometryFBO( RenderParam& param )
{
	glBindFramebuffer(GL_FRAMEBUFFER ,mFBO);		
	glFramebufferTexture2D(GL_FRAMEBUFFER , GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, OpenGLCast::GetHandle(mTexGeometry), 0);
	GLenum DrawBuffers[] =
	{
		GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 , GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4,
		GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7 , GL_COLOR_ATTACHMENT8, GL_COLOR_ATTACHMENT9
	};
	glDrawBuffers(1, DrawBuffers);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
	glClearColor(0.0, 0.0, 0.0, 1.0f);
	glLoadIdentity();	

	glPushMatrix();
	//Sprite(Vec2(0,0),Vec2(igra->DajRW()->getSize().x, igra->DajRW()->getSize().y),mt->DajTexturu(1)->id);
	glTranslatef( - param.camera->getPos().x, - param.camera->getPos().y, 0);			

	renderTerrain( param.level , param.terrainRange );
	renderObjects( RP_DIFFUSE , param.level );

	glPopMatrix();

	glBindFramebuffer(GL_FRAMEBUFFER ,0);
}

void RenderEngine::renderNormalFBO( RenderParam& param )
{
	glBindFramebuffer( GL_FRAMEBUFFER ,mFBO);		
	glFramebufferTexture2D(GL_FRAMEBUFFER , GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, OpenGLCast::GetHandle(mTexNormalMap), 0 );
	GLenum DrawBuffers[] =
	{
		GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 , GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4,
		GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7 , GL_COLOR_ATTACHMENT8, GL_COLOR_ATTACHMENT9
	};
	glDrawBuffers(1, DrawBuffers);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
	glClearColor(0.0, 0.0, 0.0, 1.0f);
	glLoadIdentity();

	glPushMatrix();	

	glTranslatef( - param.camera->getPos().x, - param.camera->getPos().y, 0);

	//Sprite(Vec2(0,0),Vec2(igra->DajRW()->getSize().x, igra->DajRW()->getSize().y),mt->DajTexturu(1)->id);
	renderTerrainNormal( param.level , param.terrainRange );

	renderObjects( RP_NORMAL , param.level );

	glPopMatrix();
	glBindFramebuffer(GL_FRAMEBUFFER ,0);
}



void RenderEngine::renderBasePass(RenderParam& param)
{

}

void RenderEngine::renderLightingFBO( RenderParam& param )
{
	renderNormalFBO( param );

	glBindFramebuffer(GL_FRAMEBUFFER ,mFBO);		
	glFramebufferTexture2D(GL_FRAMEBUFFER , GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, OpenGLCast::GetHandle(mTexLightmap), 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mRBODepth ); 
	GLenum DrawBuffers[] =
	{
		GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 , GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4,
		GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7 , GL_COLOR_ATTACHMENT8, GL_COLOR_ATTACHMENT9
	};
	glDrawBuffers(1, DrawBuffers);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glClearColor( mAmbientLight.x , mAmbientLight.y , mAmbientLight.z , 1.0f);
	
	glLoadIdentity();	

	glEnable(GL_BLEND);	
	glBlendFunc(GL_ONE,GL_ONE);

	//RENDERIRA SE SVE GLOW

	Object* camera = param.camera;


	glPushMatrix();
	glTranslatef(-camera->getPos().x, -camera->getPos().y, 0);

	renderTerrainGlow( param.level , param.terrainRange );
	renderObjects( RP_GLOW , param.level );

	glPopMatrix();

	float w = param.renderWidth;
	float h = param.renderHeight;

	ShaderProgram& shader = mShaderLighting;

	RenderLightList& lights = param.level->getRenderLights();
	TileMap& terrain = param.level->getTerrain();

	for( RenderLightList::iterator iter = lights.begin() , itEnd = lights.end();
		iter != itEnd ; ++iter )
	{		
		Light* light = *iter;
		Vec2f const& lightPos = light->cachePos;

		if( lightPos.x + light->radius < camera->getPos().x ||
			lightPos.x - light->radius > camera->getPos().x + w ||
			lightPos.y + light->radius < camera->getPos().y || 
			lightPos.y - light->radius > camera->getPos().y + h )
			continue;

		if ( light->drawShadow )
		{
			glEnable( GL_STENCIL_TEST );
			glClear(GL_STENCIL_BUFFER_BIT);

#if 1
			glColorMask(false, false, false, false);
#else
			glColor3f( 0.1 , 0.1 , 0.1 );
#endif
			glStencilFunc(GL_ALWAYS, 1, 1);
			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

			
			glPushMatrix();
			glTranslatef(-camera->getPos().x, -camera->getPos().y, 0);

			TileRange range = param.terrainRange;

			int tx = int( lightPos.x / BLOCK_SIZE );
			int ty = int( lightPos.y / BLOCK_SIZE );

			if ( tx < range.xMin )
				range.xMin = tx;
			else if ( tx > range.xMax )
				range.yMax = tx + 1;

			if ( ty < range.yMin )
				range.yMin = ty;
			else if ( ty > range.yMax )
				range.yMax = ty + 1;

			range.xMin = Math::Clamp( range.xMin , 0 , terrain.getSizeX() );
			range.xMax = Math::Clamp( range.xMax , 0 , terrain.getSizeX() );
			range.yMin = Math::Clamp( range.yMin , 0 , terrain.getSizeY() );
			range.yMax = Math::Clamp( range.yMax , 0 , terrain.getSizeY() );

			renderTerrainShadow( param.level , lightPos , light , range );

#if 1
			glStencilFunc(GL_ALWAYS, 0, 1);
			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

			for(int i = param.terrainRange.xMin; i < param.terrainRange.xMax; ++i )
			for(int j = param.terrainRange.yMin; j < param.terrainRange.yMax; ++j )
			{
				Tile const& tile = terrain.getData( i , j );
				Block* block = Block::Get( tile.id );
				if ( !block->checkFlag( BF_CAST_SHADOW ) )
					continue;

				block->renderNoTexture( tile );
			}
#endif

			glPopMatrix();

			glStencilFunc(GL_EQUAL, 0, 1);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			glColorMask(true, true, true, true);
		}
		else
		{
			glDisable( GL_STENCIL_TEST );
		}

		renderLight( param , lightPos , light );
	}

	glDisable( GL_STENCIL_TEST );
	glDisable(GL_BLEND);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0 ); 
	glBindFramebuffer( GL_FRAMEBUFFER ,0);
}

static int const offsetX[4] = {-1,0,1,0};
static int const offsetY[4] = { 0,-1,0,1};
static Vec2f const tileVertex[4] = { Vec2f(0,0) , Vec2f(BLOCK_SIZE,0) , Vec2f(BLOCK_SIZE,BLOCK_SIZE ) , Vec2f(0,BLOCK_SIZE)   };
static Vec2f const tileNormal[4] = { Vec2f(-1,0) , Vec2f(0,-1) , Vec2f(1,0) , Vec2f(0,1) };

void RenderEngine::renderTerrainShadow( Level* level , Vec2f const& lightPos , Light* light , TileRange const& range )
{
	TileMap& terrain = level->getTerrain();

#if 1
	Vec2i tpLight = Vec2i( Math::FloorToInt( lightPos.x / BLOCK_SIZE ) , Math::FloorToInt( lightPos.y / BLOCK_SIZE ) );
	if ( terrain.checkRange( tpLight.x , tpLight.y ) )
	{
		Tile const& tile = terrain.getData( tpLight.x , tpLight.y );
		Block* block = Block::Get( tile.id );
		if ( block->checkFlag( BF_CAST_SHADOW ) )
		{
			if ( !block->checkFlag( BF_NONSIMPLE ) )
				return;

			Rect bBox;
			bBox.min = lightPos - Vec2f(0.1,0.1);
			bBox.max = lightPos + Vec2f(0.1,0.1);
			if ( block->testIntersect( tile , bBox ) )
				return;
		}
	}
#endif
	
	
	for(int i = range.xMin; i < range.xMax ; ++i )
	{
		for(int j = range.yMin; j < range.yMax; ++j )
		{
			Tile const& tile = terrain.getData( i , j );
			Block* block = Block::Get( tile.id );

			if ( !block->checkFlag( BF_CAST_SHADOW ) )
				continue;

			if ( block->checkFlag( BF_NONSIMPLE ) )
			{
				block->renderShadow( tile , lightPos , *light );
			}
			else
			{
				Vec2f tileOffset = tile.pos - lightPos;

				for( int idxCur = 0 , idxPrev = 3; idxCur < 4; idxPrev = idxCur , ++idxCur )
				{
					int nx = i + offsetX[idxCur];
					int ny = j + offsetY[idxCur];

#if 1
					if ( terrain.checkRange( nx , ny ) )
					{
						Block* block = Block::Get( terrain.getData( nx , ny ).id );

						if ( !block->checkFlag( BF_NONSIMPLE ) && 
							block->checkFlag( BF_CAST_SHADOW ) )
							continue;

					}
#endif
					Vec2f offsetCur  = tileVertex[ idxCur ]  + tileOffset;

					if ( offsetCur.dot( tileNormal[ idxCur ] ) >= 0 )
						continue;
	
					Vec2f offsetPrev = tileVertex[ idxPrev ] + tileOffset;

					Vec2f const& cur  = tile.pos + tileVertex[ idxCur ];
					Vec2f const& prev = tile.pos + tileVertex[ idxPrev ];

					Vec2f v1 = lightPos + 5000 * offsetPrev;
					Vec2f v2 = lightPos + 5000 * offsetCur;

					glBegin( GL_QUADS );
					glVertex2f( prev.x , prev.y );
					glVertex2f( v1.x , v1.y );
					glVertex2f( v2.x , v2.y  );
					glVertex2f( cur.x , cur.y );
					glEnd();
	
				}
			}
		}
	}
}

void RenderEngine::renderObjects( RenderPass pass , Level* level )
{
	if ( gUseGroupRender )
	{
		for( RenderGroupVec::iterator iter = mRenderGroups.begin() , itEnd = mRenderGroups.end();
			iter != itEnd ; ++iter )
		{
			RenderGroup* group = *iter;
			group->renderer->renderGroup( pass , group->numObject , group->objectLists );
		}
	}
	else
	{
		level->renderObjects( pass );
	}
}



void RenderEngine::updateRenderGroup( RenderParam& param )
{

	mRenderGroups.clear();
	mBodyList.clear();

	Rect bBox;
	bBox.min = param.camera->getPos();
	bBox.max = param.camera->getPos() + Vec2f( param.renderWidth  , param.renderHeight );
	param.level->getColManager().findBody( bBox , COL_RENDER , mBodyList );

	struct GroupCompFun
	{
		bool operator()( RenderGroup* a , RenderGroup* b ) const 
		{
			if ( a->order > b->order )
				return false;
			if ( a->order == b->order )
				return a->renderer < b->renderer;
			return true;
		}
	};


	for( ColBodyVec::iterator iter = mBodyList.begin() , itEnd = mBodyList.end();
		 iter != itEnd ; ++iter )
	{
		LevelObject* obj = (*iter)->getClient();
		IObjectRenderer* renderer = obj->getRenderer();

		RenderGroup testGroup;
		testGroup.renderer = renderer;
		testGroup.order    = renderer->getOrder();
		RenderGroupVec::iterator iterGroup = std::lower_bound( mRenderGroups.begin() , mRenderGroups.end() , &testGroup , GroupCompFun() );
		
		RenderGroup* group;
		if ( iterGroup != mRenderGroups.end() && (*iterGroup)->renderer == renderer )
		{
			group = *iterGroup;
			obj->renderLink = group->objectLists;
			group->objectLists = obj;
			group->numObject += 1;
		}
		else
		{
			group = new ( mAllocator ) RenderGroup;

			group->order = renderer->getOrder();
			group->renderer = renderer;
			group->objectLists = obj;
			group->numObject = 1;
			obj->renderLink = NULL;

			mRenderGroups.insert( iterGroup , group );
		}
	}

}
