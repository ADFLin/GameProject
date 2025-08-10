#pragma once
#ifndef ShadertoyCore_H_09B8FFB4_85AF_4F47_A44A_A93D142E74A7
#define ShadertoyCore_H_09B8FFB4_85AF_4F47_A44A_A93D142E74A7


#include "RHI/ShaderProgram.h"
#include "RHI/GlobalShader.h"
#include "Renderer/RenderTargetPool.h"

#include "Audio/AudioTypes.h"
#include "Audio/AudioStreamSource.h"

namespace Render
{
	class ScreenVS;
}

namespace Shadertoy
{
	using namespace Render;
	struct GPU_ALIGN InputParam
	{
		DECLARE_UNIFORM_BUFFER_STRUCT(InputBlock);

		Vector3 iResolution;
		float  iTime;
		float  iTimeDelta;
		int    iFrame;
		float  iFrameRate;
		float  iSampleRate;
		Vector4 iMouse;
		Vector4 iDate;
	};

	enum class EPassType
	{
		None,
		Sound,
		Buffer,
		CubeMap,
		Image,
	};

	enum class EInputType
	{
		Keyboard,
		Webcam,
		Micrphone,
		Soundcloud,
		Buffer,
		CubeBuffer,
		Texture,
		CubeMap,
		Volume,
		Vedio,
		Music,

		COUNT,
	};

	struct RenderInput
	{
		EInputType type;
		int resId;
		int channel;
		bool bVFlip = false;
		ESampler::Filter      filter = ESampler::Bilinear;
		ESampler::AddressMode addressMode = ESampler::Clamp;
	};

	struct RenderOutput
	{
		int bufferId;
		int resId;
		int channel;
		bool bGenMipmaps;
	};

	struct RenderPassInfo
	{
		EPassType passType;
		int typeIndex;
		std::string code;

		TArray<RenderInput>  inputs;
		TArray<RenderOutput> outputs;
	};

	class CubeOnePassVS : public GlobalShader
	{
		using BaseClass = GlobalShader;
		DECLARE_SHADER(CubeOnePassVS, Global);
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/Game/ShadertoyTemplate";
		}
	};

	class RenderPassShader : public Shader
	{
	public:
		void bindParameters(ShaderParameterMap const& parameterMap) override;

		RenderPassInfo const* passInfo = nullptr;
		void bindInputParameters(TArray<RenderInput> const& inputs, ShaderParameterMap const& parameterMap);

		static RHISamplerState& GetSamplerState(RenderInput const& input);

		static RHITextureBase* GetInputTexture(RenderInput const& input, TArray< RHITextureRef > const& inputResources);

		void setInputParameters(RHICommandList& commandList, TArray<RenderInput> const& inputs, TArray< RHITextureRef > const& inputResources);

		void setSoundParameters(RHICommandList& commandList, float timeOffset, int64 sampleOffset);

		void setCubeParameters(RHICommandList& commandList, IntVector2 const& viewportSize, int subPassIndex = INDEX_NONE);;

		ShaderParameter mParamChannelResolution;
		TArray< ShaderParameter > mParamInputs;
		TArray< ShaderParameter > mParamInputSamplers;
		TArray< ShaderParameter > mParamOutputs;
	};

	RHITexture3D* LoadBinTexture(char const* path, bool bGenMipmap);


	struct RenderPassData
	{
		RenderPassInfo* info;
		RenderPassShader shader;
	};

	char constexpr* AlphaSeq = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";


	class Renderer
	{
	public:

		void setRenderTarget(RHICommandList& commandList, RHITexture2D& target);

		void setPixelShader(RHICommandList& commandList, RenderPassShader& shader);

		void updateInput(InputParam const& inputParam, uint8 keyboardData[]);

		void setInputParameters(RHICommandList& commandList, RenderPassData& pass);

		static char const* GetOutputName(int index)
		{
			static char const* OutputNames[] = { "OutTexture", "OutTexture1" , "OutTexture2" , "OutTexture3" };
			return OutputNames[index];
		}
		static constexpr int GROUP_SIZE = 8;

		PooledRenderTargetRef render(RHICommandList& commandList, TArray< std::unique_ptr<RenderPassData> > const& renderPassList, IntVector2 const& screenSize);

		void renderImage(RHICommandList& commandList, RenderPassData& pass, IntVector2 const& viewportSize, TArrayView< PooledRenderTargetRef > outputBuffers);


		void renderCubeOnePass(RHICommandList& commandList, RenderPassData& pass, IntVector2 const& viewportSize, TArrayView< PooledRenderTargetRef > outputBuffers);

		void renderSound(RHICommandList& commandList, RenderPassData& pass, WaveFormatInfo const& format, int64 samplePos, int sampleCount, AudioStreamSample& outData);

		bool initializeRHI();

		void releaseRHI();

		static TVector2<int16> Decode(uint8 const* pData)
		{
			int16 left = int16(MaxInt16 *(-1.0 + 2.0*(pData[0] + 256.0*pData[1]) / 65535.0));
			int16 right = int16(MaxInt16 *(-1.0 + 2.0*(pData[2] + 256.0*pData[3]) / 65535.0));
			return { left, right };
		}

		void readSampleData(int16* pOutData, int numSamples);

		void addInputResource(RenderInput const& input, RHITextureBase& texture);

		bool canUseComputeShader(RenderPassData const& pass)
		{
			return bAllowComputeShader /*&& pass.info->passType != EPassType::CubeMap*/;
		}

		bool compileShader(RenderPassData& pass, TArrayView<uint8 const> codeTemplate, RenderPassInfo* commonInfo);


		bool bAllowComputeShader = false;
		bool bAllowRenderCubeOnePass = true;
		bool bUseMultisample = false;


		TArray< PooledRenderTargetRef > mOutputBuffers;
		TArray< RHITextureRef > mInputResources;

		static constexpr int SoundSampleRate = 44100;
		static constexpr int SoundTextureSize = 256;

		ScreenVS* mScreenVS;
		CubeOnePassVS* mCubeOnePassVS;
		RHIFrameBufferRef mFrameBuffer;
		TStructuredBuffer< InputParam > mInputBuffer;
		RHITexture2DRef mTexKeyboard;
		RHITexture2DRef mTexSound;
	};

}//namespace Shadertoy

#endif // ShadertoyCore_H_09B8FFB4_85AF_4F47_A44A_A93D142E74A7