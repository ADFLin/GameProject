#include "TestRenderStageBase.h"

#include "RHI/RHICommand.h"
#include "Renderer/MeshBuild.h"


namespace Render
{

	struct GPU_ALIGN ParticleData
	{
		DECLARE_BUFFER_STRUCT(ParticleDataBlock, Particles);

		Vector3 pos;
		float   lifeTime;
		Vector3 velocity;
		float   size;
		Vector4 color;
		Vector3  dumy;
		float   lifeTimeScale;	
	};

	struct GPU_ALIGN ParticleParameters
	{
		DECLARE_UNIFORM_BUFFER_STRUCT(ParticleParamBlock);
		Vector3  gravity;
		uint32   numParticle;
		Vector4  dynamic;
	};

	struct GPU_ALIGN ParticleInitParameters
	{
		DECLARE_UNIFORM_BUFFER_STRUCT(ParticleInitParamBlock);
		Vector3 posMin;
		Vector3 posMax;
	};

	struct GPU_ALIGN CollisionPrimitive
	{
		DECLARE_UNIFORM_BUFFER_STRUCT(CollisionPrimitiveBlock);
		Vector3 velocity;
		int32   type;
		Vector4 param;
	};

	class ParticleSimBaseProgram : public GlobalShaderProgram
	{
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/ParticleSimulation";
		}

		void setParameters(RHICommandList& commandList,
			TStructuredBuffer< ParticleData >& particleBuffer,
			TStructuredBuffer< ParticleParameters >& paramBuffer,
			TStructuredBuffer< ParticleInitParameters >& paramInitBuffer)
		{
			setStructuredStorageBufferT< ParticleData >(commandList, *particleBuffer.getRHI());
			setStructuredUniformBufferT< ParticleInitParameters >(commandList, *paramInitBuffer.getRHI());
			setStructuredUniformBufferT< ParticleParameters >(commandList, *paramBuffer.getRHI());
		}
	};

	class ParticleInitProgram : public ParticleSimBaseProgram
	{
		DECLARE_SHADER_PROGRAM(ParticleInitProgram, Global);
		using BaseClass = ParticleSimBaseProgram;


		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
		}

		void setParameters(RHICommandList& commandList,
			TStructuredBuffer< ParticleData >& particleBuffer,
			TStructuredBuffer< ParticleParameters >& paramBuffer,
			TStructuredBuffer< ParticleInitParameters >& paramInitBuffer)
		{
			BaseClass::setParameters(commandList, particleBuffer, paramBuffer, paramInitBuffer);
		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ EShader::Compute , SHADER_ENTRY(MainInitCS) },
			};
			return entries;
		}
	};


	class ParticleUpdateProgram : public ParticleSimBaseProgram
	{

		DECLARE_SHADER_PROGRAM(ParticleUpdateProgram, Global);
		using BaseClass = ParticleSimBaseProgram;

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BaseClass::bindParameters(parameterMap);
			mParamDeltaTime.bind(parameterMap, SHADER_PARAM(DeltaTime));
			mParamNumCollisionPrimitive.bind(parameterMap, SHADER_PARAM(NumCollisionPrimitive));
		}

		void setParameters(RHICommandList& commandList,
			TStructuredBuffer< ParticleData >& particleBuffer,
			TStructuredBuffer< ParticleParameters >& paramBuffer,
			TStructuredBuffer< ParticleInitParameters >& paramInitBuffer,
			float deltaTime , int32 numCol )
		{
			BaseClass::setParameters(commandList, particleBuffer, paramBuffer, paramInitBuffer);
			setParam(commandList, mParamDeltaTime, deltaTime);
			setParam(commandList, mParamNumCollisionPrimitive, numCol);
		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);

		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ EShader::Compute , SHADER_ENTRY(MainUpdateCS) },
			};
			return entries;
		}

		ShaderParameter mParamNumCollisionPrimitive;
		ShaderParameter mParamDeltaTime;
	};


	IMPLEMENT_SHADER_PROGRAM(ParticleInitProgram);
	IMPLEMENT_SHADER_PROGRAM(ParticleUpdateProgram);

	class SimpleSpriteParticleProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(SimpleSpriteParticleProgram, Global);
		using BaseClass = GlobalShaderProgram;

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			mParamBaseTexture.bind(parameterMap, SHADER_PARAM(BaseTexture));
		}

		void setParameters(RHICommandList& commandList,
			TStructuredBuffer< ParticleData >& particleBuffer , RHITexture2D& texture )
		{
			setStructuredStorageBufferT< ParticleData >(commandList, *particleBuffer.getRHI());
			setTexture(commandList, mParamBaseTexture, texture);
		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/SimpleSpriteParticle";
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Geometry , SHADER_ENTRY(MainGS) },
				{ EShader::Pixel , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		ShaderParameter mParamBaseTexture;
	};

	IMPLEMENT_SHADER_PROGRAM(SimpleSpriteParticleProgram);

	class SplineProgram : public GlobalShaderProgram
	{
	public:
		using BaseClass = GlobalShaderProgram;

		static bool constexpr UseTesselation = true;

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{

		}

		void setParameters()
		{

		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/Spline";
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			if (UseTesselation)
			{
				static ShaderEntryInfo entriesWithTesselation[] =
				{
					{ EShader::Vertex , SHADER_ENTRY(MainVS) },
					{ EShader::Hull   , SHADER_ENTRY(MainHS) },
					{ EShader::Domain , SHADER_ENTRY(MainDS) },
					{ EShader::Pixel , SHADER_ENTRY(MainPS) },
				};
				return entriesWithTesselation;
			}

			static ShaderEntryInfo entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}
	};

	template< int SplineType >
	class TSplineProgram : public SplineProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(TSplineProgram, Global);
		using BaseClass = SplineProgram;

		static bool constexpr UseTesselation = true;

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM(USE_TESSELLATION), UseTesselation);
			option.addDefine(SHADER_PARAM(SPLINE_TYPE), SplineType);
		}
	};

	IMPLEMENT_SHADER_PROGRAM_T(template<>, TSplineProgram<0>);
	IMPLEMENT_SHADER_PROGRAM_T(template<>, TSplineProgram<1>);

	template< bool bEnable >
	class TTessellationProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(TTessellationProgram, Global);
		using BaseClass = GlobalShaderProgram;

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{

		}

		void setParameters()
		{

		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM(USE_TESSELLATION), bEnable);
		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/Tessellation";
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			if( bEnable )
			{
				static ShaderEntryInfo entriesWithTesselation[] =
				{
					{ EShader::Vertex , SHADER_ENTRY(MainVS) },
					{ EShader::Hull   , SHADER_ENTRY(MainHS) },
					{ EShader::Domain , SHADER_ENTRY(MainDS) },
					{ EShader::Pixel , SHADER_ENTRY(MainPS) },
				};
				return entriesWithTesselation;
			}

			static ShaderEntryInfo entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}
	};
	IMPLEMENT_SHADER_PROGRAM_T( template<>, TTessellationProgram<true>);
	IMPLEMENT_SHADER_PROGRAM_T( template<>, TTessellationProgram<false>);

	struct alignas(16) WaterVertexData
	{
		//Vector3 normal;
		Vector2 normal;
		float h;
		float v;
		
	};

	class WaterSimulationProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(WaterSimulationProgram, Global);
		using BaseClass = GlobalShaderProgram;

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BaseClass::bindParameters(parameterMap);
			mParamDataIn.bind(parameterMap, SHADER_PARAM(WaterDataInBlock));
			mParamDataOut.bind(parameterMap, SHADER_PARAM(WaterDataOutBlock));
			mParamWaterParam.bind(parameterMap, SHADER_PARAM(WaterParam));
			mParamTileNum.bind(parameterMap, SHADER_PARAM(TileNum));
		}

		void setParameters(RHICommandList& commandList, Vector4 const& param , int TileNum , RHIVertexBuffer& DataIn , RHIVertexBuffer& DataOut )
		{
			setStorageBuffer(commandList, mParamDataIn, DataIn);
			setStorageBuffer(commandList, mParamDataOut, DataOut);
			setParam(commandList, mParamWaterParam, param);
			setParam(commandList, mParamTileNum, TileNum);
		}


		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);

		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/WaterSimulation";
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ EShader::Compute , SHADER_ENTRY(MainUpdateCS) },
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
		DECLARE_SHADER_PROGRAM(WaterUpdateNormalProgram, Global);
		using BaseClass = GlobalShaderProgram;

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BaseClass::bindParameters(parameterMap);
			mParamData.bind(parameterMap, SHADER_PARAM(WaterDataOutBlock));
			mParamTileNum.bind(parameterMap, SHADER_PARAM(TileNum));
		}

		void setParameters(RHICommandList& commandList, int TileNum, RHIVertexBuffer& Data)
		{
			setStorageBuffer(commandList, mParamData, Data);
			setParam(commandList, mParamTileNum, TileNum);
		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);

		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/WaterSimulation";
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ EShader::Compute , SHADER_ENTRY(MainUpdateNormal) },
			};
			return entries;
		}

		ShaderParameter mParamData;
		ShaderParameter mParamTileNum;
	};


	class WaterProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(WaterProgram, Global);
		using BaseClass = GlobalShaderProgram;

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BaseClass::bindParameters(parameterMap);
			mParamDataIn.bind(parameterMap, SHADER_PARAM(WaterDataInBlock));
			mParamTileNum.bind(parameterMap, SHADER_PARAM(TileNum));
		}

		void setParameters(RHICommandList& commandList, int TileNum, RHIVertexBuffer& DataIn)
		{
			setStorageBuffer(commandList, mParamDataIn, DataIn);
			if (mParamTileNum.isBound())
			{
				setParam(commandList, mParamTileNum, TileNum);
			}
		}


		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);

		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/Water";
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		ShaderParameter mParamDataIn;
		ShaderParameter mParamTileNum;
	};

	IMPLEMENT_SHADER_PROGRAM(WaterSimulationProgram);
	IMPLEMENT_SHADER_PROGRAM(WaterUpdateNormalProgram);
	IMPLEMENT_SHADER_PROGRAM(WaterProgram);

	struct GPUParticleData
	{

		bool initialize()
		{
			
			VERIFY_RETURN_FALSE(mProgInit = ShaderManager::Get().getGlobalShaderT< ParticleInitProgram >(true));
			VERIFY_RETURN_FALSE(mProgUpdate = ShaderManager::Get().getGlobalShaderT< ParticleUpdateProgram >(true));

			VERIFY_RETURN_FALSE(mProgParticleRender = ShaderManager::Get().getGlobalShaderT< SimpleSpriteParticleProgram >(true));

			VERIFY_RETURN_FALSE(mParticleBuffer.initializeResource(1000));
			VERIFY_RETURN_FALSE(mInitParamBuffer.initializeResource(1));
			VERIFY_RETURN_FALSE(mParamBuffer.initializeResource(1));

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
				VERIFY_RETURN_FALSE(mCollisionPrimitiveBuffer.initializeResource(mPrimitives.size()));
				CollisionPrimitive* pData = mCollisionPrimitiveBuffer.lock();
				memcpy(pData, &mPrimitives[0], mPrimitives.size() * sizeof(CollisionPrimitive));
				mCollisionPrimitiveBuffer.unlock();

				return true;
			}
		}


		void initParticleData(RHICommandList& commandList)
		{
			RHISetShaderProgram(commandList, mProgInit->getRHIResource());
			mProgInit->setParameters(commandList, mParticleBuffer, mParamBuffer, mInitParamBuffer);
			RHIDispatchCompute(commandList, mParticleBuffer.getElementNum(), 1, 1);
		}

		void updateParticleData(RHICommandList& commandList, float dt)
		{
			RHISetShaderProgram(commandList, mProgUpdate->getRHIResource());
			mProgUpdate->setParameters(commandList, mParticleBuffer, mParamBuffer, mInitParamBuffer, dt, mPrimitives.size());
			mProgUpdate->setStructuredUniformBufferT<CollisionPrimitive>(commandList, *mCollisionPrimitiveBuffer.getRHI());
			RHIDispatchCompute(commandList, mParticleBuffer.getElementNum(), 1, 1);
		}

		TStructuredBuffer< ParticleInitParameters > mInitParamBuffer;
		TStructuredBuffer< ParticleParameters >     mParamBuffer;
		TStructuredBuffer< ParticleData > mParticleBuffer;
		TStructuredBuffer< CollisionPrimitive > mCollisionPrimitiveBuffer;

		ParticleInitProgram* mProgInit;
		ParticleUpdateProgram* mProgUpdate;
		SimpleSpriteParticleProgram* mProgParticleRender;

		std::vector< CollisionPrimitive > mPrimitives;
	};



	class GPUParticleTestStage : public TestRenderStageBase
		                       , public GPUParticleData
	{
		using BaseClass = TestRenderStageBase;
	public:
		GPUParticleTestStage() {}



		int mIndexWaterBufferUsing = 0;
		TStructuredBuffer< WaterVertexData > mWaterDataBuffers[2];
		WaterSimulationProgram* mProgWaterSimulation;
		WaterUpdateNormalProgram* mProgWaterUpdateNormal;
		WaterProgram* mProgWater;
		

		RHITexture2DRef mTexture;
		RHITexture2DRef mBaseTexture;
		RHITexture2DRef mNormalTexture;

		Mesh mTileMesh;
		int  mTileNum = 500;
		Mesh mCube;

		
		float mWaterSpeed = 10;

		Mesh mTilePlane;

		int TessFactor1 = 5;
		int TessFactor2 = 1;
		int TessFactor3 = 1;
		float mDispFactor = 1.0;

		SplineProgram* mProgSplines[2];
		int mSplineType = 0;

		bool bUseTessellation = true;
		bool bWireframe = false;
		bool onInit() override
		{
			if( !BaseClass::onInit() )
				return false;

			VERIFY_RETURN_FALSE(SharedAssetData::loadCommonShader());
			VERIFY_RETURN_FALSE(GPUParticleData::initialize());
			
			VERIFY_RETURN_FALSE(mProgSplines[0] = ShaderManager::Get().getGlobalShaderT< TSplineProgram<0> >(true));
			VERIFY_RETURN_FALSE(mProgSplines[1] = ShaderManager::Get().getGlobalShaderT< TSplineProgram<1> >(true));

			VERIFY_RETURN_FALSE(mTexture = RHIUtility::LoadTexture2DFromFile("Texture/star.png"));
			VERIFY_RETURN_FALSE(mBaseTexture = RHIUtility::LoadTexture2DFromFile("Texture/stones.bmp"));
			VERIFY_RETURN_FALSE(mNormalTexture = RHIUtility::LoadTexture2DFromFile("Texture/stones_NM_height.tga"));

			VERIFY_RETURN_FALSE(mTexture.isValid());
			Vector3 v[] =
			{
				Vector3(1,1,0),
				Vector3(-1,1,0) ,
				Vector3(-1,-1,0),
				Vector3(1,-1,0),
			};
			int   idx[6] = { 0 , 1 , 2 , 0 , 2 , 3 };
			mSpherePlane.mInputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
			VERIFY_RETURN_FALSE(mSpherePlane.createRHIResource(&v[0], 4, &idx[0], 6, true));

			VERIFY_RETURN_FALSE(MeshBuild::Tile(mTileMesh, mTileNum - 1, 100, false));
			VERIFY_RETURN_FALSE(MeshBuild::Tile(mTilePlane, 4, 1, false));
			VERIFY_RETURN_FALSE(MeshBuild::CubeShare(mCube,1));

			VERIFY_RETURN_FALSE(mWaterDataBuffers[0].initializeResource(mTileNum * mTileNum));
			VERIFY_RETURN_FALSE(mWaterDataBuffers[1].initializeResource(mTileNum * mTileNum));
			VERIFY_RETURN_FALSE(mProgWaterSimulation = ShaderManager::Get().getGlobalShaderT< WaterSimulationProgram >(true));
			VERIFY_RETURN_FALSE(mProgWaterUpdateNormal = ShaderManager::Get().getGlobalShaderT< WaterUpdateNormalProgram >(true));
			VERIFY_RETURN_FALSE(mProgWater = ShaderManager::Get().getGlobalShaderT< WaterProgram >(true));
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


			Vec2i screenSize = ::Global::GetScreenSize();
			::Global::GUI().cleanupWidget();

			auto devFrame = WidgetUtility::CreateDevFrame();
			devFrame->addText("Water Speed");
			auto slider = devFrame->addSlider(UI_ANY);
			FWidgetProperty::Bind(slider, mWaterSpeed, 0, 10);
			devFrame->addText("Spline Type");
			FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mSplineType, 0 , 1);
			devFrame->addText("TessFactor");
			FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), TessFactor1, 0, 70);
			FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), TessFactor2, 0, 70);
			FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), TessFactor3, 0, 70);
			FWidgetProperty::Bind(devFrame->addCheckBox(UI_ANY, "UseTess"), bUseTessellation);
			FWidgetProperty::Bind(devFrame->addCheckBox(UI_ANY, "WireFrame"), bWireframe);
			devFrame->addText("DispFactor");
			FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mDispFactor, 0, 10);
			restart();

			return true;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void drawSphere(RHICommandList& commandList, Vector3 const& pos, float radius)
		{
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

			RHISetShaderProgram(commandList, mProgSphere->getRHIResource());
			mView.setupShader(commandList, *mProgSphere);

			mProgSphere->setParameters(commandList, pos, radius, Color3f(1, 1, 1));
			mSpherePlane.draw(commandList);
		}

		void upateWaterData(RHICommandList& commandList, float dt)
		{
			mIndexWaterBufferUsing = 1 - mIndexWaterBufferUsing;
			{
				RHISetShaderProgram(commandList, mProgWaterSimulation->getRHIResource());
				Vector4 waterParam;
				waterParam.x = dt;
				waterParam.y = mWaterSpeed * dt * mTileNum / 10;
				waterParam.z = 1;
				mProgWaterSimulation->setParameters(commandList, waterParam, mTileNum, *mWaterDataBuffers[1 - mIndexWaterBufferUsing].getRHI(), *mWaterDataBuffers[mIndexWaterBufferUsing].getRHI());
				RHIDispatchCompute(commandList, mTileNum, mTileNum, 1);
			}
			{
				RHISetShaderProgram(commandList, mProgWaterUpdateNormal->getRHIResource());
				mProgWaterUpdateNormal->setParameters(commandList, mTileNum, *mWaterDataBuffers[mIndexWaterBufferUsing].getRHI());
				RHIDispatchCompute(commandList, mTileNum, mTileNum, 1);

			}
		}
		void restart() override 
		{
			initParticleData(RHICommandList::GetImmediateList());
		}
		void tick() override {}
		void updateFrame(int frame) override {}

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);

			float dt = float(time) / 1000;
			updateParticleData(RHICommandList::GetImmediateList() , dt);
			upateWaterData(RHICommandList::GetImmediateList(), dt);
		}

		Mesh mSpherePlane;
		ShaderProgram mProgSimpleParticle;


		void onRender(float dFrame) override
		{
			Graphics2D& g = Global::GetGraphics2D();


			RHICommandList& commandList = RHICommandList::GetImmediateList();


			initializeRenderState();

			{
				RHISetShaderProgram(commandList, mProgWater->getRHIResource());
				mProgWater->setParameters(commandList, mTileNum, *mWaterDataBuffers[mIndexWaterBufferUsing].getRHI());
				mView.setupShader(commandList, *mProgWater);
				mTileMesh.draw(commandList, LinearColor(1, 0, 0));
			}

			for( auto const& primitive : mPrimitives )
			{
				switch( primitive.type )
				{
				case 0:
					break;
				case 1:
					drawSphere( commandList, primitive.param.xyz(), primitive.param.w);
					break;
				}
			}


			{
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

				RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::SrcAlpha, EBlend::One >::GetRHI());
				RHISetShaderProgram(commandList, mProgParticleRender->getRHIResource());
				mView.setupShader(commandList, *mProgParticleRender);
				mProgParticleRender->setParameters(commandList, mParticleBuffer, *mTexture);
				//glDrawArrays(GL_POINTS , 0, mParticleBuffer.getElementNum() - 2);

				RHIDrawPrimitive(commandList, EPrimitive::Points, 0, mParticleBuffer.getElementNum() - 2);
			}



			{
				int width = ::Global::GetScreenSize().x;
				int height = ::Global::GetScreenSize().y;

				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

				struct MyVertex
				{
					Vector2 pos;
					Vector3 color;
				};

				MyVertex vertices[] =
				{
					Vector2(100, 100), Vector3(1, 0, 0),
					Vector2(400, 100), Vector3(0, 1, 0),
					Vector2(100, 400), Vector3(0, 0, 1),
					Vector2(400, 400), Vector3(1, 1, 1),
				};

				uint32 indices[] =
				{
					0,0,1,2, 0,1,2,3, 1,2,3,3
				};

				SplineProgram* progSpline = mProgSplines[mSplineType];
				RHISetShaderProgram(commandList, progSpline->getRHIResource());
				progSpline->setParam(commandList, SHADER_PARAM(XForm), AdjProjectionMatrixForRHI(OrthoMatrix(0, width, 0, height, -100, 100) ));
				progSpline->setParam(commandList, SHADER_PARAM(TessFactor), TessFactor1);

				if (mSplineType == 0)
				{
					TRenderRT< RTVF_XY_C > ::Draw(commandList, SplineProgram::UseTesselation ? EPrimitive::PatchPoint4 : EPrimitive::LineStrip, vertices, 4);
				}
				else
				{

					TRenderRT< RTVF_XY_C > ::DrawIndexed(commandList, SplineProgram::UseTesselation ? EPrimitive::PatchPoint4 : EPrimitive::LineStrip, vertices, 4, indices, ARRAY_SIZE(indices));
				}
			}

			{
				int width = ::Global::GetScreenSize().x;
				int height = ::Global::GetScreenSize().y;

				RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				if( bWireframe )
				{
					RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back, EFillMode::Wireframe>::GetRHI());
				}

				ShaderProgram* program;
				if( bUseTessellation )
				{
					program = ShaderManager::Get().getGlobalShaderT< TTessellationProgram<true> >();
				}
				else
				{
					program = ShaderManager::Get().getGlobalShaderT< TTessellationProgram<false> >();
				}

				RHISetShaderProgram(commandList, program->getRHIResource());
				Matrix4 localToWorld = Matrix4::Scale(10) * Matrix4::Translate( -20 , - 20 , 10 );
				Matrix4 worldToLocal;
				float det;
				localToWorld.inverseAffine(worldToLocal, det);
				program->setParam(commandList, SHADER_PARAM(XForm), AdjProjectionMatrixForRHI( OrthoMatrix(0, width, 0, height, -100, 100) ) );
				program->setTexture(commandList, SHADER_PARAM(BaseTexture), *mBaseTexture);
				program->setParam(commandList, SHADER_PARAM(Primitive.localToWorld), localToWorld );
				program->setParam(commandList, SHADER_PARAM(Primitive.worldToLocal), worldToLocal );

				mView.setupShader(commandList, *program);
				if ( bUseTessellation )
				{
					program->setParam(commandList, SHADER_PARAM(TessOuter), TessFactor1);
					program->setParam(commandList, SHADER_PARAM(TessInner), TessFactor2);
					program->setTexture(commandList, SHADER_PARAM(DispTexture), *mNormalTexture);
					program->setParam(commandList, SHADER_PARAM(DispFactor), mDispFactor);

				}

#if 1
				struct MyVertex
				{
					Vector3 pos;
					Vector3 color;
					Vector3 normal;
					Vector2 uv;
				};


				MyVertex vertices[] =
				{
					Vector3(-1, -1,0), Vector3(1, 0, 0), Vector3(-1, -1,1 ).getNormal() , Vector2(0,0),
					Vector3(1, -1,0), Vector3(0, 1, 0),   Vector3(1, -1,1).getNormal() ,Vector2(1,0),
					Vector3(-1, 1,0),	Vector3(0, 0, 1),   Vector3(-1, 1,1).getNormal() ,Vector2(0,1),
					Vector3(1, 1,0),	Vector3(1, 1, 1),   Vector3(1, 1,1).getNormal() ,Vector2(1,1),
				};

				uint32 indices[] = { 0 , 1 , 2 , 1 , 3 , 2 };

				TRenderRT< RTVF_XYZ_C_N_T2 > ::DrawIndexed(commandList,
					bUseTessellation ? EPrimitive::PatchPoint3 : EPrimitive::TriangleList, 
					vertices, ARRAY_SIZE(vertices) , indices , ARRAY_SIZE(indices) );
#else
				//mTilePlane.drawTessellation(commandList);
				mCube.drawTessellation(commandList,3);
#endif
				RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());

			}

			g.beginRender();

			g.endRender();
		}


		bool onMouse(MouseMsg const& msg) override
		{
			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}

		bool onKey(KeyMsg const& msg) override
		{
			if( msg.isDown() )
			{
				switch( msg.getCode() )
				{
				case EKeyCode::Left: --TessFactor1; break;
				case EKeyCode::Right: ++TessFactor1; break;
				case EKeyCode::Down: --TessFactor2; break;
				case EKeyCode::Up: ++TessFactor2; break;
				case EKeyCode::F2:
					{
						ShaderManager::Get().reloadAll();
						//initParticleData(RHICommandList::GetImmediateList());
					}
					break;
				}
			}
			return BaseClass::onKey(msg);
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
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

	REGISTER_STAGE("GPU Particle Test", GPUParticleTestStage, EStageGroup::FeatureDev, 1, "Render|Shader");
}
