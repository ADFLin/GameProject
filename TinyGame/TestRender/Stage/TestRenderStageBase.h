#include "Stage/TestStageHeader.h"

#include "RHI/RHICommon.h"
#include "RHI/ShaderCompiler.h"
#include "RHI/RenderContext.h"
#include "RHI/DrawUtility.h"
#include "RHI/MeshUtility.h"
#include "RHI/GpuProfiler.h"

#include "Core/ScopeExit.h"
#include "Math/PrimitiveTest.h"
#include "Serialize/SerializeFwd.h"

#include "DataCacheInterface.h"

#define GPU_BUFFER_ALIGN alignas(16)

namespace Render
{
	class StaticMesh;

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
		bool initializeResource(uint32 numElement)
		{
			mResource = RHICreateUniformBuffer(sizeof(T), numElement);
			if( !mResource.isValid() )
				return false;
			return true;
		}


	};

	template< class T >
	class TStructuredStorageBuffer : public TStructuredBufferBase< T, RHIVertexBuffer >
	{
	public:
		bool initializeResource(uint32 numElement)
		{
			mResource = RHICreateVertexBuffer(sizeof(T), numElement);
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
			return PerspectiveMatrix(mYFov, mAspect, mNear, mFar);
		}

		float mNear;
		float mFar;
		float mYFov;
		float mAspect;
	};


	class TINY_API TestRenderStageBase : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestRenderStageBase() {}

		ViewInfo      mView;
		ViewFrustum   mViewFrustum;
		SimpleCamera  mCamera;
		bool          mbGamePased;

		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;

			if( !::Global::GetDrawEngine().initializeRHI( RHITargetName::OpenGL , 1 ) )
				return false;

			mView.gameTime = 0;
			mView.realTime = 0;

			mbGamePased = false;

			Vec2i screenSize = ::Global::GetDrawEngine().getScreenSize();
			mViewFrustum.mNear = 0.01;
			mViewFrustum.mFar = 800.0;
			mViewFrustum.mAspect = float(screenSize.x) / screenSize.y;
			mViewFrustum.mYFov = Math::Deg2Rad(60 / mViewFrustum.mAspect);

			mCamera.setPos(Vector3(20, 0, 20));
			mCamera.setViewDir(Vector3(-1, 0, 0), Vector3(0, 0, 1));

			return true;
		}

		virtual void onEnd()
		{
			::Global::GetDrawEngine().shutdownRHI(true);
			BaseClass::onEnd();
		}

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
			glClearDepth(1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			RHISetViewport(mView.rectOffset.x, mView.rectOffset.y, mView.rectSize.x, mView.rectSize.y);
			RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI());
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
	protected:
	};

}//namespace Render