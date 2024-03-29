#pragma once
#ifndef VertexFactory_H_86132998_96CC_46B9_8772_AD134BCD249E
#define VertexFactory_H_86132998_96CC_46B9_8772_AD134BCD249E

#include "RHICommon.h"
#include "CoreShare.h"

namespace Render
{
	class ShaderCompileOption;
	class VertexFactoryShaderData;
	class VertexFactoryType;
	class MaterialShaderProgram;

	class VertexFactoryType
	{
	public:

		using CreateShaderDataFunc = VertexFactoryShaderData* (*)();
		using ModifyCompilationOptionFunc = void (*)(ShaderCompileOption& opation);

		CORE_API VertexFactoryType(char const* inFileName , ModifyCompilationOptionFunc MCO );
		
		char const* fileName;
		ModifyCompilationOptionFunc ModifyCompilationOption;
		CreateShaderDataFunc CreateShaderData;

		void getCompileOption(ShaderCompileOption& option);
		virtual void setupShader( MaterialShaderProgram& shader , VertexFactoryShaderData* shaderData)
		{

		}
		
		CORE_API static VertexFactoryType* DefaultType;
		CORE_API static TArray< VertexFactoryType* > TypeList;
	};

	class VertexFactoryShaderData
	{
	public:
	};

	class VertexFactory
	{
	public:
		virtual VertexFactoryType& getType() = 0;

		static VertexFactoryShaderData* CreateShaderData() { return nullptr; }
		static void ModifyCompilationOption(ShaderCompileOption& option){}
	};

#define DECL_VERTEX_FACTORY_TYPE( CLASS )\
	friend class VertexFarcoryType;\
	public:\
		CORE_API static VertexFactoryType StaticType;\
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

}//Render

#endif // VertexFactory_H_86132998_96CC_46B9_8772_AD134BCD249E
