#ifndef RenderEngine_h__
#define RenderEngine_h__

#include "Base.h"
#include "Dependence.h"
#include "RenderUtility.h"

#include "RHI/ShaderProgram.h"
#include "RHI/GlobalShader.h"
#include "RHI/RHICommand.h"
#include "Memory/FrameAllocator.h"

#include "DataStructure/Array.h"

using namespace Render;

class ColBody;
using ColBodyVec = TArray< ColBody* >;

class Level;
class Object;
class Light;
class IObjectRenderer;

struct TileRange
{
	int xMin;
	int xMax;
	int yMin;
	int yMax;
};

enum RenderMode
{
	RM_ALL = 0 ,
	RM_GEOMETRY  ,
	RM_LINGHTING ,
	RM_NORMAL_MAP ,

	NUM_RENDER_MODE ,
};

struct RenderConfig
{
	Level*     level;
	Object*    camera;
	float      scaleFactor;
	RenderMode mode;
};


struct RenderParam : RenderConfig
{
	Math::Matrix4     worldToClipRHI;
	RenderTransform2D worldToView;
	TileRange  terrainRange;
	float      renderWidth;
	float      renderHeight;
};



class QBasePassProgram : public GlobalShaderProgram
{
public:
	using BaseClass = GlobalShaderProgram;
	DECLARE_SHADER_PROGRAM(QBasePassProgram, Global);

	static char const* GetShaderFileName()
	{
		return "Shader/QuadAssault/QBasePass";
	}

	static TArrayView< ShaderEntryInfo const > GetShaderEntries()
	{
		static ShaderEntryInfo const entries[] =
		{
			{ EShader::Vertex , SHADER_ENTRY(MainVS) },
			{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
		};
		return entries;
	}

};

class QGlowProgram : public GlobalShaderProgram
{
public:
	using BaseClass = GlobalShaderProgram;
	DECLARE_SHADER_PROGRAM(QGlowProgram, Global);

	static char const* GetShaderFileName()
	{
		return "Shader/QuadAssault/QGlow";
	}

	static TArrayView< ShaderEntryInfo const > GetShaderEntries()
	{
		static ShaderEntryInfo const entries[] =
		{
			{ EShader::Vertex , SHADER_ENTRY(MainVS) },
			{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
		};
		return entries;
	}
};

class QLightingProgram : public GlobalShaderProgram
{
public:
	using BaseClass = GlobalShaderProgram;
	DECLARE_SHADER_PROGRAM(QLightingProgram, Global);

	static char const* GetShaderFileName()
	{
		return "Shader/QuadAssault/QLighting";
	}

	static TArrayView< ShaderEntryInfo const > GetShaderEntries()
	{
		static ShaderEntryInfo const entries[] =
		{
			{ EShader::Vertex , SHADER_ENTRY(LightingVS) },
			{ EShader::Pixel  , SHADER_ENTRY(LightingPS) },
		};
		return entries;
	}
};

class QShadowProgram : public GlobalShaderProgram
{
public:
	using BaseClass = GlobalShaderProgram;
	DECLARE_SHADER_PROGRAM(QShadowProgram, Global);

	static char const* GetShaderFileName()
	{
		return "Shader/QuadAssault/QShadow";
	}
	static TArrayView< ShaderEntryInfo const > GetShaderEntries()
	{
		static ShaderEntryInfo const entries[] =
		{
			{ EShader::Vertex , SHADER_ENTRY(MainVS) },
			{ EShader::Geometry , SHADER_ENTRY(MainGS) },
			{ EShader::Pixel , SHADER_ENTRY(MainPS) } ,
		};
		return entries;
	}

	void bindParameters(ShaderParameterMap const& parameterMap)
	{
		BIND_SHADER_PARAM(parameterMap, LightPosAndDist);
		BIND_SHADER_PARAM(parameterMap, ScreenSize);
	}

	void setParameters(RHICommandList& commandList, Vector2 const& lightPos, float castDist, Vector2 const& screenSize)
	{
		SET_SHADER_PARAM(commandList, *this, LightPosAndDist, Vector3(lightPos.x, lightPos.y, castDist));
		SET_SHADER_PARAM(commandList, *this, ScreenSize, screenSize);
	}

	DEFINE_SHADER_PARAM(LightPosAndDist);
	DEFINE_SHADER_PARAM(ScreenSize);
};

class QSceneProgram : public GlobalShaderProgram
{
public:
	using BaseClass = GlobalShaderProgram;
	DECLARE_SHADER_PROGRAM(QSceneProgram, Global);

	SHADER_PERMUTATION_TYPE_INT_COUNT(RenderMode, SHADER_PARAM(RENDER_MODE), NUM_RENDER_MODE);
	using PermutationDomain = TShaderPermutationDomain<RenderMode>;
	static char const* GetShaderFileName()
	{
		return "Shader/QuadAssault/QScene";
	}

	static TArrayView< ShaderEntryInfo const > GetShaderEntries(PermutationDomain const& domain)
	{
		static ShaderEntryInfo const entries[] =
		{
			{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
			{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
		};
		return entries;
	}
};

struct GPU_ALIGN LightParams
{
	DECLARE_UNIFORM_BUFFER_STRUCT(LightParamsBlock);

	Vector2 pos;
	float   angle;
	float   radius;
	Vector3 color;
	float   intensity;
	Vector2 dir;
	int     isExplosion;
	int     dummy;
};

struct ViewParams
{
	DECLARE_UNIFORM_BUFFER_STRUCT(ViewParamsBlock);

	Vector2 rectPos;
	Vector2 sizeInv;
};



struct RenderGroup
{
	int                order;
	IObjectRenderer*   renderer;
	int                numObject;
	LevelObject*       objectLists;
};

class RenderPrimitiveDrawer : public PrimitiveDrawer
{
public:
	ShaderProgram* mShader;

	void setGlow(Texture* texture, Color3f const& color) override;
	void setMaterial(PrimitiveMat const& mat) override;
	void beginBlend(float alpha, Render::ESimpleBlendMode mode) override;
	void endBlend() override;

	float mAlpha;

};

class RenderEngine
{
public:
	RenderEngine();
	bool         init( int width , int height );
	void         cleanup();

	void         renderScene( RenderConfig const& config );
	Color3f const& getAmbientLight() const { return mAmbientLight; }
	void         setAmbientLight( Color3f const& color ) { mAmbientLight = color; }
private:

	void   renderBasePass(RHICommandList& commandList, RenderParam& param);
	void   renderLighting(RHICommandList& commandList, RenderParam& param );
	void   renderSceneFinal(RHICommandList& commandList, RenderParam& param );

	void   renderLight(RHICommandList& commandList, RenderParam& param, Light* light);
	void   renderTerrainShadow(RHICommandList& commandList, RenderParam& param, Light* light);

	bool   setupFrameBuffer( int width , int height );
	void   updateRenderGroup( RenderParam& param );


	void   renderObjects( RenderPass pass , Level* level );

	template< typename TFunc >
	void   visitTiles(Level* level, TileRange const& range, TFunc&& func )
	{
		TileMap& terrain = level->getTerrain();
		for (int i = range.xMin; i < range.xMax; ++i)
		{
			for (int j = range.yMin; j < range.yMax; ++j)
			{
				Tile const& tile = terrain.getData(i, j);
				func(tile);
			}
		}
	}

	
	typedef std::vector< RenderGroup* > RenderGroupVec;

	RenderPrimitiveDrawer mDrawer;
	RenderGroupVec  mRenderGroups;
	FrameAllocator  mAllocator;
	ColBodyVec      mBodyList;

	float   mFrameWidth;
	float   mFrameHeight;

	ShaderProgram mShaderLighting;


	RHITexture2DRef mTexLightmap;
	RHITexture2DRef mTexNormalMap;
	RHITexture2DRef mTexGeometry;
	RHITexture2DRef mTexDepth;

	RHIFrameBufferRef mDeferredFrameBuffer;
	RHIFrameBufferRef mLightingFrameBuffer;

	TStructuredBuffer<LightParams>  mLightBuffer;
	TStructuredBuffer<ViewParams>   mViewBuffer;
	class QBasePassProgram* mProgBasePass;
	class QGlowProgram*     mProgGlow;
	class QShadowProgram*   mProgShadow;
	class QLightingProgram* mProgLighting;
	class QSceneProgram*    mProgScenes[NUM_RENDER_MODE];
	bool bUseGeometryShader = true;
	Color3f  mAmbientLight = Color3f(0,0,0);

};

#endif // RenderEngine_h__
