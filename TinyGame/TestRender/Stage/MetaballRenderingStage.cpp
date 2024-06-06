#include "TestRenderStageBase.h"
#include "RHI/RHIGraphics2D.h"

namespace Render
{

	class MetaballProgram : public GlobalShaderProgram
	{
	public:
		using BaseClass = GlobalShaderProgram;

		DECLARE_SHADER_PROGRAM(MetaballProgram, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/Game/Metaball";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(BasePassVS) },
				{ EShader::Pixel  , SHADER_ENTRY(BasePassPS) },
			};
			return entries;
		}
	};

	IMPLEMENT_SHADER_PROGRAM(MetaballProgram);

	class MetaballRenderingStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		MetaballRenderingStage() {}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			IntVector2 screenSize = ::Global::GetScreenSize();
			::Global::GUI().cleanupWidget();

			return true;
		}

		virtual ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}

		virtual void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{

		}


		virtual bool setupRenderResource(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));
			VERIFY_RETURN_FALSE(SharedAssetData::loadCommonShader());
			VERIFY_RETURN_FALSE(SharedAssetData::createSimpleMesh());

			Vector2 vertices[] =
			{
				Vector2(-1,-1),
				Vector2(-1, 1),
				Vector2(1,-1),
				Vector2(1, 1),
			};

			mQuadVertexBuffer = RHICreateVertexBuffer(sizeof(Vector2), 4, BCF_DefalutValue, vertices);
			uint16 indices[] =
			{
				0,1,2,2,1,3
			};
			mQuadIndexBuffer = RHICreateIndexBuffer(ARRAY_SIZE(indices), false, BCF_DefalutValue, indices);

			mProgMetaball = ShaderManager::Get().getGlobalShaderT<MetaballProgram>();
			return true;
		}

		RHIBufferRef mQuadVertexBuffer;
		RHIBufferRef mQuadIndexBuffer;

		MetaballProgram* mProgMetaball;

		virtual void preShutdownRenderSystem(bool bReInit = false) override
		{
			BaseClass::preShutdownRenderSystem(bReInit);
		}


		void onEnd() override
		{


		}

		void restart() override
		{

		}

		void tick() override
		{

		}

		void updateFrame(int frame) override
		{

		}

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);
		}

		void onRender(float dFrame) override
		{
			GRenderTargetPool.freeAllUsedElements();

			Vec2i screenSize = ::Global::GetScreenSize();

			RHICommandList& commandList = RHICommandList::GetImmediateList();

			initializeRenderState();
			
			RHISetFixedShaderPipelineState(commandList, AdjProjectionMatrixForRHI(mView.worldToClip));
			DrawUtility::AixsLine(commandList, 20);

			RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

			RHISetShaderProgram(commandList, mProgMetaball->getRHI());
			mView.setupShader(commandList, *mProgMetaball);
			mProgMetaball->setParam(commandList, SHADER_PARAM(SpherePosAndRadius), Vector4(0,0,0,10));
			InputStreamInfo stream;
			stream.buffer = mQuadVertexBuffer;
			stream.offset = 0;
			RHISetInputStream(commandList, &TRenderRT<RTVF_XY>::GetInputLayout(), &stream, 1);
			RHIDrawPrimitive(commandList, EPrimitive::TriangleStrip, 0, 4);

			RHIFlushCommand(commandList);
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				default:
					break;
				}
			}
			return BaseClass::onKey(msg);
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch (id)
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}
	protected:
	};

	REGISTER_STAGE_ENTRY("Metaball Rendering", MetaballRenderingStage, EExecGroup::FeatureDev);

}