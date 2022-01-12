#pragma once
#ifndef TestRenderStageBase_H_E5786E12_7554_40DA_A42E_924732B7AB5D
#define TestRenderStageBase_H_E5786E12_7554_40DA_A42E_924732B7AB5D

#include "Stage/TestStageHeader.h"

#include "RHI/RHICommand.h"
#include "RHI/ShaderManager.h"
#include "RHI/RenderContext.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/ShaderCore.h"
#include "RHI/Scene.h"

#include "Renderer/Mesh.h"
#include "Renderer/SimpleCamera.h"

#include "Core/ScopeExit.h"
#include "Math/PrimitiveTest.h"
#include "Serialize/SerializeFwd.h"

#include "DataCacheInterface.h"

#include "Clock.h"
#include "GameRenderSetup.h"

#include "RenderDebug.h"


namespace Render
{


	class StaticMesh;


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

		if( !::Global::DataCache().loadDelegate(key, MeshLoad) )
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

		if (!::Global::DataCache().loadDelegate(key, MeshLoad))
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

	class InstancedMesh
	{
	public:

		void setupMesh(Mesh& InMesh)
		{
			mMesh = &InMesh;
			InputLayoutDesc desc = InMesh.mInputLayoutDesc;
			desc.addElement(1, EVertex::ATTRIBUTE4, EVertex::Float4, false, true, 1);
			desc.addElement(1, EVertex::ATTRIBUTE5, EVertex::Float4, false, true, 1);
			desc.addElement(1, EVertex::ATTRIBUTE6, EVertex::Float4, false, true, 1);
			desc.addElement(1, EVertex::ATTRIBUTE7, EVertex::Float4, false, true, 1);
			mInputLayout = RHICreateInputLayout(desc);
		}

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

		bool UpdateInstanceBuffer()
		{
			if( !mInstancedBuffer.isValid() || mInstancedBuffer->getNumElements() < mInstanceTransforms.size() )
			{
				mInstancedBuffer = RHICreateVertexBuffer(sizeof(Vector4) * 4, mInstanceTransforms.size(), BCF_CpuAccessWrite, nullptr);
				if( !mInstancedBuffer.isValid() )
				{
					LogMsg("Can't create vertex buffer!!");
					return false;
				}
			}

			Vector4* ptr = (Vector4*)RHILockBuffer(mInstancedBuffer, ELockAccess::WriteDiscard);
			if( ptr == nullptr )
			{
				return false;
			}

			for( int i = 0; i < mInstanceTransforms.size(); ++i )
			{
				ptr[0] = mInstanceTransforms[i].row(0);
				ptr[0].w = mInstanceParams[i].x;
				ptr[1] = mInstanceTransforms[i].row(1);
				ptr[1].w = mInstanceParams[i].y;
				ptr[2] = mInstanceTransforms[i].row(2);
				ptr[2].w = mInstanceParams[i].z;
				ptr[3] = mInstanceTransforms[i].row(3);
				ptr[3].w = mInstanceParams[i].w;

				ptr += 4;
			}
			RHIUnlockBuffer(mInstancedBuffer);
			return true;
		}

		void draw(RHICommandList& comandList)
		{
			if( mMesh && mMesh->mVertexBuffer.isValid() )
			{
				if( !bBufferValid )
				{
					if( UpdateInstanceBuffer() )
					{
						bBufferValid = true;
					}
					else
					{
						return;
					}
				}
				InputStreamInfo inputStreams[2];
				inputStreams[0].buffer = mMesh->mVertexBuffer;
				inputStreams[1].buffer = mInstancedBuffer;
				RHISetInputStream(comandList, mInputLayout, inputStreams, 2);
				if( mMesh->mIndexBuffer.isValid() )
				{
					RHISetIndexBuffer(comandList, mMesh->mIndexBuffer);
					RHIDrawIndexedPrimitiveInstanced(comandList, mMesh->mType, 0, mMesh->mIndexBuffer->getNumElements(), mInstanceParams.size());
				}
				else
				{
					RHIDrawPrimitiveInstanced(comandList, mMesh->mType, 0, mMesh->mVertexBuffer->getNumElements(), mInstanceParams.size());
				}
			}
		}

		void releaseRHI()
		{
			mInputLayout.release();
			mInstancedBuffer.release();
			bBufferValid = false;
		}

		Mesh* mMesh;

		RHIInputLayoutRef mInputLayout;
		RHIVertexBufferRef mInstancedBuffer;
		std::vector< Matrix4 > mInstanceTransforms;
		std::vector< Vector4 > mInstanceParams;
		bool bBufferValid = false;
	};

	class ViewFrustum
	{
	public:
		Matrix4 getPerspectiveMatrix()
		{
			if( bUseReverse )
				return ReverseZPerspectiveMatrix(mYFov, mAspect, mNear, mFar);
				       
			return PerspectiveMatrix(mYFov, mAspect, mNear, mFar);
		}

		bool  bUseReverse = false;
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


	struct TINY_API SharedAssetData
	{
		bool loadCommonShader();
		bool createSimpleMesh();

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

		SphereProgram* mProgSphere;

		Mesh   mSimpleMeshs[SimpleMeshId::NumSimpleMesh];

		void releaseRHIResource(bool bReInit = false);
	};


	class TINY_API TestRenderStageBase : public StageBase
		                               , public SharedAssetData
		                               , public IGameRenderSetup
		                               , public TextureShowManager
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
		virtual bool setupRenderSystem(ERenderSystem systemName)
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
			ShaderHelper::Get().releaseRHI();
			TextureShowManager::releaseRHI();
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

	};

}//namespace Render

#endif // TestRenderStageBase_H_E5786E12_7554_40DA_A42E_924732B7AB5D