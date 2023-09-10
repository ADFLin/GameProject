#include "VertexFactory.h"
#include "ShaderManager.h"

namespace Render
{
	
#if CORE_SHARE_CODE
	CORE_API VertexFactoryType* VertexFactoryType::DefaultType = &LocalVertexFactory::StaticType;
	CORE_API TArray< VertexFactoryType* > VertexFactoryType::TypeList;

	VertexFactoryType::VertexFactoryType(char const* inFileName, ModifyCompilationOptionFunc MCO)
		:fileName(inFileName)
		, ModifyCompilationOption(MCO)
	{
		TypeList.push_back(this);
	}

	IMPL_VERTEX_FACTORY_TYPE(LocalVertexFactory, "LocalVertexFactory")
	IMPL_VERTEX_FACTORY_TYPE(GPUSkinVertexFactory, "GPUSkinVertexFactory")
#endif CORE_SHARE_CODE

	void VertexFactoryType::getCompileOption(ShaderCompileOption& opation)
	{
		//opation.addInclude(fileName);
		opation.addDefine(SHADER_PARAM(VEREX_FACTORY_FILENAME), ShaderCompileOption::GetIncludeFileName(fileName));

		ModifyCompilationOption(opation);
	}




}//namespace Render

