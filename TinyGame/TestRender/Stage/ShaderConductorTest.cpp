#if 0
#include "Stage/TestRenderStageBase.h"

#include "ShaderConductor/ShaderConductor.hpp"


#pragma comment(lib , "ShaderConductor.lib")


namespace SC = ShaderConductor;

class SCTestStage : public StageBase
{
	using BaseClass = StageBase;
public:
	SCTestStage() {}

	bool onInit() override
	{
		if (!BaseClass::onInit())
			return false;

		static const char* vertexShader = 
CODE_STRING(
cbuffer vertexBuffer : register(b0)
{
    float4x4 ProjectionMatrix;
};
struct VS_INPUT
{
    float2 pos : POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};
float SValue;
struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};
            
PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    output.pos = SValue * mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
    output.col = input.col;
    output.uv  = input.uv;
    return output;
};
);

		SC::Compiler::SourceDesc source;
		source.numDefines = 0;
		source.defines = nullptr;
		source.source = vertexShader;
		source.entryPoint = "main";
		source.stage = SC::ShaderStage::VertexShader;
		source.fileName = nullptr;
		SC::Compiler::Options options;
		SC::Compiler::TargetDesc target;
		target.options = nullptr;
		target.numOptions = 0;
		target.version = "460";
		target.asModule = false;
		target.language = ShaderConductor::ShadingLanguage::Glsl;
		SC::Compiler::ResultDesc result = SC::Compiler::Compile(source, options, target);
		if (result.hasError)
		{


		}
		else /*if ( result.isText )*/
		{
			char const* text = (char const*)result.target.Data();
			LogMsg("code = %s", text);
		}

		::Global::GUI().cleanupWidget();
		restart();
		return true;
	}

	void onEnd() override
	{
		BaseClass::onEnd();
	}

	void restart() {}
	void tick() {}
	void updateFrame(int frame) {}

	void onUpdate(long time) override
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		for (int i = 0; i < frame; ++i)
			tick();

		updateFrame(frame);
	}

	void onRender(float dFrame) override
	{
		Graphics2D& g = Global::GetGraphics2D();
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
			case EKeyCode::R: restart(); break;
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

REGISTER_STAGE_ENTRY("Shader Conductor Test", SCTestStage, EExecGroup::FeatureDev);
#endif