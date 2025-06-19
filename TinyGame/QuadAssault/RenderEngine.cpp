#include "RenderEngine.h"

#include "RenderSystem.h"
#include "Texture.h"
#include "TextureManager.h"

#include "Level.h"
#include "Light.h"
#include "ObjectRenderer.h"
#include "Block.h"

#include "RHI/ShaderManager.h"
#include "RHI/RHICommand.h"

#include "RHI/OpenGLCommon.h"
#include "RHI/OpenGLShaderCommon.h"
#include "RHI/RHIGlobalResource.h"
#include "RHI/DrawUtility.h"
#include "RenderUtility.h"

#include "RenderDebug.h"

using namespace Render;

#include <algorithm>
#include "Math/GeometryPrimitive.h"
#include "Renderer/BatchedRender2D.h"

static int const offsetX[4] = { -1,0,1,0 };
static int const offsetY[4] = { 0,-1,0,1 };
static Vec2f const tileVertex[4] = { Vec2f(0,0) , Vec2f(BLOCK_SIZE,0) , Vec2f(BLOCK_SIZE,BLOCK_SIZE) , Vec2f(0,BLOCK_SIZE) };
static Vec2f const tileNormal[4] = { Vec2f(-1,0) , Vec2f(0,-1) , Vec2f(1,0) , Vec2f(0,1) };

bool gUseGroupRender = true;
bool gUseMRT = true;


IMPLEMENT_SHADER_PROGRAM(QBasePassProgram);
IMPLEMENT_SHADER_PROGRAM(QGlowProgram);
IMPLEMENT_SHADER_PROGRAM(QShadowProgram);
IMPLEMENT_SHADER_PROGRAM(QLightingProgram);
IMPLEMENT_SHADER_PROGRAM(QSceneProgram);

RenderEngine::RenderEngine()
	:mAllocator( 512 )
{

}

bool RenderEngine::init( int width , int height )
{
	mAmbientLight = Color3f(0.1f, 0.1f, 0.1f);
	mAmbientLight = Color3f( 0 , 0 , 0 );
	//mAmbientLight = Color3f(0.3f, 0.3f, 0.3f);

	mFrameWidth  = width;
	mFrameHeight = height;

	if ( !setupFrameBuffer( width , height ) )
		return false;

	mLightBuffer.initializeResource(1);
	mViewBuffer.initializeResource(1);

	VERIFY_RETURN_FALSE(mProgBasePass = ShaderManager::Get().getGlobalShaderT< QBasePassProgram >());
	VERIFY_RETURN_FALSE(mProgGlow = ShaderManager::Get().getGlobalShaderT< QGlowProgram >());
	VERIFY_RETURN_FALSE(mProgShadow = ShaderManager::Get().getGlobalShaderT< QShadowProgram >());
	VERIFY_RETURN_FALSE(mProgLighting = ShaderManager::Get().getGlobalShaderT< QLightingProgram >());

	for (int mode = 0; mode < NUM_RENDER_MODE; ++mode)
	{
		QSceneProgram::PermutationDomain domain;
		domain.set<QSceneProgram::RenderMode>(mode);
		VERIFY_RETURN_FALSE(mProgScenes[mode] = ShaderManager::Get().getGlobalShaderT< QSceneProgram >(domain));
	}
	ShaderHelper::Get().init();
	return true;
}

void RenderEngine::cleanup()
{

}

bool RenderEngine::setupFrameBuffer( int width , int height )
{
	mTexLightmap  = RHICreateTexture2D(TextureDesc::Type2D(ETexture::FloatRGBA, width, height).Flags(TCF_CreateSRV | TCF_RenderTarget | TCF_DefalutValue));
	mTexLightmap->setDebugName("TexLightmap");
	mTexGeometry  = RHICreateTexture2D(TextureDesc::Type2D(ETexture::FloatRGBA, width, height).Flags(TCF_CreateSRV | TCF_RenderTarget | TCF_DefalutValue));
	mTexGeometry->setDebugName("TexGeometry");
	mTexNormalMap = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGB10A2, width, height).Flags(TCF_CreateSRV | TCF_RenderTarget | TCF_DefalutValue));
	mTexNormalMap->setDebugName("TexNormalMap");

	mTexDepth     = RHICreateTexture2D(TextureDesc::Type2D(ETexture::D24S8, width, height).Flags(TCF_None));
	mDeferredFrameBuffer = RHICreateFrameBuffer();
	mDeferredFrameBuffer->setTexture(0, *mTexGeometry);
	mDeferredFrameBuffer->setTexture(1, *mTexNormalMap);

	mLightingFrameBuffer = RHICreateFrameBuffer();
	mLightingFrameBuffer->setTexture(0, *mTexLightmap);
	mLightingFrameBuffer->setDepth(*mTexDepth);

	GTextureShowManager.registerTexture("TexLightmap", mTexLightmap);
	GTextureShowManager.registerTexture("TexGeom", mTexGeometry);
	GTextureShowManager.registerTexture("TexNormal", mTexNormalMap);

	return true;
}

void RenderEngine::renderScene(RenderConfig const& config)
{
	RenderParam param;
	static_cast< RenderConfig& >( param ) = config;
	mAllocator.clearFrame();

	param.renderWidth  = mFrameWidth * param.scaleFactor;
	param.renderHeight = mFrameHeight * param.scaleFactor;
	param.worldToView = RenderTransform2D::LookAt(Vector2(mFrameWidth, mFrameHeight),
		param.camera->getPos() + (0.5f * param.scaleFactor) * Vector2(mFrameWidth, mFrameHeight), Vector2(0, -1), mFrameWidth / param.renderWidth);
	param.worldToClipRHI = AdjustProjectionMatrixForRHI(param.worldToView.toMatrix4() * OrthoMatrix(0, param.renderWidth, param.renderHeight, 0, 0, 1));
	ViewParams* viewParams = mViewBuffer.lock();
	if (viewParams)
	{
		viewParams->rectPos = Vector2(0, 0);
		viewParams->sizeInv = Vector2(1.0 / mFrameWidth, 1.0 / mFrameHeight);
		mViewBuffer.unlock();
	}
	mDrawer.mBaseTransformRHI = param.worldToClipRHI;

	RHICommandList& commandList = RHICommandList::GetImmediateList();
	RHISetViewport(commandList, 0, 0, mFrameWidth, mFrameHeight);

	mDrawer.mStack.clear();
	mDrawer.mCommandList = &commandList;

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

	renderBasePass(commandList, param);
	renderLighting(commandList, param);
	RHISetFrameBuffer(commandList, nullptr);
	renderSceneFinal(commandList, param);
}

void RenderEngine::renderBasePass(RHICommandList& commandList, RenderParam& param)
{
	RHIResourceTransition(commandList, { mTexGeometry , mTexNormalMap }, EResourceTransition::RenderTarget);

	RHISetFrameBuffer(commandList, mDeferredFrameBuffer);
	LinearColor clearColors[] =
	{
		LinearColor(0,0,0,1),
		LinearColor(0,0,0,1),
		//LinearColor(0,0,0,1),
	};
	RHIClearRenderTargets(commandList, EClearBits::Color, clearColors, ARRAY_SIZE(clearColors));
	RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
	RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());
	RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

	RHISetShaderProgram(commandList, mProgBasePass->getRHI());

	mProgBasePass->setParam(commandList, SHADER_PARAM(WorldToClip), param.worldToClipRHI);
	mProgBasePass->setParam(commandList, SHADER_PARAM(LocalToWorld), Matrix4::Identity());
	mDrawer.mShader = mProgBasePass;

	visitTiles(param.level, param.terrainRange,
		[this](Tile const& tile)
		{
			Block::Get(tile.id)->renderBasePass(mDrawer, tile);
		}
	);
	renderObjects(RP_BASE_PASS, param.level);
	RHIResourceTransition(commandList, { mTexGeometry ,mTexNormalMap }, EResourceTransition::SRV);
}


void RenderEngine::renderLighting(RHICommandList& commandList, RenderParam& param )
{
	RHIResourceTransition(commandList, { mTexLightmap }, EResourceTransition::RenderTarget);

	RHISetFrameBuffer(commandList, mLightingFrameBuffer);
	RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(mAmbientLight, 1.0), 1);

	RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA, EBlend::One, EBlend::One>::GetRHI());

	RHISetShaderProgram(commandList, mProgGlow->getRHI());
	mProgGlow->setParam(commandList, SHADER_PARAM(XForm), param.worldToClipRHI);
	mDrawer.mShader = mProgGlow;
	visitTiles(param.level, param.terrainRange, 
		[this](Tile const& tile)
		{
			Block::Get(tile.id)->renderGlow(mDrawer, tile);
		}
	);
	renderObjects(RP_GLOW, param.level);

	Object* camera = param.camera;

	float w = param.renderWidth;
	float h = param.renderHeight;

	RenderLightList& lights = param.level->getRenderLights();
	TileMap& terrain = param.level->getTerrain();
	Vector2 screenCenter = param.worldToView.transformInvPosition(Vector2(mFrameWidth, mFrameHeight) / 2);
	Vector2 screenHalfSize = param.worldToView.transformInvVector(Vector2(mFrameWidth, mFrameHeight) / 2);
	for (Light* light : lights)
	{
		Vec2f const& lightPos = light->cachedPos;

		if (!Math::BoxCircleTest(screenCenter, screenHalfSize, lightPos, light->radius))
			continue;

		bool bShowShadowRender = false;
		if (light->drawShadow)
		{
			if (bShowShadowRender)
			{
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::One, EBlend::One, EBlend::Add >::GetRHI());
			}
			else
			{
				RHIClearRenderTargets(commandList, EClearBits::Stencil, nullptr, 0, 1.0, 0);

				RHISetDepthStencilState(commandList,
					TStaticDepthStencilState<
					true, ECompareFunc::Always,
					true, ECompareFunc::Always,
					EStencil::Keep, EStencil::Keep, EStencil::Replace, 0x1 >::GetRHI(), 0x1);

				RHISetBlendState(commandList, TStaticBlendState< CWM_None >::GetRHI());
			}

			renderTerrainShadow(commandList, param, light);

			RHISetDepthStencilState(commandList,
				TStaticDepthStencilState<
				true, ECompareFunc::Always,
				true, ECompareFunc::Equal,
				EStencil::Keep, EStencil::Keep, EStencil::Keep, 0x1
				>::GetRHI(), 0x0);
			RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None > ::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::One, EBlend::One >::GetRHI());
			renderLight(commandList, param, light);
		}
		else
		{
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			renderLight(commandList, param, light);
		}

	}

	RHIResourceTransition(commandList, { mTexLightmap }, EResourceTransition::SRV);
}

void RenderEngine::renderLight(RHICommandList& commandList, RenderParam& param , Light* light)
{
	RHISetShaderProgram(commandList, mProgLighting->getRHI());
	Vec2f const& lightPos = light->cachedPos;
	auto& samplerState = TStaticSamplerState<ESampler::Bilinear>::GetRHI();

	mProgLighting->setTexture(commandList, SHADER_PARAM(TexNormalMap), mTexNormalMap, SHADER_SAMPLER(TexNormalMap), samplerState);
	mProgLighting->setTexture(commandList, SHADER_PARAM(TexBaseColor), mTexGeometry, SHADER_SAMPLER(TexBaseColor), samplerState);
	mProgLighting->setParam(commandList, SHADER_PARAM(WorldToClip), param.worldToClipRHI);

	LightParams* lightParams = mLightBuffer.lock();
	if (lightParams)
	{
		lightParams->pos = lightPos;
		lightParams->radius = light->radius;
		lightParams->intensity = light->intensity;
		lightParams->isExplosion = (light->isExplosion) ? 1 : 0;
		lightParams->dir = light->dir;
		lightParams->angle = light->angle;
		lightParams->color = light->color;

		mLightBuffer.unlock();
	}

	SetStructuredUniformBuffer(commandList, *mProgLighting, mLightBuffer);
	SetStructuredUniformBuffer(commandList, *mProgLighting, mViewBuffer);

	Vector2 halfSize = Vector2(light->radius, light->radius);
	mDrawer.drawRect(lightPos - halfSize , 2 * halfSize);
}

void RenderEngine::renderTerrainShadow(RHICommandList& commandList, RenderParam& param, Light* light)
{
	TileMap& terrain = param.level->getTerrain();
	TileRange range = param.terrainRange;
	Vec2f const& lightPos = light->cachedPos;

	int tx = Math::FloorToInt(Math::ToTileValue(lightPos.x, float(BLOCK_SIZE)));
	int ty = Math::FloorToInt(Math::ToTileValue(lightPos.y, float(BLOCK_SIZE)));

	if (tx < range.xMin)
		range.xMin = tx;
	else if (tx > range.xMax)
		range.yMax = tx + 1;

	if (ty < range.yMin)
		range.yMin = ty;
	else if (ty > range.yMax)
		range.yMax = ty + 1;

	range.xMin = Math::Clamp(range.xMin, 0, terrain.getSizeX());
	range.xMax = Math::Clamp(range.xMax, 0, terrain.getSizeX());
	range.yMin = Math::Clamp(range.yMin, 0, terrain.getSizeY());
	range.yMax = Math::Clamp(range.yMax, 0, terrain.getSizeY());


	if (bUseGeometryShader)
	{
		RHISetShaderProgram(commandList, mProgShadow->getRHI());
		mProgShadow->setParam(commandList, SHADER_PARAM(XForm), param.worldToClipRHI);
		mProgShadow->setParam(commandList, SHADER_PARAM(LightPosAndDist), Vector3(lightPos.x, lightPos.y, light->radius));
	}
	else
	{
		RHISetFixedShaderPipelineState(commandList, param.worldToClipRHI);
	}

	TArray< Vector2 > buffer;

#if 0
	Vec2i tpLight = Vec2i(Math::FloorToInt(lightPos.x / BLOCK_SIZE), Math::FloorToInt(lightPos.y / BLOCK_SIZE));
	if (terrain.checkRange(tpLight.x, tpLight.y))
	{
		Tile const& tile = terrain.getData(tpLight.x, tpLight.y);
		Block* block = Block::Get(tile.id);
		if (block->checkFlag(BF_CAST_SHADOW))
		{
			if (!block->checkFlag(BF_NONSIMPLE))
				return;

			Rect bBox;
			bBox.min = lightPos - Vec2f(0.1, 0.1);
			bBox.max = lightPos + Vec2f(0.1, 0.1);
			if (block->testIntersect(tile, bBox))
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
				block->renderShadow(mDrawer, tile , lightPos , *light);
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

					Vec2f const& cur = tile.pos + tileVertex[idxCur];
					Vec2f const& prev = tile.pos + tileVertex[idxPrev];

					Vec2f offsetCur = tileVertex[idxCur] + tileOffset;
					if (offsetCur.dot(tileNormal[idxCur]) < 0)
						continue;

#endif
					if (bUseGeometryShader)
					{
						buffer.append({ prev, cur });
					}
					else
					{
						Vec2f offsetPrev = tileVertex[idxPrev] + tileOffset;

						Vec2f v1 = lightPos + light->radius * offsetPrev;
						Vec2f v2 = lightPos + light->radius * offsetCur;
						buffer.append({ prev, v1, v2, cur });
					}
				}
			}
		}
	}

	if (buffer.size())
	{
		if (bUseGeometryShader)
		{
			TRenderRT< RTVF_XY >::Draw(commandList, EPrimitive::LineList, buffer.data(), buffer.size());
		}
		else
		{
			TRenderRT< RTVF_XY >::Draw(commandList, EPrimitive::Quad, buffer.data(), buffer.size());
		}
	}
}

void RenderEngine::renderSceneFinal(RHICommandList& commandList, RenderParam& param)
{
	QSceneProgram* progScene = mProgScenes[param.mode];

	RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
	RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());
	RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

	RHISetShaderProgram(commandList, progScene->getRHI());

	auto& samplerState = TStaticSamplerState<ESampler::Bilinear>::GetRHI();

	progScene->setTexture(commandList, SHADER_PARAM(TexGeometry) , mTexGeometry , SHADER_SAMPLER(TexGeometry), samplerState);
	progScene->setTexture(commandList, SHADER_PARAM(TexLightmap) , mTexLightmap, SHADER_SAMPLER(TexLightmap), samplerState);
	progScene->setTexture(commandList, SHADER_PARAM(TexNormal), mTexNormalMap, SHADER_SAMPLER(TexNormal), samplerState);
	progScene->setParam(commandList, SHADER_PARAM(ambientLight) , mAmbientLight );

	DrawUtility::ScreenRect(commandList);
}

void RenderEngine::renderObjects( RenderPass pass , Level* level )
{
	if ( gUseGroupRender )
	{
		for( RenderGroupVec::iterator iter = mRenderGroups.begin() , itEnd = mRenderGroups.end();
			iter != itEnd ; ++iter )
		{
			RenderGroup* group = *iter;
			group->renderer->renderGroup(mDrawer, pass , group->numObject , group->objectLists);
		}
	}
	else
	{
		level->renderObjects(mDrawer, pass);
	}
}

void RenderEngine::updateRenderGroup( RenderParam& param )
{

	mRenderGroups.clear();
	mBodyList.clear();

	BoundBox bBox;
	bBox.min = param.camera->getPos();
	bBox.max = param.camera->getPos() + Vec2f( param.renderWidth  , param.renderHeight );
	param.level->getColManager().findBody( bBox , COL_RENDER , mBodyList );


	struct GroupCompFunc
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
		RenderGroupVec::iterator iterGroup = std::lower_bound( mRenderGroups.begin() , mRenderGroups.end() , &testGroup , GroupCompFunc() );
		
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

RHITexture2D& GetResource(Texture* texture)
{
	if (texture)
	{
		return *texture->resource;
	}
	return *GBlackTexture2D.getResource();
}

void RenderPrimitiveDrawer::setGlow(Texture* texture, Color3f const& color)
{
	auto& samplerState = TStaticSamplerState<ESampler::Bilinear>::GetRHI();
	mShader->setTexture(*mCommandList, SHADER_PARAM(Texture), GetResource(texture), SHADER_SAMPLER(Texture), samplerState);
	mShader->setParam(*mCommandList, SHADER_PARAM(Color), Color4f(color, 1.0f));
}

void RenderPrimitiveDrawer::setMaterial(PrimitiveMat const& mat)
{
	auto& samplerState = TStaticSamplerState<ESampler::Bilinear>::GetRHI();
	mShader->setTexture(*mCommandList, SHADER_PARAM(BaseTexture), GetResource(mat.baseTex), SHADER_SAMPLER(BaseTexture), samplerState);
	mShader->setTexture(*mCommandList, SHADER_PARAM(NormalTexture), GetResource(mat.normalTex), SHADER_SAMPLER(NormalTexture), samplerState);
	mShader->setParam(*mCommandList, SHADER_PARAM(Color), Color4f(mat.color, mAlpha));
}

RHIBlendState& GetBlendState(ESimpleBlendMode mode)
{
#define BLEND_ARGS EBlend::One, EBlend::Zero, EBlend::Add, false, true, CWM_None

	switch (mode)
	{
	case ESimpleBlendMode::Translucent:
		return TStaticBlendState< CWM_RGBA, EBlend::SrcAlpha, EBlend::OneMinusSrcAlpha, EBlend::Add, BLEND_ARGS >::GetRHI();
	case ESimpleBlendMode::Add:
		return TStaticBlendState< CWM_RGBA, EBlend::One, EBlend::One, EBlend::Add, BLEND_ARGS >::GetRHI();
	default:
		break;
	}


	return TStaticBlendState<>::GetRHI();
#undef BLEND_ARGS

}


void RenderPrimitiveDrawer::beginBlend(float alpha, Render::ESimpleBlendMode mode)
{
	RHISetBlendState(*mCommandList, GetBlendState(mode));
	mAlpha = alpha;
}

void RenderPrimitiveDrawer::endBlend()
{
	RHISetBlendState(*mCommandList, TStaticBlendState<>::GetRHI());
	mAlpha = 1.0f;
}
