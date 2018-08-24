#include "TestStageHeader.h"


#include "RHI/RHICommon.h"
#include "RHI/ShaderCompiler.h"
#include "RHI/RenderContext.h"
#include "RHI/DrawUtility.h"
#include "RHI/MeshUtility.h"


namespace RenderGL
{
	struct alignas(16) ParticleData
	{
		DECLARE_BUFFER_STRUCT(ParticleDataBlock);

		Vector3 pos;
		float   lifeTime;
		Vector3 velocity;
		float   size;
		Vector4 color;
		Vector3  dumy;
		float   lifeTimeScale;	
	};

	struct alignas(16) ParticleParameters
	{
		DECLARE_BUFFER_STRUCT(ParticleParamBlock);
		Vector3  gravity;
		uint32   numParticle;
		Vector4  dynamic;
	};

	struct alignas(16) ParticleInitParameters
	{
		DECLARE_BUFFER_STRUCT(ParticleInitParamBlock);
		Vector3 posMin;
		Vector3 posMax;
	};

	struct alignas(16) CollisionPrimitive
	{
		DECLARE_BUFFER_STRUCT(CollisionPrimitiveBlock);
		Vector3 velocity;
		int32   type;
		Vector4 param;
	};

	template< class T , class ResourceType >
	class TStructuredBufferBase
	{
	public:

		void releaseResources()
		{
			mResource->release();
		}


		uint32 getElementNum() { return mResource->getSize() / sizeof(T); }

		ResourceType* getRHI() { return mResource; }
		TRefCountPtr<ResourceType> mResource;
	};


	template< class T >
	class TStructuredUniformBuffer : public TStructuredBufferBase< T , RHIUniformBuffer >
	{
	public:
		bool initializeResource(uint32 numElement)
		{
			mResource = RHICreateUniformBuffer(sizeof(T) , numElement );
			if( !mResource.isValid() )
				return false;
			return true;
		}

		T*   lock()
		{
			return (T*)OpenGLCast::To( mResource )->lock(ELockAccess::WriteOnly);
		}
		void unlock()
		{
			OpenGLCast::To( mResource )->unlock();
		}
	};

	template< class T >
	class TStructuredStorageBuffer : public TStructuredBufferBase< T , RHIVertexBuffer >
	{
	public:
		bool initializeResource(uint32 numElement)
		{
			mResource = RHICreateVertexBuffer(sizeof(T) , numElement );
			if( !mResource.isValid() )
				return false;
			return true;
		}

		T*   lock()
		{
			return (T*)RHILockBuffer( mResource , ELockAccess::WriteOnly);
		}
		void unlock()
		{
			RHIUnlockBuffer( mResource );
		}
	};

	class ParticleSimBaseProgram : public GlobalShaderProgram
	{
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/ParticleSimulation";
		}

		void setParameters(
			TStructuredStorageBuffer< ParticleData >& particleBuffer,
			TStructuredUniformBuffer< ParticleParameters >& paramBuffer,
			TStructuredUniformBuffer< ParticleInitParameters >& paramInitBuffer)
		{
			setStructuredBufferT< ParticleData >(*particleBuffer.getRHI());
			setStructuredBufferT< ParticleInitParameters >(*paramInitBuffer.getRHI());
			setStructuredBufferT< ParticleParameters >(*paramBuffer.getRHI());
		}
	};

	class ParticleInitProgram : public ParticleSimBaseProgram
	{
		DECLARE_GLOBAL_SHADER(ParticleInitProgram);
		typedef ParticleSimBaseProgram BaseClass;


		void bindParameters(ShaderParameterMap& parameterMap)
		{
		}

		void setParameters(
			TStructuredStorageBuffer< ParticleData >& particleBuffer,
			TStructuredUniformBuffer< ParticleParameters >& paramBuffer,
			TStructuredUniformBuffer< ParticleInitParameters >& paramInitBuffer)
		{
			BaseClass::setParameters(particleBuffer, paramBuffer, paramInitBuffer);
		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
		}

		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ Shader::eCompute , SHADER_ENTRY(MainInitCS) },
				{ Shader::eEmpty , nullptr },
			};
			return entries;
		}
	};


	class ParticleUpdateProgram : public ParticleSimBaseProgram
	{

		DECLARE_GLOBAL_SHADER(ParticleUpdateProgram);
		typedef ParticleSimBaseProgram BaseClass;

		void bindParameters(ShaderParameterMap& parameterMap)
		{
			BaseClass::bindParameters(parameterMap);
			parameterMap.bind( mParamDeltaTime , SHADER_PARAM(DeltaTime));
			parameterMap.bind( mParamNumCollisionPrimitive , SHADER_PARAM(NumCollisionPrimitive));
		}

		void setParameters(
			TStructuredStorageBuffer< ParticleData >& particleBuffer,
			TStructuredUniformBuffer< ParticleParameters >& paramBuffer,
			TStructuredUniformBuffer< ParticleInitParameters >& paramInitBuffer,
			float deltaTime , int32 numCol )
		{
			BaseClass::setParameters(particleBuffer, paramBuffer, paramInitBuffer);
			setParam(mParamDeltaTime, deltaTime);
			setParam(mParamNumCollisionPrimitive, numCol);
		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);

		}

		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ Shader::eCompute , SHADER_ENTRY(MainUpdateCS) },
				{ Shader::eEmpty , nullptr },
			};
			return entries;
		}

		ShaderParameter mParamNumCollisionPrimitive;
		ShaderParameter mParamDeltaTime;
	};


	IMPLEMENT_GLOBAL_SHADER(ParticleInitProgram);
	IMPLEMENT_GLOBAL_SHADER(ParticleUpdateProgram);

	class SimpleSpriteParticleProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_GLOBAL_SHADER(SimpleSpriteParticleProgram);
		typedef GlobalShaderProgram BaseClass;

		void bindParameters(ShaderParameterMap& parameterMap)
		{
			parameterMap.bind(mParamBaseTexture, SHADER_PARAM(BaseTexture));
		}

		void setParameters(
			TStructuredStorageBuffer< ParticleData >& particleBuffer , RHITexture2D& texture )
		{
			setStructuredBufferT< ParticleData >(*particleBuffer.getRHI());
			setTexture(mParamBaseTexture, texture);
		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/SimpleSpriteParticle";
		}

		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(MainVS) },
				{ Shader::eGeometry , SHADER_ENTRY(MainGS) },
				{ Shader::ePixel , SHADER_ENTRY(MainPS) },
				{ Shader::eEmpty , nullptr },
			};
			return entries;
		}

		ShaderParameter mParamBaseTexture;
	};

	IMPLEMENT_GLOBAL_SHADER(SimpleSpriteParticleProgram);

	class SplineProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_GLOBAL_SHADER(SplineProgram);
		typedef GlobalShaderProgram BaseClass;

		static bool constexpr UseTesselation = true;

		void bindParameters(ShaderParameterMap& parameterMap)
		{

		}

		void setParameters()
		{

		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM(USE_TESSELLATION), UseTesselation);
		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/Spline";
		}

		static ShaderEntryInfo const* GetShaderEntries()
		{
			if ( UseTesselation )
			{
				static ShaderEntryInfo entriesWithTesselation[] =
				{
					{ Shader::eVertex , SHADER_ENTRY(MainVS) },
					{ Shader::eHull   , SHADER_ENTRY(MainHS) },
					{ Shader::eDomain , SHADER_ENTRY(MainDS) },
					{ Shader::ePixel , SHADER_ENTRY(MainPS) },
					{ Shader::eEmpty , nullptr },
				};
				return entriesWithTesselation;
			}

			static ShaderEntryInfo entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(MainVS) },
				{ Shader::ePixel , SHADER_ENTRY(MainPS) },
				{ Shader::eEmpty , nullptr },
			};
			return entries;
		}
	};

	IMPLEMENT_GLOBAL_SHADER(SplineProgram);

	struct alignas(16) WaterVertexData
	{
		//Vector3 normal;
		Vector2 normal;
		float h;
		float v;
		
	};

	class WaterSimulationProgram : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(WaterSimulationProgram);
		typedef GlobalShaderProgram BaseClass;

		void bindParameters(ShaderParameterMap& parameterMap)
		{
			BaseClass::bindParameters(parameterMap);
			parameterMap.bind(mParamDataIn, SHADER_PARAM(WaterDataInBlock));
			parameterMap.bind(mParamDataOut, SHADER_PARAM(WaterDataOutBlock));
			parameterMap.bind(mParamWaterParam, SHADER_PARAM(WaterParam));
			parameterMap.bind(mParamTileNum, SHADER_PARAM(TileNum));
		}

		void setParameters( Vector4 const& param , int TileNum , RHIVertexBuffer& DataIn , RHIVertexBuffer& DataOut )
		{
			setStorageBuffer(mParamDataIn, DataIn);
			setStorageBuffer(mParamDataOut, DataOut);
			setParam(mParamWaterParam, param);
			setParam(mParamTileNum, TileNum);
		}


		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);

		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/WaterSimulation";
		}

		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ Shader::eCompute , SHADER_ENTRY(MainUpdateCS) },
				{ Shader::eEmpty , nullptr },
			};
			return entries;
		}

		ShaderParameter mParamDataIn;
		ShaderParameter mParamDataOut;
		ShaderParameter mParamWaterParam;
		ShaderParameter mParamTileNum;
	};

	class WaterUpdateNormalProgram : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(WaterUpdateNormalProgram);
		typedef GlobalShaderProgram BaseClass;

		void bindParameters(ShaderParameterMap& parameterMap)
		{
			BaseClass::bindParameters(parameterMap);
			parameterMap.bind(mParamData, SHADER_PARAM(WaterDataOutBlock));
			parameterMap.bind(mParamTileNum, SHADER_PARAM(TileNum));
		}

		void setParameters(int TileNum, RHIVertexBuffer& Data)
		{
			setStorageBuffer(mParamData, Data);
			setParam(mParamTileNum, TileNum);
		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);

		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/WaterSimulation";
		}

		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ Shader::eCompute , SHADER_ENTRY(MainUpdateNormal) },
				{ Shader::eEmpty , nullptr },
			};
			return entries;
		}

		ShaderParameter mParamData;
		ShaderParameter mParamTileNum;
	};


	class WaterProgram : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(WaterProgram);
		typedef GlobalShaderProgram BaseClass;

		void bindParameters(ShaderParameterMap& parameterMap)
		{
			BaseClass::bindParameters(parameterMap);
			parameterMap.bind(mParamDataIn, SHADER_PARAM(WaterDataInBlock));
			parameterMap.bind(mParamTileNum, SHADER_PARAM(TileNum));
		}

		void setParameters(int TileNum, RHIVertexBuffer& DataIn)
		{
			setStorageBuffer(mParamDataIn, DataIn);
			setParam(mParamTileNum, TileNum);
		}


		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);

		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/Water";
		}

		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(MainVS) },
				{ Shader::ePixel , SHADER_ENTRY(MainPS) },
				{ Shader::eEmpty , nullptr },
			};
			return entries;
		}

		ShaderParameter mParamDataIn;
		ShaderParameter mParamTileNum;
	};

	IMPLEMENT_GLOBAL_SHADER(WaterSimulationProgram);
	IMPLEMENT_GLOBAL_SHADER(WaterUpdateNormalProgram);
	IMPLEMENT_GLOBAL_SHADER(WaterProgram);

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


	struct FloatWidgetPropery
	{
		static float Get(GSlider* widget)
		{
			return widget->getValue();
		}
		static void Set(GSlider* widget, float value)
		{
			return widget->setValue(int(value));
		}
		static float Get(GTextCtrl* widget)
		{
			return std::atof(widget->getValue());
		}
		static void Set(GTextCtrl* widget, float value)
		{
			return widget->setValue(std::to_string(value).c_str());
		}
	};

	struct IPropertyBinder
	{
		virtual ~IPropertyBinder() {}
	};

	struct FloatPropertyBinder : public IPropertyBinder
	{
		FloatPropertyBinder(GSlider* widget, float& valueRef, float min, float max )
			:widgetBinded( widget ) , mValueRef(valueRef)
		{
			float constexpr scale = 0.001;
			float len = max - min;
			widget->setRange(0, len / scale);
			FloatWidgetPropery::Set(widget, ( mValueRef - min ) / scale );
			widget->onEvent = [this,scale, min](int event, GWidget* widget)
			{
				mValueRef = min + scale * FloatWidgetPropery::Get(widget->cast<GSlider>());
				return false;
			};
		}


		~FloatPropertyBinder()
		{
			widgetBinded->onEvent = 0;
		}
		GWidget* widgetBinded;
		float&   mValueRef;
	};


	struct GPUParticleData
	{

		bool initialize()
		{
			
			VERIFY_INITRESULT(mProgInit = ShaderManager::Get().getGlobalShaderT< ParticleInitProgram >(true));
			VERIFY_INITRESULT(mProgUpdate = ShaderManager::Get().getGlobalShaderT< ParticleUpdateProgram >(true));

			VERIFY_INITRESULT(mProgParticleRender = ShaderManager::Get().getGlobalShaderT< SimpleSpriteParticleProgram >(true));

			VERIFY_INITRESULT(mParticleBuffer.initializeResource(1000));
			VERIFY_INITRESULT(mInitParamBuffer.initializeResource(1));
			VERIFY_INITRESULT(mParamBuffer.initializeResource(1));

			{
				ParticleParameters& parameters = *mParamBuffer.lock();
				parameters.numParticle = mParticleBuffer.getElementNum();
				parameters.gravity = Vector3(0, 0, -9.8);
				parameters.dynamic = Vector4(1, 1, 1, 0);
				mParamBuffer.unlock();
			}

			{
				ParticleInitParameters& parameters = *mInitParamBuffer.lock();
				parameters.posMax = Vector3(10, 10, 1);
				parameters.posMin = Vector3(-10, -10, -1);
				mInitParamBuffer.unlock();
			}

			{

				{
					CollisionPrimitive primitive;
					primitive.type = 0;
					primitive.param = Vector4(0, 0, 1, 0);
					mPrimitives.push_back(primitive);
				}
				{
					CollisionPrimitive primitive;
					primitive.type = 1;
					primitive.param = Vector4(5, 0, 0, 10);
					mPrimitives.push_back(primitive);
				}

				{
					CollisionPrimitive primitive;
					primitive.type = 1;
					primitive.param = Vector4(-5, 0, 5, 15);
					mPrimitives.push_back(primitive);
				}
				VERIFY_INITRESULT(mCollisionPrimitiveBuffer.initializeResource(mPrimitives.size()));
				CollisionPrimitive* pData = mCollisionPrimitiveBuffer.lock();
				memcpy(pData, &mPrimitives[0], mPrimitives.size() * sizeof(CollisionPrimitive));
				mCollisionPrimitiveBuffer.unlock();

				return true;
			}
		}


		void initParticleData()
		{
			GL_BIND_LOCK_OBJECT(mProgInit);
			mProgInit->setParameters(mParticleBuffer, mParamBuffer, mInitParamBuffer);
			glDispatchCompute(mParticleBuffer.getElementNum(), 1, 1);
		}

		void updateParticleData(float dt)
		{
			GL_BIND_LOCK_OBJECT(mProgUpdate);
			mProgUpdate->setParameters(mParticleBuffer, mParamBuffer, mInitParamBuffer, dt, mPrimitives.size());
			mProgUpdate->setStructuredBufferT<CollisionPrimitive>(*mCollisionPrimitiveBuffer.getRHI());
			glDispatchCompute(mParticleBuffer.getElementNum(), 1, 1);
		}

		TStructuredUniformBuffer< ParticleInitParameters > mInitParamBuffer;
		TStructuredUniformBuffer< ParticleParameters >     mParamBuffer;
		TStructuredStorageBuffer< ParticleData > mParticleBuffer;
		TStructuredUniformBuffer< CollisionPrimitive > mCollisionPrimitiveBuffer;

		ParticleInitProgram* mProgInit;
		ParticleUpdateProgram* mProgUpdate;
		SimpleSpriteParticleProgram* mProgParticleRender;

		std::vector< CollisionPrimitive > mPrimitives;
	};



	class GPUParticleTestStage : public StageBase
		                       , public GPUParticleData
	{
		typedef StageBase BaseClass;
	public:
		GPUParticleTestStage() {}


		SplineProgram* mProgSpline;


		int mIndexWaterBufferUsing = 0;
		TStructuredStorageBuffer< WaterVertexData > mWaterDataBuffers[2];
		WaterSimulationProgram* mProgWaterSimulation;
		WaterUpdateNormalProgram* mProgWaterUpdateNormal;
		WaterProgram* mProgWater;
		

		RHITexture2DRef mTexture;
		ViewFrustum mViewFrustum;
		SimpleCamera  mCamera;

		Mesh mTileMesh;
		int  mTileNum = 500;

		
		std::vector < std::unique_ptr< IPropertyBinder > > mBinders;
		float mWaterSpeed = 10;

		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;

			if( !::Global::GetDrawEngine()->startOpenGL() )
				return false;

			VERIFY_INITRESULT(GPUParticleData::initialize());
			
			VERIFY_INITRESULT(mProgSpline = ShaderManager::Get().getGlobalShaderT< SplineProgram >(true));

			mTexture = RHIUtility::LoadTexture2DFromFile("Texture/star.png");
			VERIFY_INITRESULT(mTexture.isValid());
			VERIFY_INITRESULT(ShaderManager::Get().loadFileSimple(mProgSphere, "Shader/Game/Sphere"));
			Vector3 v[] =
			{
				Vector3(1,1,0),
				Vector3(-1,1,0) ,
				Vector3(-1,-1,0),
				Vector3(1,-1,0),
			};
			int   idx[6] = { 0 , 1 , 2 , 0 , 2 , 3 };
			mSpherePlane.mInputLayoutDesc.addElement(Vertex::ePosition, Vertex::eFloat3);
			VERIFY_INITRESULT(mSpherePlane.createRHIResource(&v[0], 4, &idx[0], 6, true));

			VERIFY_INITRESULT(MeshBuild::Tile(mTileMesh, mTileNum - 1, 100, false));
			VERIFY_INITRESULT(mWaterDataBuffers[0].initializeResource(mTileNum * mTileNum));
			VERIFY_INITRESULT(mWaterDataBuffers[1].initializeResource(mTileNum * mTileNum));
			VERIFY_INITRESULT(mProgWaterSimulation = ShaderManager::Get().getGlobalShaderT< WaterSimulationProgram >(true));
			VERIFY_INITRESULT(mProgWaterUpdateNormal = ShaderManager::Get().getGlobalShaderT< WaterUpdateNormalProgram >(true));
			VERIFY_INITRESULT(mProgWater = ShaderManager::Get().getGlobalShaderT< WaterProgram >(true));
			{
				auto pWaterData = mWaterDataBuffers[0].lock();
				for( int i = 0; i < mTileNum * mTileNum; ++i )
				{
					int x = i % mTileNum;
					int y = i / mTileNum;
					pWaterData[i].h = 0;
					pWaterData[i].v = 0;

					float dist = Distance(Vector2(x, y), 0.5 * Vector2(mTileNum, mTileNum));
#if 0
					if( dist < 30 )
					{
						pWaterData[i].h = 10 * ( 30 - dist ) / 30 ;
					}
#else
					pWaterData[i].h = 10 * Math::Exp(-Math::Squre(dist / 10));
#endif
					//pWaterData[i].normal = Vector3(0, 0, 1);
				}
				mWaterDataBuffers[0].unlock();
			}


			Vec2i screenSize = ::Global::GetDrawEngine()->getScreenSize();
			mViewFrustum.mNear = 0.01;
			mViewFrustum.mFar = 800.0;
			mViewFrustum.mAspect = float(screenSize.x) / screenSize.y;
			mViewFrustum.mYFov = Math::Deg2Rad(60 / mViewFrustum.mAspect);

			mCamera.setPos(Vector3(20, 0, 20));
			mCamera.setViewDir(Vector3(-1, 0, 0), Vector3(0, 0, 1));
			::Global::GUI().cleanupWidget();

			auto devFrame = WidgetUtility::CreateDevFrame();
			auto slider = devFrame->addSlider(UI_ANY);
			mBinders.push_back(std::make_unique< FloatPropertyBinder >(slider, mWaterSpeed , 0 , 10 ) );
			restart();

			return true;
		}

		virtual void onEnd()
		{
			BaseClass::onEnd();
		}

		void drawSphere(Vector3 const& pos, float radius)
		{
			RHISetBlendState(TStaticBlendState<>::GetRHI());

			GL_BIND_LOCK_OBJECT(mProgSphere);
			mView.setupShader(mProgSphere);
			mProgSphere.setParam(SHADER_PARAM(Sphere.radius), radius);
			mProgSphere.setParam(SHADER_PARAM(Sphere.localPos), pos);
			
			mSpherePlane.draw();
		}

		void upateWaterData(float dt)
		{
			mIndexWaterBufferUsing = 1 - mIndexWaterBufferUsing;
			{
				GL_BIND_LOCK_OBJECT(mProgWaterSimulation);
				Vector4 waterParam;
				waterParam.x = dt;
				waterParam.y = mWaterSpeed * dt * mTileNum / 10;
				waterParam.z = 1;
				mProgWaterSimulation->setParameters(waterParam, mTileNum, *mWaterDataBuffers[1 - mIndexWaterBufferUsing].getRHI(), *mWaterDataBuffers[mIndexWaterBufferUsing].getRHI());
				glDispatchCompute(mTileNum, mTileNum, 1);
			}
			{
				GL_BIND_LOCK_OBJECT(mProgWaterUpdateNormal);
				mProgWaterUpdateNormal->setParameters(mTileNum, *mWaterDataBuffers[mIndexWaterBufferUsing].getRHI());
				glDispatchCompute(mTileNum, mTileNum, 1);

			}
		}
		void restart() 
		{
			initParticleData();
		}
		void tick() {}
		void updateFrame(int frame) {}

		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			float dt = float(time) / 1000;
			updateParticleData(dt);
			upateWaterData(dt);

			updateFrame(frame);
		}

		ShaderProgram mProgSphere;
		Mesh mSpherePlane;

		ShaderProgram mProgSimpleParticle;
		ViewInfo mView;



		void onRender(float dFrame)
		{
			Graphics2D& g = Global::GetGraphics2D();


			GameWindow& window = Global::GetDrawEngine()->getWindow();

			mView.gameTime = 0;
			mView.realTime = 0;
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

			{
				GL_BIND_LOCK_OBJECT(mProgWater);
				mProgWater->setParameters(mTileNum, *mWaterDataBuffers[mIndexWaterBufferUsing].getRHI());
				mView.setupShader(*mProgWater);
				//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				mTileMesh.drawShader(LinearColor(1, 0, 0));
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}

			for( auto const& primitive : mPrimitives )
			{
				switch( primitive.type )
				{
				case 0:
					break;
				case 1:
					drawSphere(primitive.param.xyz(), primitive.param.w);
					break;
				}
			}


			{
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

				RHISetBlendState(TStaticBlendState< CWM_RGBA, Blend::eSrcAlpha, Blend::eOne >::GetRHI());
				GL_BIND_LOCK_OBJECT(mProgParticleRender);
				mView.setupShader(*mProgParticleRender);
				mProgParticleRender->setParameters(mParticleBuffer, *mTexture);
				glDrawArrays(GL_POINTS , 0, mParticleBuffer.getElementNum() - 2);
			}



			{
				int width = ::Global::GetDrawEngine()->getScreenWidth();
				int height = ::Global::GetDrawEngine()->getScreenHeight();

				RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
				RHISetBlendState(TStaticBlendState<>::GetRHI());
				struct MyVertex
				{
					Vector2 pos;
					Vector3 color;
				};

				MyVertex vertices[] =
				{
					Vector2(100, 100), Vector3(1, 0, 0),
					Vector2(400, 100), Vector3(0, 1, 0),
					Vector2(100, 400),	Vector3(0, 0, 1),
					Vector2(400, 400), Vector3(1, 1, 1),
				};
				GL_BIND_LOCK_OBJECT(mProgSpline);
				mProgSpline->setParam(SHADER_PARAM(XForm), OrthoMatrix(0, width, 0, height, -100, 100));
				mProgSpline->setParam(SHADER_PARAM(TessOuter0), TessFactor2);
				mProgSpline->setParam(SHADER_PARAM(TessOuter1), TessFactor);
				glPatchParameteri(GL_PATCH_VERTICES, 4);
				TRenderRT< RTVF_XY | RTVF_C > ::DrawShader( SplineProgram::UseTesselation ? PrimitiveType::Patchs : PrimitiveType::LineStrip , vertices, 4);
	
			}

			g.beginRender();

			g.endRender();
		}

		int TessFactor = 5;
		int TessFactor2 = 1;
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
			case Keyboard::eLEFT: --TessFactor; break;
			case Keyboard::eRIGHT: ++TessFactor; break;
			case Keyboard::eDOWN: --TessFactor2; break;
			case Keyboard::eUP: ++TessFactor2; break;
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

	REGISTER_STAGE2("GPU Particle Test", GPUParticleTestStage, EStageGroup::FeatureDev, 1);
}
