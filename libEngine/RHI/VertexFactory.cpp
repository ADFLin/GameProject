#include "VertexFactory.h"
#include "ShaderManager.h"

namespace Render
{
	
#if CORE_SHARE_CODE
	VertexFactoryType* VertexFactoryType::DefaultType = &LocalVertexFactory::StaticType;
	std::vector< VertexFactoryType* > VertexFactoryType::TypeList;

	VertexFactoryType::VertexFactoryType(char const* inFileName, ModifyCompilationOptionFun MCO)
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
		opation.addInclude(fileName);
		ModifyCompilationOption(opation);
	}




}//namespace Render

