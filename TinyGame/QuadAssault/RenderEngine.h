#ifndef RenderEngine_h__
#define RenderEngine_h__

#include "Base.h"
#include "Dependence.h"
#include "RenderUtility.h"
#include "GBuffer.h"

#include "RHI/ShaderProgram.h"
#include "RHI/GlobalShader.h"
using namespace Render;

#include "FrameAllocator.h"

#define QA_USE_RHI 1

class ColBody;
typedef std::vector< ColBody* > ColBodyVec;

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

struct RenderParam
{
	Level*     level;
	Object*    camera;
	float      scaleFactor;
	RenderMode mode;
	////////////////////////
	TileRange  terrainRange;
	float      renderWidth;
	float      renderHeight;
};



class QBasePassBaseProgram : public GlobalShaderProgram
{
public:
	using BaseClass = GlobalShaderProgram;
	DECLARE_SHADER_PROGRAM(QBasePassBaseProgram, Global);

	static char const* GetShaderFileName()
	{
		return "QBasePass";
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




struct RenderGroup
{
	int                order;
	IObjectRenderer*   renderer;
	int                numObject;
	LevelObject*       objectLists;
};

class RenderEngine
{
public:
	RenderEngine();
	bool         init( int width , int height );
	void         cleanup();

	void         renderScene( RenderParam& param );
	Vec3f const& getAmbientLight() const { return mAmbientLight; }
	void         setAmbientLight( Vec3f const& color ) { mAmbientLight = color; }
private:

	void   renderBasePass(RenderParam& param);

	void   renderLightingFBO( RenderParam& param );
	void   renderNormalFBO( RenderParam& param );
	void   renderGeometryFBO( RenderParam& param );
	void   renderSceneFinal( RenderParam& param );

	void   renderTerrain( Level* level , TileRange const& range );
	void   renderTerrainNormal( Level* level , TileRange const& range );
	void   renderTerrainGlow( Level* level , TileRange const& range );
	void   renderTerrainShadow( Level* level , Vec2f const& lightPos , Light* light , TileRange const& range );
	void   renderLight( RenderParam& param , Vec2f const& lightPos , Light* light );
	bool   setupFBO( int width , int height );
	void   setupLightShaderParam(ShaderProgram& shader , Light* light );

	void   updateRenderGroup( RenderParam& param );


	void   renderObjects( RenderPass pass , Level* level );

	
	typedef std::vector< RenderGroup* > RenderGroupVec;

	PrimitiveDrawer mDrawer;
	RenderGroupVec  mRenderGroups;
	FrameAllocator  mAllocator;
	ColBodyVec      mBodyList;

	GBuffer        mGBuffer;
	float   mFrameWidth;
	float   mFrameHeight;

	ShaderProgram mShaderLighting;
	static int const NumMode = 3;
	ShaderProgram mShaderScene[NumMode];

	GLuint mFBO;	
	GLuint mRBODepth;

	RHITexture2DRef mTexLightmap;
	RHITexture2DRef mTexNormalMap;
	RHITexture2DRef mTexGeometry;
	RHIFrameBufferRef mDeferredFrameBuffer;


	class QBasePassBaseProgram* mProgBasePass;

	Vec3f  mAmbientLight;

};

#endif // RenderEngine_h__
