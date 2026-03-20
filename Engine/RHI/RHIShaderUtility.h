#pragma once
#ifndef RHIShaderUtility_H_B0B0B0B0_B0B0_B0B0_B0B0_B0B0B0B0B0B0
#define RHIShaderUtility_H_B0B0B0B0_B0B0_B0B0_B0B0_B0B0B0B0B0B0

#include "GlobalShader.h"
#include "RHICommand.h"

namespace Render
{
	class RHIShaderUtility
	{
	public:
		static void GenerateMips(RHICommandList& commandList, RHITextureBase& texture);
	};

}

#endif
