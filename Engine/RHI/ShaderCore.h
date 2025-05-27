#pragma once
#ifndef ShaderCore_H_999A5DE0_B9BF_41DF_8A7A_28D440730A62
#define ShaderCore_H_999A5DE0_B9BF_41DF_8A7A_28D440730A62

#include "RHICommon.h"

#include "HashString.h"
#include "Core/StringConv.h"

#include <vector>
#include <unordered_map>

#define SHADER_DEBUG _DEBUG

namespace Render
{
	class RHICommandList;
	class RHIContext;
	struct ShaderEntryInfo;
	class ShaderParameterMap;
	class ShaderFormat;

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

	enum class EShaderCodePos : uint8
	{
		BeforeCommon,
		AfterCommon,
		BeforeInclude,
		AfterInclude,
		Last,
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
			mConfigVars.push_back(std::move(var));
		}
		void addDefine(char const* name, float value)
		{
			ConfigVar var;
			var.name = name;
			var.value = FStringConv::From(value);
			mConfigVars.push_back(std::move(var));
		}
		void addDefine(char const* name, char const* value)
		{
			ConfigVar var;
			var.name = name;
			var.value = value;
			mConfigVars.push_back(std::move(var));
		}
		void addDefine(char const* name, std::string const& value)
		{
			ConfigVar var;
			var.name = name;
			var.value = value;
			mConfigVars.push_back(std::move(var));
		}
		void addDefine(char const* name)
		{
			ConfigVar var;
			var.name = name;
			mConfigVars.push_back(std::move(var));
		}
		
		void addCode(char const* code, EShaderCodePos pos = EShaderCodePos::Last)
		{
			mCodes.emplace_back(pos, code);
		}

		void addMeta(HashString key, char const* value)
		{
			mMetaMap[key] = value;
		}

		void appendMeta(HashString key, char const* value)
		{
			std::string& metaValue = mMetaMap[key];
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

		static std::string GetIncludeFileName(char const* name);
		struct ConfigVar
		{
			std::string name;
			std::string value;
		};

		std::string getCode(ShaderEntryInfo const& entry, char const* defCode = nullptr) const;

		void setup(ShaderFormat& shaderFormat) const;

		bool     bShowComplieInfo = false;
		bool     bHadShaderFormatSetup = false;

		struct CodeInfo
		{
			EShaderCodePos pos;
			std::string    content;

			CodeInfo(EShaderCodePos pos, char const* code)
				:pos(pos),content(code){}
		};
		TArray< CodeInfo > mCodes;
		TArray< ConfigVar >   mConfigVars;
		TArray< std::string > mIncludeFiles;
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


		void setup(ShaderCompileOption& option) const;
	};

	enum class EShaderParamBindType
	{
		Uniform ,
		Texture ,
		Sampler ,
		UniformBuffer ,
		TextureBuffer ,
		AtomCounter ,
		StorageBuffer ,

		Unknown,
	};

	class ShaderParameter
	{
	public:
		ShaderParameter()
		{
			mLoc = INDEX_NONE;
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

#if SHADER_DEBUG
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
#if SHADER_DEBUG
			result.mName = name;
#endif
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

		void addShaderParameterMap(int shaderIndex, ShaderParameterMap const& parameterMap);

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
				func(pParamEntry->shaderIndex, pParamEntry->param);
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
			int shaderIndex;
			ShaderParameter param;
		};

		TArray< ParameterEntry >   mParamEntryMap;
		TArray< ShaderParamEntry > mParamEntries;

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

#define MAKE_STRUCTUREED_BUFFER_INFO( NAME ) Render::StructuredBufferInfo{ #NAME"Block" , #NAME }

#define DECLARE_UNIFORM_BUFFER_STRUCT( NAME)\
	static Render::StructuredBufferInfo& GetStructInfo()\
	{\
		static Render::StructuredBufferInfo sMyStruct( #NAME , nullptr );\
		return sMyStruct;\
	}

#define DECLARE_BUFFER_STRUCT( VAR )\
	static Render::StructuredBufferInfo& GetStructInfo()\
	{\
		static Render::StructuredBufferInfo sMyStruct( #VAR"Block" , #VAR );\
		return sMyStruct;\
	}

	enum class EShaderResourceType
	{
		Uniform ,
		Storage ,
		AtomicCounter ,
	};

	class RHIShaderObject : public RHIResource
	{
	public:
		using RHIResource::RHIResource;

		virtual bool getParameter(char const* name, ShaderParameter& outParam) = 0;
		virtual bool getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParam) = 0;
		virtual bool getResourceParameter(EShaderResourceType resourceType, StructuredBufferInfo const& structInfo, ShaderParameter& outParam) = 0;
		virtual char const* getParameterName(EShaderResourceType resourceType, StructuredBufferInfo const& structInfo) = 0;
	};

	class RHIShader : public RHIShaderObject
	{
	public:
		RHIShader():RHIShaderObject(TRACE_TYPE_NAME("Shader")){}

		void initType(EShader::Type type)
		{
			mType = type;
			FRHIResourceTable::Register(*this);
		}

		EShader::Type getType() const { return mType; }
		EShader::Type mType = EShader::Empty;
		uint32 mGUID = 0;
	};

	using RHIShaderRef = TRHIResourceRef< RHIShader >;
	
	class RHIShaderProgram : public RHIShaderObject
	{
	public:
		RHIShaderProgram() :RHIShaderObject(TRACE_TYPE_NAME("ShaderProgram"))
		{
			FRHIResourceTable::Register(*this);
		}

		uint32 mGUID = 0;
	};

	using RHIShaderProgramRef = TRHIResourceRef< RHIShaderProgram >;

	struct ShaderBoundStateKey
	{
		enum Type
		{
			eNone = 0,
			eGraphiscsVsPs,
			eGraphiscsVsPsGs,
			eGraphiscsVsPsGsHsDs,
			eMesh,
			eCompute,
			eShaderProgram,
		};

		enum 
		{
			TYPE_BIT_COUNT = 3,
		};
		union
		{
			struct
			{
				uint64 type : TYPE_BIT_COUNT;
				uint64 sv   : 61;
			};
			struct
			{
				uint64 type : TYPE_BIT_COUNT;
				uint64 sv21 : 31;
				uint64 sv22 : 30;
			};

			struct
			{
				uint64 type : TYPE_BIT_COUNT;
				uint64 sv31 : 23;
				uint64 sv32 : 22;
				uint64 sv33 : 16;
			};

			struct
			{
				uint64 type : TYPE_BIT_COUNT;
				uint64 sv51 : 15;
				uint64 sv52 : 14;
				uint64 sv53 : 11;
				uint64 sv54 : 11;
				uint64 sv55 : 10;
			};
			uint64 value;
		};


		static uint32 GetGUID(RHIShader* shader)
		{
			return shader ? shader->mGUID : 0;
		}
		void initialize(GraphicsShaderStateDesc const& stateDesc)
		{
			if (stateDesc.hull || stateDesc.domain)
			{
				type = eGraphiscsVsPsGsHsDs;
				sv51 = GetGUID(stateDesc.pixel);
				sv52 = GetGUID(stateDesc.vertex);
				sv53 = GetGUID(stateDesc.hull);
				sv54 = GetGUID(stateDesc.domain);
				sv55 = GetGUID(stateDesc.geometry);
			}
			else if (stateDesc.geometry)
			{
				type = eGraphiscsVsPsGs;
				sv31 = GetGUID(stateDesc.pixel);
				sv32 = GetGUID(stateDesc.vertex);
				sv33 = GetGUID(stateDesc.geometry);
			}
			else if (stateDesc.pixel || stateDesc.vertex)
			{
				type = eGraphiscsVsPs;
				sv21 = GetGUID(stateDesc.pixel);
				sv22 = GetGUID(stateDesc.vertex);
			}
			else
			{
				value = 0;
			}
		}

		void initialize(MeshShaderStateDesc const& stateDesc)
		{
			type = eNone;
			sv31 = GetGUID(stateDesc.pixel);
			sv32 = GetGUID(stateDesc.mesh);
			sv33 = GetGUID(stateDesc.task);
			if (value)
			{
				type = eMesh;
			}
		}

		void initialize(RHIShader& shader)
		{
			CHECK(shader.getType() == EShader::Compute);

			type = eCompute;
			sv = shader.mGUID;
		}

		void initialize(RHIShaderProgram& shaderProgram)
		{
			type = eShaderProgram;
			sv = shaderProgram.mGUID;
		}

		bool isValid() const
		{
			return !!value;
		}

		bool operator == (ShaderBoundStateKey const& rhs) const
		{
			return value == rhs.value;
		}
		uint32 getTypeHash() const
		{
			uint32 result = HashValue(value);
			return result;
		}

#if 0
		ShaderBoundStateKey()
		{
		}
#endif
	};

}//namespace Render

#endif // ShaderCore_H_999A5DE0_B9BF_41DF_8A7A_28D440730A62