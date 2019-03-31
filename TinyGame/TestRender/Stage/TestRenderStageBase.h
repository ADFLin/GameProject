#pragma once
#ifndef TestRenderStageBase_H_E5786E12_7554_40DA_A42E_924732B7AB5D
#define TestRenderStageBase_H_E5786E12_7554_40DA_A42E_924732B7AB5D

#include "Stage/TestStageHeader.h"

#include "RHI/RHICommon.h"
#include "RHI/ShaderCompiler.h"
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

	template< class T, class ResourceType >
	class TStructuredBufferBase
	{
	public:

		void releaseResources()
		{
			mResource->release();
		}


		uint32 getElementNum() { return mResource->getSize() / sizeof(T); }

		ResourceType* getRHI() { return mResource; }

		T*   lock()
		{
			return (T*)RHILockBuffer(mResource, ELockAccess::WriteOnly);
		}
		void unlock()
		{
			RHIUnlockBuffer(mResource);
		}

		TRefCountPtr<ResourceType> mResource;
	};


	template< class T >
	class TStructuredUniformBuffer : public TStructuredBufferBase< T, RHIUniformBuffer >
	{
	public:
		bool initializeResource(uint32 numElement, uint32 creationFlags = BCF_DefalutValue)
		{
			mResource = RHICreateUniformBuffer(sizeof(T), numElement, creationFlags);
			if( !mResource.isValid() )
				return false;
			return true;
		}

	};

	template< class T >
	class TStructuredStorageBuffer : public TStructuredBufferBase< T, RHIVertexBuffer >
	{
	public:
		bool initializeResource(uint32 numElement , uint32 creationFlags = BCF_DefalutValue )
		{
			mResource = RHICreateVertexBuffer(sizeof(T), numElement , creationFlags );
			if( !mResource.isValid() )
				return false;
			return true;
		}
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

		void initializeRenderState()
		{
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

			RHISetViewport(mView.rectOffset.x, mView.rectOffset.y, mView.rectSize.x, mView.rectSize.y);
			RHISetDepthStencilState( mViewFrustum.bUseReverse ?
				TStaticDepthStencilState< true , ECompareFun::Greater >::GetRHI() :
				TStaticDepthStencilState<>::GetRHI());
			RHISetBlendState(TStaticBlendState<>::GetRHI());

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


		void drawLightPoints(ViewInfo& view, TArrayView< LightInfo > lights);

		void executeShowTexture(char const* name)
		{
			auto iter = mTextureMap.find(name);
			if( iter != mTextureMap.end() )
			{
				TextureShowFrame* textureFrame = new TextureShowFrame(UI_ANY, Vec2i(0, 0), Vec2i(200, 200), nullptr);
				textureFrame->texture = iter->second;
				::Global::GUI().addWidget(textureFrame);
			}
		}

		void reigsterTexture(char const* name, RHITexture2D& texture)
		{
			mTextureMap.emplace(name, &texture);
		}

		std::unordered_map< HashString, RHITexture2DRef > mTextureMap;



	};

}//namespace Render

#endif // TestRenderStageBase_H_E5786E12_7554_40DA_A42E_924732B7AB5D