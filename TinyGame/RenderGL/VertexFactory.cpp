#include "VertexFactory.h"
#include "ShaderCompiler.h"

namespace RenderGL
{

	void VertexFarcoryType::GetCompileOption(ShaderCompileOption& opation)
	{
		opation.addInclude(fileName);
		ModifyCompilationOption(opation);
	}

	VertexFarcoryType* VertexFarcoryType::DefaultType = &LocalVertexFactory::StaticType;
	std::vector< VertexFarcoryType* > VertexFarcoryType::TypeList;

	VertexFarcoryType::VertexFarcoryType(char const* inFileName , ModifyCompilationOptionFun MCO )
		:fileName(inFileName)
		,ModifyCompilationOption( MCO )
	{
		TypeList.push_back(this);
	}

	IMPL_VERTEX_FACTORY_TYPE( LocalVertexFactory , "LocalVertexFactory" )
	IMPL_VERTEX_FACTORY_TYPE( GPUSkinVertexFactory , "GPUSkinVertexFactory")

}//namespace RenderGL

