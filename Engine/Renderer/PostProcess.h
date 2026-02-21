#pragma once

#include "RHI/RHICommand.h"
#include "RHI/GlobalShader.h"
#include "InlineString.h"

namespace Render
{
	struct PostProcessContext
	{
	public:
		RHITexture2D* getTexture(int slot = 0) const { return mInputTexture[slot]; }
		RHITexture2DRef mInputTexture[4];
	};


	struct PostProcessParameters
	{
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			for (int i = 0; i < MaxInputNum; ++i)
			{
				InlineString<128> name;
				name.format("TextureInput%d", i);
				mParamTextureInput[i].bind(parameterMap, name);
			}
		}

		void setParameters(RHICommandList& commandList, ShaderProgram& shader, PostProcessContext const& context)
		{
			for (int i = 0; i < MaxInputNum; ++i)
			{
				if (!mParamTextureInput[i].isBound())
					break;
				if (context.getTexture(i))
					shader.setTexture(commandList, mParamTextureInput[i], *context.getTexture(i));
			}
		}

		static int const MaxInputNum = 4;

		ShaderParameter mParamTextureInput[MaxInputNum];
	};
}
