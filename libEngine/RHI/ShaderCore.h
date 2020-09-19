#pragma once
#ifndef ShaderCore_H_999A5DE0_B9BF_41DF_8A7A_28D440730A62
#define ShaderCore_H_999A5DE0_B9BF_41DF_8A7A_28D440730A62

#include "RHICommon.h"

#include "HashString.h"
#include "Core/StringConv.h"

#include <vector>
#include <unordered_map>


namespace Render
{
	class RHICommandList;
	class RHIContext;
	struct ShaderEntryInfo;
	class ShaderParameterMap;


	struct ShaderEntryInfo
	{
		EShader::Type type;
		char const*  name;

#if 0
		ShaderEntryInfo() = default;
		ShaderEntryInfo(EShader::Type inType, char const* inName)
			:type(inType), name(inName)
		{

		}
#endif
	};


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
			var.value = FStringConv::From(value);
			mConfigVars.push_back(var);
		}
		void addDefine(char const* name, float value)
		{
			ConfigVar var;
			var.name = name;
			var.value = FStringConv::From(value);
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

		void appendMeta(HashString key, char const* value)
		{
			std::string metaValue = mMetaMap[key];
			if (metaValue.size())
			{
				metaValue += ' ';
				metaValue += value;
			}
			else
			{
				metaValue = value;
			}
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

	enum class EShaderParamBindType
	{
		Uniform ,
		Texture ,
		Sampler ,
		UniformBuffer,
		StorageBuffer ,

		Unknown,
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

		ShaderParameter(uint32 inBindIndex, uint32 inOffset, uint32 inSize)
			:bindIndex(inBindIndex), offset(inOffset), size(inSize)
		{

		}
		bool isBound() const
		{
			return mLoc != -1;
		}

		bool bind(ShaderParameterMap const& paramMap, char const* name);
	public:
		friend class ShaderProgram;

#if _DEBUG
		HashString mName;
		bool bHasLogWarning = false;
		EShaderParamBindType mbindType = EShaderParamBindType::Unknown;
#endif
		union
		{
			int32 mLoc;
			struct
			{
				uint32 bindIndex;
				uint32 offset;
				uint32 size;
			};
		};
	};

	class ShaderParameterMap
	{
	public:
		ShaderParameter& addParameter(char const* name, uint32 bindIndex, uint16 offset = 0, uint16 size = 0)
		{
			ShaderParameter entry = { bindIndex, offset, size };

			auto& result = mMap[name];
			result = entry;
			return result;
		}
		void clear()
		{
			mMap.clear();
		}
		std::unordered_map< HashString, ShaderParameter > mMap;
	};

	class ShaderPorgramParameterMap : public ShaderParameterMap
	{
	public:

		void clear();

		void addShaderParameterMap(EShader::Type shaderType, ShaderParameterMap const& parameterMap);

		void finalizeParameterMap();

		template< class TFunc >
		void setupShader(ShaderParameter const& parameter, TFunc&& func)
		{
			if (parameter.mLoc < 0 || parameter.mLoc >= mParamEntryMap.size())
				return;
			assert(0 <= parameter.mLoc && parameter.mLoc < mParamEntryMap.size());
			auto const& entry = mParamEntryMap[parameter.mLoc];

			ShaderParamEntry* pParamEntry = &mParamEntries[entry.paramIndex];
			for (int i = entry.numParam; i; --i)
			{
				func(pParamEntry->type, pParamEntry->param);
				++pParamEntry;
			}
		}

		struct ParameterEntry
		{
			uint16  numParam;
			uint16  paramIndex;
		};

		struct ShaderParamEntry
		{
			int loc;
			EShader::Type    type;
			ShaderParameter param;
		};

		std::vector< ParameterEntry >   mParamEntryMap;
		std::vector< ShaderParamEntry > mParamEntries;

	};


	struct StructuredBufferInfo
	{
		char const* blockName;
		char const* variableName;

		StructuredBufferInfo(char const* bloackName , char const* varName = nullptr)
			:blockName(bloackName)
			,variableName(varName)
		{
		}
	};

#define DECLARE_UNIFORM_BUFFER_STRUCT( NAME)\
	static StructuredBufferInfo& GetStructInfo()\
	{\
		static StructuredBufferInfo sMyStruct( #NAME , nullptr );\
		return sMyStruct;\
	}

#define DECLARE_BUFFER_STRUCT( NAME , VAR )\
	static StructuredBufferInfo& GetStructInfo()\
	{\
		static StructuredBufferInfo sMyStruct( #NAME , #VAR );\
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
	public:
		RHIShader():RHIResource(TRACE_TYPE_NAME("Shader")){}

		virtual bool getParameter(char const* name, ShaderParameter& outParam) = 0;
		virtual bool getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParam) = 0;
		virtual char const* getStructParameterName(EShaderResourceType resourceType, StructuredBufferInfo const& structInfo) { return structInfo.blockName; }

		void initType(EShader::Type type)
		{
			mType = type;
			FRHIResourceTable::Register(*this);
		}

		EShader::Type getType() const { return mType; }

		EShader::Type mType = EShader::Empty;
		uint32 mGUID = 0;
	};

	using RHIShaderRef = TRefCountPtr< RHIShader >;
	


	class RHIShaderProgram : public RHIResource
	{
	public:
		RHIShaderProgram() :RHIResource(TRACE_TYPE_NAME("ShaderProgram")) 
		{
			FRHIResourceTable::Register(*this);
		}

		virtual bool getParameter(char const* name, ShaderParameter& outParam) = 0;
		virtual bool getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParam) = 0;
		virtual char const* getStructParameterName(EShaderResourceType resourceType, StructuredBufferInfo const& structInfo) { return structInfo.blockName; }

		uint32 mGUID = 0;
	};

	using RHIShaderProgramRef = TRefCountPtr< RHIShaderProgram >;

}//namespace Render

#endif // ShaderCore_H_999A5DE0_B9BF_41DF_8A7A_28D440730A62