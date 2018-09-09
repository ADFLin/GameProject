#include "VertexFactory.h"
#include "ShaderCompiler.h"

namespace Render
{

#if CORE_SHARE_CODE
	VertexFactoryType* VertexFactoryType::DefaultType = &LocalVertexFactory::StaticType;
	std::vector< VertexFactoryType* > VertexFactoryType::TypeList;
#endif CORE_SHARE_CODE

	void VertexFactoryType::getCompileOption(ShaderCompileOption& opation)
	{
		opation.addInclude(fileName);
		ModifyCompilationOption(opation);
	}

	VertexFactoryType::VertexFactoryType(char const* inFileName , ModifyCompilationOptionFun MCO )
		:fileName(inFileName)
		,ModifyCompilationOption( MCO )
	{
		TypeList.push_back(this);
	}

	IMPL_VERTEX_FACTORY_TYPE( LocalVertexFactory , "LocalVertexFactory" )
	IMPL_VERTEX_FACTORY_TYPE( GPUSkinVertexFactory , "GPUSkinVertexFactory")

}//namespace Render

