#pragma once
#ifndef VertexFactory_H_86132998_96CC_46B9_8772_AD134BCD249E
#define VertexFactory_H_86132998_96CC_46B9_8772_AD134BCD249E

#include "RHICommon.h"

namespace RenderGL
{
	class ShaderCompileOption;
	class VertexFactoryShaderData;

	class VertexFarcoryType
	{
	public:

		typedef VertexFactoryShaderData* (*CreateShaderDataFun)();
		typedef void (*ModifyCompilationOptionFun)(ShaderCompileOption& opation);

		VertexFarcoryType(char const* inFileName , ModifyCompilationOptionFun MCO );
		
		char const* fileName;
		ModifyCompilationOptionFun ModifyCompilationOption;
		CreateShaderDataFun CreateShaderData;

		void getCompileOption(ShaderCompileOption& option);
		
		static VertexFarcoryType* DefaultType;
		static std::vector< VertexFarcoryType* > TypeList;
	};

	class VertexFactoryShaderData
	{
	public:
	};
	class VertexFactory
	{
	public:
		virtual VertexFarcoryType& getType() = 0;
		VertexDecl*  vertexDecl;
		static VertexFactoryShaderData* CreateShaderData() { return nullptr; }
		static void ModifyCompilationOption(ShaderCompileOption& option){}
	};

#define DECL_VERTEX_FACTORY_TYPE( CLASS )\
	friend class VertexFarcoryType;\
	static VertexFarcoryType StaticType;\
	virtual VertexFarcoryType& getType() override { return StaticType; }

#define IMPL_VERTEX_FACTORY_TYPE( CLASS , FILE_NAME )\
	VertexFarcoryType CLASS::StaticType( FILE_NAME , &CLASS::ModifyCompilationOption);\

	class LocalVertexFactory : public VertexFactory
	{
		DECL_VERTEX_FACTORY_TYPE(LocalVertexFactory)
	public:		
	};

	class GPUSkinVertexFactory : public VertexFactory
	{
		DECL_VERTEX_FACTORY_TYPE(GPUSkinVertexFactory)
	public:
	};

}//RenderGL

#endif // VertexFactory_H_86132998_96CC_46B9_8772_AD134BCD249E
