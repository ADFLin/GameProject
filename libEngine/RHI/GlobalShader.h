#pragma once
#ifndef GlobalShader_H_99D977DA_B0C5_46A1_8282_C63EA1B52228
#define GlobalShader_H_99D977DA_B0C5_46A1_8282_C63EA1B52228

#include "ShaderProgram.h"
#include "ShaderPermutation.h"

#include "Template/ArrayView.h"
#include "Meta/Concept.h"

namespace Render
{
	class GlobalShaderProgram : public ShaderProgram
	{
	public:
		static GlobalShaderProgram* CreateShader() { assert(0); return nullptr; }
		static void SetupShaderCompileOption(ShaderCompileOption& option) {}
		static char const* GetShaderFileName()
		{
			assert(0);
			return nullptr;
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{

			assert(0);
#if 0
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
#else
			return TArrayView< ShaderEntryInfo const >();
#endif
		}
		class GlobalShaderProgramClass const* myClass;
	};

	class GlobalShader : public Shader
	{
	public:
		static GlobalShader* CreateShader() { assert(0); return nullptr; }
		static void SetupShaderCompileOption(ShaderCompileOption& option) {}
		static char const* GetShaderFileName()
		{
			assert(0);
			return nullptr;
		}
		class GlobalShaderClass const* myClass;
	};

	class GlobalShaderObjectClass
	{
	public:
		typedef ShaderObject* (*CreateShaderObjectFunc)();
		typedef void(*SetupShaderCompileOptionFunc)(ShaderCompileOption&, uint32);
		typedef char const* (*GetShaderFileNameFunc)();

		CreateShaderObjectFunc CreateShaderObject;
		SetupShaderCompileOptionFunc SetupShaderCompileOption;
		GetShaderFileNameFunc GetShaderFileName;

		CORE_API GlobalShaderObjectClass(
			CreateShaderObjectFunc inCreateShaderObject,
			SetupShaderCompileOptionFunc inSetupShaderCompileOption,
			GetShaderFileNameFunc inGetShaderFileName);
	};

	class GlobalShaderProgramClass : public GlobalShaderObjectClass
	{
	public:

		typedef TArrayView< ShaderEntryInfo const > (*GetShaderEntriesFunc)();
	
		GetShaderEntriesFunc GetShaderEntries;

		CORE_API GlobalShaderProgramClass(
			CreateShaderObjectFunc inCreateShaderObject,
			SetupShaderCompileOptionFunc inSetupShaderCompileOption,
			GetShaderFileNameFunc inGetShaderFileName,
			GetShaderEntriesFunc inGetShaderEntries,
			uint32 inPermutationCount);
	};

	class GlobalShaderClass : public GlobalShaderObjectClass
	{
	public:
		ShaderEntryInfo  entry;

		CORE_API GlobalShaderClass(
			CreateShaderObjectFunc inCreateShaderObject,
			SetupShaderCompileOptionFunc inSetupShaderCompileOption,
			GetShaderFileNameFunc inGetShaderFileName,
			ShaderEntryInfo entry,
			uint32 inPermutationCount);
	};

	
	struct CPermutationDomainDefined
	{
		template< class T >
		static auto requires(typename T::PermutationDomain& value) -> decltype
		(
			value
		);
	};

	template< class TShaderClass >
	void SetupShaderCompileOption(ShaderCompileOption& option , uint32 permutationId)
	{
		if constexpr (TCheckConcept<CPermutationDomainDefined , TShaderClass >::Value)
		{
			typename TShaderClass::PermutationDomain domain;
			domain.setPermutationId(permutationId);
			domain.setupShaderCompileOption(option);
			TShaderClass::SetupShaderCompileOption(option);
		}
		else
		{
			TShaderClass::SetupShaderCompileOption(option);
		}
	}

	template< class TShaderClass >
	uint32 GetShaderPermutationCount()
	{
		if constexpr (TCheckConcept<CPermutationDomainDefined, TShaderClass >::Value)
		{
			return TShaderClass::PermutationDomain::GetPermutationCount();
		}
		else
		{
			return 1;
		}
	}


	class GlobalShaderObjectClass;

	class MaterialShaderClass;
	class GlobalShaderClass;

	class MaterialShaderProgramClass;
	class GlobalShaderProgramClass;

#define DECLARE_EXPORTED_SHADER_PROGRAM( CLASS , CALSS_TYPE_NAME , API , ...)\
	public: \
		using ThisClass = CLASS;\
		using ShaderClassType = CALSS_TYPE_NAME##ShaderProgramClass; \
		static API ShaderClassType ShaderClass; \
		static ShaderClassType const& GetShaderClass(){ return ShaderClass; }\
		static CLASS* CreateShaderObject() { return new CLASS(); }

#define DECLARE_EXPORTED_SHADER( CLASS , CALSS_TYPE_NAME , API , ...)\
	public: \
		using ThisClass = CLASS;\
		using ShaderClassType = CALSS_TYPE_NAME##ShaderClass; \
		static API ShaderClassType ShaderClass; \
		static ShaderClassType const& GetShaderClass(){ return ShaderClass; }\
		static CLASS* CreateShaderObject() { return new CLASS(); }

#define DECLARE_SHADER_PROGRAM( CLASS , CALSS_TYPE_NAME , ...) DECLARE_EXPORTED_SHADER_PROGRAM( CLASS , CALSS_TYPE_NAME , , __VA_ARGS__ )
#define DECLARE_SHADER( CLASS , CALSS_TYPE_NAME , ...)         DECLARE_EXPORTED_SHADER( CLASS , CALSS_TYPE_NAME , , __VA_ARGS__ )

#define IMPLEMENT_SHADER_PROGRAM( CLASS )\
	CLASS::ShaderClassType CLASS::ShaderClass\
	(\
		(GlobalShaderObjectClass::CreateShaderObjectFunc) &CLASS::CreateShaderObject,\
		::Render::SetupShaderCompileOption< CLASS >,\
		CLASS::GetShaderFileName, \
		CLASS::GetShaderEntries, \
		::Render::GetShaderPermutationCount< CLASS >()\
	)
#define IMPLEMENT_SHADER_PROGRAM_T( TEMPLATE_ARGS , CLASS )\
	TEMPLATE_ARGS \
	IMPLEMENT_SHADER_PROGRAM( CLASS )


#define IMPLEMENT_SHADER( CLASS , SHADER_TYPE , ENTRY_NAME )\
	CLASS::ShaderClassType CLASS::ShaderClass\
	(\
		(GlobalShaderObjectClass::CreateShaderObjectFunc) &CLASS::CreateShaderObject,\
		::Render::SetupShaderCompileOption< CLASS >,\
		CLASS::GetShaderFileName, \
		{ SHADER_TYPE , ENTRY_NAME }, \
		::Render::GetShaderPermutationCount< CLASS >()\
	)

#define IMPLEMENT_SHADER_T( TEMPLATE_ARGS , CLASS , SHADER_TYPE , ENTRY_NAME )\
	TEMPLATE_ARGS \
	IMPLEMENT_SHADER( CLASS , SHADER_TYPE , ENTRY_NAME)

}//namespace Render

#endif // GlobalShader_H_99D977DA_B0C5_46A1_8282_C63EA1B52228