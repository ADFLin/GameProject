#pragma once
#ifndef TestRenderStageBase_H_E5786E12_7554_40DA_A42E_924732B7AB5D
#define TestRenderStageBase_H_E5786E12_7554_40DA_A42E_924732B7AB5D

#include "Stage/TestStageHeader.h"

#include "RHI/RHICommand.h"
#include "RHI/ShaderManager.h"
#include "RHI/RenderContext.h"
#include "RHI/DrawUtility.h"
#include "RHI/MeshUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/ShaderCore.h"
#include "RHI/Scene.h"

#include "Core/ScopeExit.h"
#include "Math/PrimitiveTest.h"
#include "Serialize/SerializeFwd.h"

#include "DataCacheInterface.h"

#include "Clock.h"


namespace Render
{


	class StaticMesh;

	class TINY_API TextureShowFrame : public GWidget
	{
		typedef GWidget BaseClass;
	public:
		TextureShowFrame(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
			:BaseClass(pos, size, parent)
		{
			mID = id;
		}

		RHITexture2DRef texture;
	
		virtual void onRender() override;
		virtual bool onMouseMsg(MouseMsg const& msg) override;

	};

	bool LoadObjectMesh(StaticMesh& mesh , char const* path );
	template< class Func, class ...Args >
	bool BuildMesh(Mesh& mesh, char const* meshName, Func FuncMeshCreate, Args&& ...args)
	{
		DataCacheKey key;
		key.typeName = "MESH";
		key.version = "155CABB4-4F63-4B6C-9249-5BB7861F67E6";
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

			if( !::Global::DataCache().saveDelegate(key, MeshSave) )
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
			desc.addElement(1, Vertex::ATTRIBUTE10, Vertex::eFloat4, false, true, 1);
			desc.addElement(1, Vertex::ATTRIBUTE11, Vertex::eFloat4, false, true, 1);
			desc.addElement(1, Vertex::ATTRIBUTE12, Vertex::eFloat4, false, true, 1);
			desc.addElement(1, Vertex::ATTRIBUTE13, Vertex::eFloat4, false, true, 1);
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

			int result = mInstanceTransforms.size();
			mInstanceTransforms.push_back(xform);
			mInstanceParams.push_back(param);
			bBufferValid = false;
			return result;
		}

		bool UpdateInstanceBuffer()
		{
			if( !mInstancedBuffer.isValid() )
			{
				mInstancedBuffer = RHICreateVertexBuffer(sizeof(Vector4) * 4, mInstanceTransforms.size(), BCF_UsageDynamic, nullptr);
				if( !mInstancedBuffer.isValid() )
				{
					LogMsg("Can't create vertex buffer!!");
					return false;
				}
			}
			else
			{
				if( mInstancedBuffer->getNumElements() < mInstanceTransforms.size() )
				{
					mInstancedBuffer->resetData(sizeof(Vector4) * 4, mInstanceTransforms.size(), BCF_UsageDynamic, nullptr);
				}
			}

			Vector4* ptr = (Vector4*)RHILockBuffer(mInstancedBuffer, ELockAccess::WriteOnly);
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


	struct TINY_API SharedAssetData
	{
		bool createCommonShader();
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

		ShaderProgram mProgSphere;

		Mesh   mSimpleMeshs[SimpleMeshId::NumSimpleMesh];
	};


	class TINY_API TestRenderStageBase : public StageBase
		                               , public SharedAssetData
	{
		typedef StageBase BaseClass;
	public:
		TestRenderStageBase() {}

		ViewInfo      mView;
		ViewFrustum   mViewFrustum;
		SimpleCamera  mCamera;
		bool          mbGamePased;

		virtual bool onInit();

		virtual void onEnd();

		virtual void restart(){}
		virtual void tick() {}
		virtual void updateFrame(int frame) {}
		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
			{
				float dt = gDefaultTickTime / 1000.0;
				mView.gameTime += dt;

				if( !mbGamePased )
				{
					mView.gameTime += dt;
				}
				tick();
			}

			float dt = float(time) / 1000;
			updateFrame(frame);
		}

		virtual RHITargetName getRHITargetName() { return RHITargetName::OpenGL; }

		void initializeRenderState()
		{
			RHICommandList& commandList = RHICommandList::GetImmediateList();

			mView.frameCount += 1;

			GameWindow& window = Global::GetDrawEngine().getWindow();

			mView.rectOffset = IntVector2(0, 0);
			mView.rectSize = IntVector2(window.getWidth(), window.getHeight());

			Matrix4 matView = mCamera.getViewMatrix();
			mView.setupTransform(matView, mViewFrustum.getPerspectiveMatrix());

			glMatrixMode(GL_PROJECTION);
			glLoadMatrixf(mView.viewToClip);
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf(mView.worldToView);

			glClearColor(0.2, 0.2, 0.2, 1);
			glClearDepth(mViewFrustum.bUseReverse ? 0 : 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			RHISetViewport(commandList, mView.rectOffset.x, mView.rectOffset.y, mView.rectSize.x, mView.rectSize.y);
			RHISetDepthStencilState(commandList, mViewFrustum.bUseReverse ?
				TStaticDepthStencilState< true , ECompareFun::Greater >::GetRHI() :
				TStaticDepthStencilState<>::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

		}

		bool onMouse(MouseMsg const& msg)
		{
			static Vec2i oldPos = msg.getPos();

			if( msg.onLeftDown() )
			{
				oldPos = msg.getPos();
			}
			if( msg.onMoving() && msg.isLeftDown() )
			{
				float rotateSpeed = 0.01;
				Vector2 off = rotateSpeed * Vector2(msg.getPos() - oldPos);
				mCamera.rotateByMouse(off.x, off.y);
				oldPos = msg.getPos();
			}

			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}

		bool onKey(unsigned key, bool isDown)
		{
			if( !isDown )
				return false;
			switch( key )
			{
			case 'W': mCamera.moveFront(1); break;
			case 'S': mCamera.moveFront(-1); break;
			case 'D': mCamera.moveRight(1); break;
			case 'A': mCamera.moveRight(-1); break;
			case 'Z': mCamera.moveUp(0.5); break;
			case 'X': mCamera.moveUp(-0.5); break;
			case Keyboard::eR: restart(); break;
			case Keyboard::eF2:
				{
					ShaderManager::Get().reloadAll();
					//initParticleData();
				}
				break;
			}
			return false;
		}


		virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch( id )
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}


		void drawLightPoints(RHICommandList& commandList, ViewInfo& view, TArrayView< LightInfo > lights);

		void handleShowTexture(char const* name)
		{
			auto iter = mTextureMap.find(name);
			if( iter != mTextureMap.end() )
			{
				TextureShowFrame* textureFrame = new TextureShowFrame(UI_ANY, Vec2i(0, 0), Vec2i(200, 200), nullptr);
				textureFrame->texture = iter->second;
				::Global::GUI().addWidget(textureFrame);
			}
		}

		void registerTexture(char const* name, RHITexture2D& texture)
		{
			mTextureMap.emplace(name, &texture);
		}

		std::unordered_map< HashString, RHITexture2DRef > mTextureMap;



	};

}//namespace Render

#endif // TestRenderStageBase_H_E5786E12_7554_40DA_A42E_924732B7AB5D