#pragma once
#ifndef TestRenderStageBase_H_E5786E12_7554_40DA_A42E_924732B7AB5D
#define TestRenderStageBase_H_E5786E12_7554_40DA_A42E_924732B7AB5D

#include "Stage/TestStageHeader.h"

#include "RHI/RHICommand.h"
#include "RHI/ShaderManager.h"
#include "RHI/RenderContext.h"
#include "RHI/DrawUtility.h"
#include "RHI/RHIUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/ShaderCore.h"
#include "RHI/Scene.h"

#include "Renderer/Mesh.h"
#include "Renderer/SimpleCamera.h"

#include "Core/ScopeGuard.h"
#include "Math/PrimitiveTest.h"
#include "Serialize/SerializeFwd.h"

#include "DataCacheInterface.h"

#include "Clock.h"
#include "GameRenderSetup.h"

#include "RenderDebug.h"


namespace Render
{


	class StaticMesh;
	class IBLResource;


	bool LoadObjectMesh(StaticMesh& mesh , char const* path );
	template< class TFunc, class ...Args >
	bool BuildMesh(Mesh& mesh, char const* meshName, TFunc&& FuncMeshCreate, Args&& ...args)
	{
		DataCacheKey key;
		key.typeName = "MESH";
		key.version = "155CABB4-4F63-4B6C-9249-5BB7861F67E1";
		key.keySuffix.add(meshName, std::forward<Args>(args)...);

		auto MeshLoad = [&mesh](IStreamSerializer& serializer) -> bool
		{
			return mesh.load(serializer);
		};

		auto MeshSave = [&mesh](IStreamSerializer& serializer) -> bool
		{
			return mesh.save(serializer);
		};

		if(!::Global::DataCache().loadDelegate(key, MeshLoad) || true)
		{
			if( !FuncMeshCreate(mesh, std::forward<Args>(args)...) )
			{
				return false;
			}

			if (GRHISystem->getName() == RHISystemName::D3D11)
				return true;


			if(!::Global::DataCache().saveDelegate(key, MeshSave) )
			{

			}
#if 0
			else
			{
				Mesh temp;
				::Global::DataCache().loadDelegate(key, [&temp](IStreamSerializer& serializer) -> bool
				{
					return temp.load(serializer);
				});

			}
#endif
		}

		return true;
	}

	template< class TFunc >
	bool BuildMeshFromFile(Mesh& mesh, char const* meshPath, TFunc&& FuncMeshCreate)
	{
		DataCacheKey key;
		key.typeName = "MESH";
		key.version = "155CABB4-4F63-4B6C-9249-5BB7861F67E6";
		key.keySuffix.addFileAttribute(meshPath);

		auto MeshLoad = [&mesh](IStreamSerializer& serializer) -> bool
		{
			return mesh.load(serializer);
		};

		auto MeshSave = [&mesh](IStreamSerializer& serializer) -> bool
		{
			return mesh.save(serializer);
		};

		if ( !::Global::DataCache().loadDelegate(key, MeshLoad))
		{
			if (!FuncMeshCreate(mesh, meshPath) )
			{
				return false;
			}

			if (GRHISystem->getName() == RHISystemName::D3D11)
				return true;


			if (!::Global::DataCache().saveDelegate(key, MeshSave))
			{

			}
#if 0
			else
			{
				Mesh temp;
				::Global::DataCache().loadDelegate(key, [&temp](IStreamSerializer& serializer) -> bool
				{
					return temp.load(serializer);
				});

			}
#endif
		}

		return true;
	}

	template< class TFunc >
	bool BuildMultiMeshFromFile(TArray<Mesh>& meshes, char const* meshPath, TFunc&& FuncMeshCreate)
	{
		DataCacheKey key;
		key.typeName = "MESH";
		key.version = "155CABB4-4F63-4B6C-9249-5BB7861F67E6";
		key.keySuffix.addFileAttribute(meshPath);

		auto MeshLoad = [&meshes](IStreamSerializer& serializer) -> bool
		{
			for (auto& mesh : meshes)
			{
				if (!mesh.load(serializer))
					return false;
			}
			return true;
		};

		auto MeshSave = [&meshes](IStreamSerializer& serializer) -> bool
		{
			for (auto& mesh : meshes)
			{
				if (!mesh.save(serializer))
					return false;
			}
			return true;
		};

		if (!::Global::DataCache().loadDelegate(key, MeshLoad))
		{
			if (!FuncMeshCreate(meshes, meshPath))
			{
				return false;
			}

			if (GRHISystem->getName() == RHISystemName::D3D11)
				return true;


			if (!::Global::DataCache().saveDelegate(key, MeshSave))
			{

			}
#if 0
			else
			{
				Mesh temp;
				::Global::DataCache().loadDelegate(key, [&temp](IStreamSerializer& serializer) -> bool
				{
					return temp.load(serializer);
				});

			}
#endif
		}

		return true;
	}

	class InstancedMesh
	{
	public:

		TINY_API void setupMesh(Mesh& InMesh);

		void changeMesh(Mesh& InMesh)
		{
			if( mMesh == &InMesh )
				return;

			setupMesh(InMesh);
		}

		int addInstance(Vector3 const& pos, Vector3 const& scale, Quaternion const& rotation, Vector4 const& param)
		{
			Matrix4 xform = Matrix4::Scale(scale) * Matrix4::Rotate(rotation) * Matrix4::Translate(pos);
			return addInstance(xform, param);
		}

		int addInstance(Matrix4 const& xform, Vector4 const& param)
		{
			int result = mInstanceTransforms.size();
			mInstanceTransforms.push_back(xform);
			mInstanceParams.push_back(param);
			bBufferValid = false;
			return result;
		}

		void clearInstance()
		{
			mInstanceParams.clear();
			mInstanceTransforms.clear();
			bBufferValid = true;
		}

		TINY_API bool UpdateInstanceBuffer();

		TINY_API void draw(RHICommandList& comandList);

		void releaseRHI()
		{
			mInputLayout.release();
			mInstancedBuffer.release();
			bBufferValid = false;
		}

		Mesh* mMesh;

		RHIInputLayoutRef mInputLayout;
		RHIBufferRef mInstancedBuffer;
		TArray< Matrix4 > mInstanceTransforms;
		TArray< Vector4 > mInstanceParams;
		bool bBufferValid = false;
	};

	class ViewFrustum
	{
	public:
		Matrix4 getPerspectiveMatrix()
		{
			return PerspectiveMatrixZBuffer(mYFov, mAspect, mNear, mFar);
		}

		bool  bLeftHandCoord = false;
		float mNear;
		float mFar;
		float mYFov;
		float mAspect;
	};

	class SphereProgram : public GlobalShaderProgram
	{
		DECLARE_EXPORTED_SHADER_PROGRAM(SphereProgram, Global , TINY_API)

		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/Game/Sphere";
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
	public:
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			mParamRadius.bind(parameterMap, SHADER_PARAM(Sphere.radius));
			mParamWorldPos.bind(parameterMap, SHADER_PARAM(Sphere.worldPos));
			mParamBaseColor.bind(parameterMap, SHADER_PARAM(Sphere.baseColor));
		}
		void setParameters(RHICommandList& commandList, Vector3 const& pos , float radius , Vector3 const& baseColor)
		{
			setParam(commandList, mParamRadius, radius);
			setParam(commandList, mParamWorldPos, pos);
			setParam(commandList, mParamBaseColor, baseColor);
		}

		ShaderParameter mParamRadius;
		ShaderParameter mParamWorldPos;
		ShaderParameter mParamBaseColor;
	};

	class SkyBoxProgram : public GlobalShaderProgram
	{
	public:
		using BaseClass = GlobalShaderProgram;
		DECLARE_SHADER_PROGRAM(SkyBoxProgram, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/SkyBox";
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

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, Texture);
			BIND_SHADER_PARAM(parameterMap, CubeTexture);
			BIND_SHADER_PARAM(parameterMap, CubeLevel);
		}

		DEFINE_SHADER_PARAM(Texture);
		DEFINE_SHADER_PARAM(CubeTexture);
		DEFINE_SHADER_PARAM(CubeLevel);
	};


	struct SimpleMeshId
	{
		enum
		{
			Tile,
			Sphere,
			Sphere2,
			SpherePlane,
			Box,
			Plane,
			Doughnut,
			SkyBox,
			SimpleSkin,
			Terrain,
			NumSimpleMesh,
		};
	};
	struct TINY_API SharedAssetData
	{
		bool loadCommonShader();
		bool createSimpleMesh();

		Mesh& getMesh(int id) { return mSimpleMeshs[id]; }
		SphereProgram* mProgSphere;
		SkyBoxProgram* mProgSkyBox;

		Mesh   mSimpleMeshs[SimpleMeshId::NumSimpleMesh];


		void releaseRHIResource(bool bReInit = false);
	};



	class IValueModifier
	{
	public:
		virtual bool isHook(void* ptr) { return false; }
		virtual void update(float time) = 0;
	};

	template< class TrackType >
	class TVectorTrackModifier : public IValueModifier
	{
	public:
		TVectorTrackModifier(Vector3& value)
			:mValue(value)
		{

		}
		virtual void update(float time)
		{
			mValue = track.getValue(time);
		}
		virtual bool isHook(void* ptr) { return &mValue == ptr; }
		TrackType track;
	private:
		Vector3&  mValue;
	};


	class TINY_API TestRenderStageBase : public StageBase
		                               , public SharedAssetData
		                               , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		TestRenderStageBase() {}

		ViewInfo      mView;
		ViewFrustum   mViewFrustum;
		SimpleCamera  mCamera;
		bool          mbGamePased;

		bool onInit() override;

		void onEnd() override;

		virtual void restart(){}
		virtual void tick() {}
		virtual void updateFrame(int frame) {}
		void onUpdate(long time) override;

		//
		virtual bool setupRenderResource(ERenderSystem systemName)
		{
			if ( systemName != ERenderSystem::D3D12 )
			{
				VERIFY_RETURN_FALSE(ShaderHelper::Get().init());
				VERIFY_RETURN_FALSE(mBitbltFrameBuffer = RHICreateFrameBuffer());
			}
			return true;
		}

		virtual void preShutdownRenderSystem(bool bReInit)
		{
			SharedAssetData::releaseRHIResource(bReInit);
			mView.releaseRHIResource();
			mBitbltFrameBuffer.release();
		}

		void initializeRenderState();
		void bitBltToBackBuffer(RHICommandList& commandList, RHITexture2D& texture);

		RHIFrameBufferRef mBitbltFrameBuffer;

		MsgReply onMouse(MouseMsg const& msg) override;

		MsgReply onKey(KeyMsg const& msg) override;


		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch( id )
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}

		void drawLightPoints(RHICommandList& commandList, ViewInfo& view, TArrayView< LightInfo > lights);
		struct ESkyboxShow
		{
			enum
			{
				Normal,
				Irradiance,
				Prefiltered_0,
				Prefiltered_1,
				Prefiltered_2,
				Prefiltered_3,
				Prefiltered_4,

				Count,
			};
		};
		void drawSkyBox(RHICommandList& commandList, ViewInfo& view, RHITexture2D& HDRImage, IBLResource& IBL, int skyboxShowIndex);
	};

}//namespace Render

#endif // TestRenderStageBase_H_E5786E12_7554_40DA_A42E_924732B7AB5D