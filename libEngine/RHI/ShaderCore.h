#pragma once
#ifndef ShaderCore_H_999A5DE0_B9BF_41DF_8A7A_28D440730A62
#define ShaderCore_H_999A5DE0_B9BF_41DF_8A7A_28D440730A62

#include "RHICommon.h"

#include "HashString.h"
#include "FixString.h"

#include <vector>
#include <unordered_map>

namespace Render
{
	class RHICommandList;
	class RHIContext;
	struct ShaderEntryInfo;

	class ShaderCompileOption
	{
	public:
		ShaderCompileOption()
		{

		}

		void addInclude(char const* name)
		{
			mIncludeFiles.push_back(name);
		}
		void addDefine(char const* name, bool value)
		{
			addDefine(name, value ? 1 : 0);
		}
		void addDefine(char const* name, int value)
		{
			ConfigVar var;
			var.name = name;
			FixString<128> str;
			var.value = str.format("%d", value);
			mConfigVars.push_back(var);
		}
		void addDefine(char const* name, float value)
		{
			ConfigVar var;
			var.name = name;
			FixString<128> str;
			var.value = str.format("%f", value);
			mConfigVars.push_back(var);
		}
		void addDefine(char const* name)
		{
			ConfigVar var;
			var.name = name;
			mConfigVars.push_back(var);
		}
		void addCode(char const* name)
		{
			mCodes.push_back(name);
		}

		void addMeta(HashString key, char const* value)
		{
			mMetaMap[key] = value;
		}
		char const* getMeta(HashString key) const
		{
			auto iter = mMetaMap.find(key);
			if( iter == mMetaMap.end() )
				return nullptr;
			return iter->second.c_str();
		}

		bool isMetaMatch(HashString key, char const* value) const
		{
			char const* str = getMeta(key);
			if( str == nullptr )
				return false;
			return FCString::CompareIgnoreCase(str, value) == 0;
		}

		struct ConfigVar
		{
			std::string name;
			std::string value;
		};

		std::string getCode(ShaderEntryInfo const& entry, char const* defCode = nullptr , char const* addionalCode = nullptr ) const;

		bool     bShowComplieInfo = false;

		std::vector< std::string > mCodes;
		std::vector< ConfigVar >   mConfigVars;
		std::vector< std::string > mIncludeFiles;
		std::unordered_map< HashString, std::string > mMetaMap;
	};

	enum BlendMode
	{
		Blend_Opaque,
		Blend_Masked,
		Blend_Translucent,
		Blend_Additive,
		Blend_Modulate,

		NumBlendMode,
	};


	enum class ETessellationMode
	{
		None,
		Flat,
		PNTriangle,
	};

	struct MaterialShaderCompileInfo
	{
		char const* name;
		BlendMode blendMode;
		ETessellationMode tessellationMode;
	};

	class ShaderParameter
	{
	public:
		ShaderParameter()
		{
			mLoc = -1;
		}
		ShaderParameter(int loc)
			:mLoc(loc)
		{

		}
		ShaderParameter(uint32 inBindIndex, uint16 inOffset, uint16 inSize)
			:bindIndex(inBindIndex),offset(inOffset),size(inSize)
		{

		}
		bool isBound() const
		{
			return mLoc != -1;
		}
	public:
		friend class ShaderProgram;
		union 
		{
			int32 mLoc;
			struct
			{
				uint32 bindIndex;
				uint16 offset;
				uint16 size;
			};
		};
	};


	struct ShaderParameterMap
	{

		void addParameter(char const* name, uint32 bindIndex, uint16 offset = 0, uint16 size = 0)
		{
			ShaderParameter entry = { bindIndex, offset, size };
			mMap.emplace(name, entry);
		}

		bool bind(ShaderParameter& param, char const* name) const
		{
			auto iter = mMap.find(name);
			if( iter == mMap.end() )
			{
				param = ShaderParameter();
				return false;
			}
			param = iter->second;
			return true;
		}

		std::unordered_map< HashString , ShaderParameter > mMap;
	};


	struct StructuredBufferInfo
	{
		char const* blockName;

		StructuredBufferInfo(char const* name)
			:blockName(name)
		{
		}
	};

#define DECLARE_BUFFER_STRUCT( NAME )\
	static StructuredBufferInfo& GetStructInfo()\
	{\
		static StructuredBufferInfo sMyStruct( #NAME );\
		return sMyStruct;\
	}

	enum class EShaderResourceType
	{
		Uniform ,
		Storage ,
		AtomicCounter ,
	};

	class RHIShader : public RHIResource
	{


	};
	typedef TRefCountPtr< RHIShader > RHIShaderRef;

	class RHIShaderProgram : public RHIResource
	{
	public:
		virtual bool setupShaders(RHIShaderRef shaders[], int numShader) = 0;
		virtual bool getParameter(char const* name, ShaderParameter& outParam) = 0;
		virtual bool getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParam) = 0;
	};

	
	typedef TRefCountPtr< RHIShaderProgram > RHIShaderProgramRef;

	class ShaderResource
	{
	public:
		virtual ~ShaderResource();
	};




}//namespace Render

#endif // ShaderCore_H_999A5DE0_B9BF_41DF_8A7A_28D440730A62