#ifndef LightGLStage_h__
#define LightGLStage_h__

#include "Stage/TestRenderStageBase.h"
#include "StageBase.h"
#include "RHI/ShaderCore.h"

#include "DataStructure/Grid2D.h"
#include "CppVersion.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"

#define SHADOW_USE_GEOMETRY_SHADER 1


class IEditorDetailView;

namespace Render
{
	typedef Math::Vector2 Vector2;
	typedef Math::Vector3 Color;

	Vector3 DefaultLightAttenuation = Vector3(1, 0.2, 0.1);
	int constexpr ShadowTexureWidth = 512;
	int constexpr SDFTextureWidth = 512;
	int constexpr SDFTextureHeight = 512;
	struct Light
	{
		DECLARE_BUFFER_STRUCT(Lights);
		Vector2 pos;
		float   radius;
		float   sourceRadius = 1.0f;
		Color3f color;
		float   intensity = 1.0f;
		Vector3 attenuation = DefaultLightAttenuation;

		REFLECT_STRUCT_BEGIN(Light)
			REF_PROPERTY(pos)
			REF_PROPERTY(radius)
			REF_PROPERTY(sourceRadius)
			REF_PROPERTY(color)
			REF_PROPERTY(intensity)
			REF_PROPERTY(attenuation)
		REFLECT_STRUCT_END()
	};

	struct Segment
	{
		DECLARE_BUFFER_STRUCT(BlockSegments);
		Vector2 p0;
		Vector2 p1;
	};

	struct Block
	{
		Vector2 pos;
		std::vector< Vector2 > vertices;
		int          getVertexNum() const { return (int)vertices.size(); }
		Vector2 const& getVertex( int idx ) const { return vertices[ idx ]; }
		Vector2 const* getVertices() const { return &vertices[0]; }
		void setBox( Vector2 const& pos , Vector2 const& size )
		{
			this->pos = pos;
			float wHalf = size.x / 2;
			float hHalf = size.y / 2;
			vertices.push_back( Vector2( wHalf , hHalf ) );
			vertices.push_back( Vector2( -wHalf , hHalf ) );
			vertices.push_back( Vector2( -wHalf , -hHalf ) );
			vertices.push_back( Vector2( wHalf , -hHalf ) );
		}

	};

	class LightingProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(LightingProgram, Global);
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/Game/lighting2D";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) } ,
				{ EShader::Pixel , SHADER_ENTRY(LightingPS) } ,
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, LightLocation);
			BIND_SHADER_PARAM(parameterMap, LightColor);
			BIND_SHADER_PARAM(parameterMap, LightAttenuation);
			BIND_SHADER_PARAM(parameterMap, LightRadius);
			BIND_SHADER_PARAM(parameterMap, LightSourceRadius);
		}

		void setParameters(RHICommandList& commandList, Light const& light)
		{
			SET_SHADER_PARAM(commandList, *this, LightLocation, light.pos);
			SET_SHADER_PARAM(commandList, *this, LightColor, light.intensity * Vector3(light.color.r, light.color.g, light.color.b));
			SET_SHADER_PARAM(commandList, *this, LightAttenuation, light.attenuation);
			SET_SHADER_PARAM(commandList, *this, LightRadius, light.radius);
			SET_SHADER_PARAM(commandList, *this, LightSourceRadius, light.sourceRadius);
		}

		DEFINE_SHADER_PARAM(LightLocation);
		DEFINE_SHADER_PARAM(LightColor);
		DEFINE_SHADER_PARAM(LightAttenuation);
		DEFINE_SHADER_PARAM(LightRadius);
		DEFINE_SHADER_PARAM(LightSourceRadius);
	};


	class LightingShadowProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(LightingShadowProgram, Global);
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/Game/Lighting2DShadow";
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
			BIND_SHADER_PARAM(parameterMap, LightLocation);
			BIND_SHADER_PARAM(parameterMap, WorldToClip);
		}

		void setParameters(RHICommandList& commandList, Vector2 const& lightPos, Matrix4 const& worldToClip)
		{
			SET_SHADER_PARAM(commandList, *this, LightLocation, lightPos);
			SET_SHADER_PARAM(commandList, *this, WorldToClip, worldToClip);
		}

		DEFINE_SHADER_PARAM(LightLocation);
		DEFINE_SHADER_PARAM(WorldToClip);
	};

	class Shadow1DMapCS : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(Shadow1DMapCS, Global);
	public:
		static char const* GetShaderFileName() { return "Shader/Game/Lighting2DShadow"; }

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(SHADOW_TEXTURE_WIDTH), ShadowTexureWidth);
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Compute , SHADER_ENTRY(MainCS) },
			};
			return entries;
		}
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, LightCount);
			BIND_SHADER_PARAM(parameterMap, SegmentCount);
			BIND_SHADER_PARAM(parameterMap, ShadowMapAtlas);
		}

		DEFINE_SHADER_PARAM(LightCount);
		DEFINE_SHADER_PARAM(SegmentCount);
		DEFINE_SHADER_PARAM(ShadowMapAtlas);
	};

	class ShadowBlockerSearchCS : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(ShadowBlockerSearchCS, Global);
	public:
		static char const* GetShaderFileName() { return "Shader/Game/Lighting2DShadow"; }

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(SHADOW_TEXTURE_WIDTH), ShadowTexureWidth);
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Compute , SHADER_ENTRY(BlockerSearchCS) },
			};
			return entries;
		}
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, MaxLightNum);
			BIND_SHADER_PARAM(parameterMap, ShadowMapAtlas);
		}

		DEFINE_SHADER_PARAM(MaxLightNum);
		DEFINE_SHADER_PARAM(ShadowMapAtlas);
	};

	class Lighting1DShadowProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(Lighting1DShadowProgram, Global);
	public:
		static char const* GetShaderFileName() { return "Shader/Game/lighting2D"; }

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(SHADOW_TEXTURE_WIDTH), ShadowTexureWidth);
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel , SHADER_ENTRY(LightingShadowMapPS) } ,
			};
			return entries;
		}
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, Lights);
			BIND_SHADER_PARAM(parameterMap, MaxLightNum);
			BIND_SHADER_PARAM(parameterMap, ScreenToWorld);
			BIND_SHADER_PARAM(parameterMap, ShadowBias);
			BIND_TEXTURE_PARAM(parameterMap, ShadowMapAtlas);
		}
		DEFINE_SHADER_PARAM(Lights);
		DEFINE_SHADER_PARAM(MaxLightNum);
		DEFINE_SHADER_PARAM(ScreenToWorld);
		DEFINE_SHADER_PARAM(ShadowBias);
		DEFINE_TEXTURE_PARAM(ShadowMapAtlas);
	};

	class BuildSceneSDFCS : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(BuildSceneSDFCS, Global);
	public:
		static char const* GetShaderFileName() { return "Shader/Game/Lighting2DShadow"; }
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Compute , SHADER_ENTRY(BuildSceneSDFCS) },
			};
			return entries;
		}
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, SegmentCount);
			BIND_SHADER_PARAM(parameterMap, SDFWorldMin);
			BIND_SHADER_PARAM(parameterMap, SDFWorldMax);
			BIND_SHADER_PARAM(parameterMap, SDFTextureSize);
			BIND_SHADER_PARAM(parameterMap, EmptyDistanceValue);
			BIND_SHADER_PARAM(parameterMap, SceneSDFTexture);
		}

		DEFINE_SHADER_PARAM(SegmentCount);
		DEFINE_SHADER_PARAM(SDFWorldMin);
		DEFINE_SHADER_PARAM(SDFWorldMax);
		DEFINE_SHADER_PARAM(SDFTextureSize);
		DEFINE_SHADER_PARAM(EmptyDistanceValue);
		DEFINE_SHADER_PARAM(SceneSDFTexture);
	};

	class LightingSDFProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(LightingSDFProgram, Global);
	public:
		static char const* GetShaderFileName() { return "Shader/Game/lighting2D"; }
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel , SHADER_ENTRY(LightingSDFPS) } ,
			};
			return entries;
		}
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, Lights);
			BIND_SHADER_PARAM(parameterMap, ScreenToWorld);
			BIND_SHADER_PARAM(parameterMap, WorldToClip);
			BIND_SHADER_PARAM(parameterMap, SDFWorldMin);
			BIND_SHADER_PARAM(parameterMap, SDFWorldMax);
			BIND_SHADER_PARAM(parameterMap, ShadowBias);
			BIND_SHADER_PARAM(parameterMap, SDFHitThreshold);
			BIND_SHADER_PARAM(parameterMap, SDFMinStepScale);
			BIND_SHADER_PARAM(parameterMap, SDFMaxSteps);
			BIND_SHADER_PARAM(parameterMap, SDFMaxStepLength);
			BIND_SHADER_PARAM(parameterMap, SDFSoftShadowScale);
			BIND_TEXTURE_PARAM(parameterMap, SceneSDFTexture);
		}
		DEFINE_SHADER_PARAM(Lights);
		DEFINE_SHADER_PARAM(ScreenToWorld);
		DEFINE_SHADER_PARAM(WorldToClip);
		DEFINE_SHADER_PARAM(SDFWorldMin);
		DEFINE_SHADER_PARAM(SDFWorldMax);
		DEFINE_SHADER_PARAM(ShadowBias);
		DEFINE_SHADER_PARAM(SDFHitThreshold);
		DEFINE_SHADER_PARAM(SDFMinStepScale);
		DEFINE_SHADER_PARAM(SDFMaxSteps);
		DEFINE_SHADER_PARAM(SDFMaxStepLength);
		DEFINE_SHADER_PARAM(SDFSoftShadowScale);
		DEFINE_TEXTURE_PARAM(SceneSDFTexture);
	};


	class Lighting2DTestStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;

		using LightList = TArray< Light >;
		using BlockList = TArray< Block >;

	public:

		enum
		{
			UI_RUN_BENCHMARK = BaseClass::NEXT_UI_ID ,

			NEXT_UI_ID,
		};
		LightList lights;
		BlockList blocks;

		Render::RenderTransform2D mWorldToScreen;
		Render::RenderTransform2D mScreenToWorld;

		float  mZoom = 1.0f;
		Vector2 mViewPos = 0.5 * Vector2(20, 20);
		bool    bIsDragging = false;
		Vec2i   mLastMousePos;

		void updateView()
		{
			Vec2i screenSize = ::Global::GetScreenSize();
			mWorldToScreen = RenderTransform2D::LookAt(screenSize, mViewPos, Vector2(0, 1), mZoom * screenSize.y / float(20 + 5), true);
			mScreenToWorld = mWorldToScreen.inverse();
		}


		LightingProgram* mProgLighting;
		LightingShadowProgram* mProgShadow;
		Lighting2DTestStage(){}

		TArray< Vector2 > mBuffers;

		bool bShowShadowRender = false;
		bool bUseGeometryShader = true;
		bool bUse1DShadowMap = false;
		bool bUseSDFShadow = true;

		Shadow1DMapCS* mProgShadow1D = nullptr;
		ShadowBlockerSearchCS* mProgShadowBlockerSearch = nullptr;
		Lighting1DShadowProgram* mProgLighting1D = nullptr;
		BuildSceneSDFCS* mProgBuildSceneSDF = nullptr;
		LightingSDFProgram* mProgLightingSDF = nullptr;
		RHITexture2DRef mShadowMapAtlas;
		RHITexture2DRef mSceneSDFTexture;
		RHIFrameBufferRef mShadowMapFB;
		TStructuredBuffer<Segment> mSegmentBuffer;
		TStructuredBuffer<Light>   mLightBuffer;
		TArray<Light> mVisibleLights;
		TArray<int> mVisibleBlockIndices;

		bool onInit() override;

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::None;
		}
		bool setupRenderResource(ERenderSystem systemName) override;
		void preShutdownRenderSystem(bool bReInit = false) override;

		void onUpdate(GameTimeSpan deltaTime) override;
		bool onWidgetEvent(int event, int id, GWidget* ui) override;
		void onRender( float dFrame ) override;


		void renderPolyShadow( Light const& light , Vector2 const& pos , Vector2 const* vertices , int numVertices );
		void renderSceneSDF(RHICommandList& commandList, Vector2 const& min, Vector2 const& max);
		void render1DShadowMaps(RHICommandList& commandList);
		void renderLightingSDF(RHICommandList& commandList, Matrix4 const& worldToClipRHI);
		void renderLighting1D(RHICommandList& commandList);

		void restart();

		MsgReply onMouse( MouseMsg const& msg ) override;
		MsgReply onKey(KeyMsg const& msg) override
		{
			if ( msg.isDown() )
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				case EKeyCode::Z:
					if (!lights.empty())
					{
						lights.pop_back();

						if (mSelectedLightIndex == lights.size())
							mSelectedLightIndex = INDEX_NONE;

						updateDetailView();
					}
					break;
				case EKeyCode::X:
					if (!blocks.empty())
					{
						blocks.pop_back();
					}
					break;
				}
			}

			return BaseClass::onKey(msg);
		}


		int mSelectedLightIndex = -1;
		IEditorDetailView* mDetailView = nullptr;
		void updateDetailView();

	protected:

	};



}



#endif // LightGLStage_h__
