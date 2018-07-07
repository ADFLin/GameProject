#pragma once
#ifndef VertexFactory_H_86132998_96CC_46B9_8772_AD134BCD249E
#define VertexFactory_H_86132998_96CC_46B9_8772_AD134BCD249E

#include "RHICommon.h"
#include "CoreShare.h"

namespace RenderGL
{
	class ShaderCompileOption;
	class VertexFactoryShaderData;
	class VertexFactoryType;

	class VertexFactoryType
	{
	public:

		typedef VertexFactoryShaderData* (*CreateShaderDataFun)();
		typedef void (*ModifyCompilationOptionFun)(ShaderCompileOption& opation);

		VertexFactoryType(char const* inFileName , ModifyCompilationOptionFun MCO );
		
		char const* fileName;
		ModifyCompilationOptionFun ModifyCompilationOption;
		CreateShaderDataFun CreateShaderData;

		void getCompileOption(ShaderCompileOption& option);
		virtual void setupShader( MaterialShaderProgram& shader , VertexFactoryShaderData* shaderData)
		{

		}
		
		CORE_API static VertexFactoryType* DefaultType;
		CORE_API static std::vector< VertexFactoryType* > TypeList;
	};

	class VertexFactoryShaderData
	{
	public:
	};

	class VertexFactory
	{
	public:
		virtual VertexFactoryType& getType() = 0;
		VertexDecl*  vertexDecl;
		static VertexFactoryShaderData* CreateShaderData() { return nullptr; }
		static void ModifyCompilationOption(ShaderCompileOption& option){}
	};

#define DECL_VERTEX_FACTORY_TYPE( CLASS )\
	friend class VertexFarcoryType;\
	public:\
		static VertexFactoryType StaticType;\
		virtual VertexFactoryType& getType() override { return StaticType; }

#define IMPL_VERTEX_FACTORY_TYPE( CLASS , FILE_NAME )\
	VertexFactoryType CLASS::StaticType( FILE_NAME , &CLASS::ModifyCompilationOption);\

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
