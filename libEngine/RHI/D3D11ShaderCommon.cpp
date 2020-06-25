#include "D3D11ShaderCommon.h"

#include "D3D11Common.h"
#include "ShaderProgram.h"

#include "FileSystem.h"
#include "CPreprocessor.h"

#include <streambuf>
#include <fstream>
#include <sstream>
#include "Serialize/StreamBuffer.h"
#include "RHICommand.h"


namespace Render
{

	void ShaderFormatHLSL::setupShaderCompileOption(ShaderCompileOption& option)
	{
		option.addMeta("ShaderFormat", getName());
	}

	void ShaderFormatHLSL::getHeadCode(std::string& inoutCode, ShaderCompileOption const& option, ShaderEntryInfo const& entry)
	{
		inoutCode += "#define COMPILER_HLSL 1\n";
	}

	bool ShaderFormatHLSL::compileCode(ShaderCompileInput const& input, ShaderCompileOutput& output)
	{
		bool bSuccess;
		do
		{
			bSuccess = true;
			std::vector< char > codeBuffer;

			if( bUsePreprocess )
			{
				if( !FileUtility::LoadToBuffer(input.path, codeBuffer, true) )
				{
					LogWarning(0, "Can't load shader file %s", input.path);
					return false;
				}

				if (!PreprocessCode(input.path, output.compileInfo, input.definition, codeBuffer))
					return false;
			}
			else
			{
				if(input.definition)
				{
					int lenDef = strlen(input.definition);
					codeBuffer.resize(lenDef);
					codeBuffer.assign(input.definition, input.definition + lenDef);
				}

				if( !FileUtility::LoadToBuffer(input.path, codeBuffer, true, true) )
				{
					LogWarning(0, "Can't load shader file %s", input.path);
					return false;
				}
			}

			TComPtr< ID3D10Blob > errorCode;
			TComPtr< ID3D10Blob > byteCode;
			FixString<32> profileName = FD3D11Utility::GetShaderProfile( mDevice , input.type);

			uint32 compileFlag = 0 /*| D3D10_SHADER_PACK_MATRIX_ROW_MAJOR*/;
			VERIFY_D3D11RESULT(
				D3DX11CompileFromMemory(codeBuffer.data(), codeBuffer.size(), "ShaderCode", NULL, NULL, output.compileInfo->entryName.c_str(), 
										profileName, compileFlag, 0, NULL, &byteCode, &errorCode, NULL),
				{
					bSuccess = false;
				}
			);

			if( !bSuccess )
			{
				if( bUsePreprocess )
				{
					FileUtility::SaveFromBuffer("temp" SHADER_FILE_SUBNAME, codeBuffer.data(), codeBuffer.size());
				}

				if( bRecompile )
				{
					OutputError((LPCSTR)errorCode->GetBufferPointer());
				}

			}

			auto* shaderImpl = static_cast<D3D11Shader*>(RHICreateShader(input.type));
			output.resource = shaderImpl;

			if( !shaderImpl->initialize(input.type, mDevice, byteCode) )
			{
				LogWarning(0, "Can't create shader resource");
				return false;
			}

		} while( !bSuccess && bRecompile );

		return bSuccess;
	}

	bool ShaderFormatHLSL::initializeProgram(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(*shaderProgram.mRHIResource);

		if (!shaderProgramImpl.setupShaders(setupData.shaderResources.data(), setupData.shaderResources.size()))
			return false;

		shaderProgram.bindParameters(shaderProgramImpl.mParameterMap);
		return true;
	}

	bool ShaderFormatHLSL::initializeProgram(ShaderProgram& shaderProgram, std::vector< ShaderCompileInfo > const& shaderCompiles, std::vector<uint8> const& binaryCode)
	{
		D3D11ShaderProgram& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(*shaderProgram.mRHIResource);

		auto serializer = CreateBufferSerializer<SimpleReadBuffer>(MakeView(binaryCode));
		uint8 numShaders = 0;
		serializer.read(numShaders);

		ShaderResourceInfo shaders[EShader::Count];
		for (int i = 0; i < numShaders; ++i)
		{
			D3D11Shader& shaderImpl = *shaderProgramImpl.mShaders[i];
			uint8 shaderType;
			serializer.read(shaderType);
			std::vector< uint8 > byteCode;
			serializer.read(byteCode);

			D3D11Shader* shader = (D3D11Shader*)RHICreateShader(EShader::Type(shaderType));
			shaders[i].resource = shader;
			shaders[i].type = EShader::Type(shaderType);
			shaders[i].entry = shaderCompiles[i].entryName.c_str();

			if (!shader->initialize(EShader::Type(shaderType), mDevice, std::move(byteCode)))
				return false;
		}

		if (!shaderProgramImpl.setupShaders(shaders, numShaders))
			return false;

		shaderProgram.bindParameters(shaderProgramImpl.mParameterMap);
		return true;
	}

	bool ShaderFormatHLSL::isSupportBinaryCode() const
	{
		return true;
	}

	bool ShaderFormatHLSL::getBinaryCode(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData, std::vector<uint8>& outBinaryCode)
	{
		D3D11ShaderProgram& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(*shaderProgram.mRHIResource);
		auto serializer = CreateBufferSerializer<VectorWriteBuffer>(outBinaryCode);
		uint8 numShaders = shaderProgramImpl.mNumShaders;
		serializer.write(numShaders);
		for (int i = 0; i < numShaders; ++i)
		{
			D3D11Shader& shaderImpl = *shaderProgramImpl.mShaders[i];
			uint8 shaderType = shaderImpl.mResource.type;
			serializer.write(shaderType);
			serializer.write(shaderImpl.byteCode);
		}

		return true;
	}

	void ShaderFormatHLSL::postShaderLoaded(ShaderProgram& shaderProgram)
	{
		D3D11ShaderProgram& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(*shaderProgram.mRHIResource);
		for (int i = 0; i < shaderProgramImpl.mNumShaders; ++i)
		{
			D3D11Shader& shader = *shaderProgramImpl.mShaders[i];
			if (shader.mResource.type != EShader::Vertex)
			{
				shader.byteCode.clear();
			}
		}
	}

	bool D3D11Shader::initialize(EShader::Type type, TComPtr<ID3D11Device>& device, TComPtr<ID3D10Blob>& inByteCode)
	{
		if (!createResource(type, device, (uint8*)inByteCode->GetBufferPointer(), inByteCode->GetBufferSize()))
			return false;

		byteCode.assign((uint8*)inByteCode->GetBufferPointer() , (uint8*)(inByteCode->GetBufferPointer()) + inByteCode->GetBufferSize());

		return true;
	}

	bool D3D11Shader::initialize(EShader::Type type, TComPtr<ID3D11Device>& device, std::vector<uint8>&& inByteCode)
	{
		if (!createResource(type, device, inByteCode.data(), inByteCode.size()))
			return false;

		byteCode = std::move(inByteCode);
		return true;
	}

	bool D3D11Shader::createResource(EShader::Type type, TComPtr<ID3D11Device>& device , uint8 const* pCode , size_t codeSize)
	{
		switch (type)
		{
#define CASE_SHADER(  TYPE , FUNC ,VAR )\
		case TYPE:\
			VERIFY_D3D11RESULT_RETURN_FALSE( device->FUNC(pCode, codeSize, NULL, &VAR) );\
			break;

		CASE_SHADER(EShader::Vertex, CreateVertexShader, mResource.vertex);
		CASE_SHADER(EShader::Pixel, CreatePixelShader, mResource.pixel);
		CASE_SHADER(EShader::Geometry, CreateGeometryShader, mResource.geometry);
		CASE_SHADER(EShader::Compute, CreateComputeShader, mResource.compute);
		CASE_SHADER(EShader::Hull, CreateHullShader, mResource.hull);
		CASE_SHADER(EShader::Domain, CreateDomainShader, mResource.domain);

#undef CASE_SHADER
		default:
			NEVER_REACH("D3D11Shader::createResource unknown shader Type");
		}
		mResource.type = type;
		mResource.ptr->AddRef();
		return true;
	}

	bool D3D11Shader::GenerateParameterMap(std::vector< uint8 > const& byteCode, ShaderParameterMap& parameterMap)
	{
		TComPtr< ID3D11ShaderReflection > reflection;
		VERIFY_D3D11RESULT_RETURN_FALSE(D3DReflect(byteCode.data(), byteCode.size(), IID_ID3D11ShaderReflection, (void**)&reflection.mPtr));

		D3D11_SHADER_DESC shaderDesc;
		reflection->GetDesc(&shaderDesc);
		for( int i = 0; i < shaderDesc.BoundResources; ++i )
		{
			D3D11_SHADER_INPUT_BIND_DESC bindDesc;
			reflection->GetResourceBindingDesc(i, &bindDesc);

			switch( bindDesc.Type )
			{
			case D3D11_CT_CBUFFER:
			case D3D11_CT_TBUFFER:
				{
					ID3D11ShaderReflectionConstantBuffer* buffer = reflection->GetConstantBufferByName(bindDesc.Name);
					assert(buffer);
					D3D11_SHADER_BUFFER_DESC bufferDesc;
					buffer->GetDesc(&bufferDesc);
					if( FCString::Compare(bufferDesc.Name, "$Globals") == 0 )
					{
						for( int idxVar = 0; idxVar < bufferDesc.Variables; ++idxVar )
						{
							ID3D11ShaderReflectionVariable* var = buffer->GetVariableByIndex(idxVar);
							if( var )
							{
								D3D11_SHADER_VARIABLE_DESC varDesc;
								var->GetDesc(&varDesc);
								parameterMap.addParameter(varDesc.Name, bindDesc.BindPoint, varDesc.StartOffset, varDesc.Size);
							}
						}
					}
					else
					{
						//CBuffer TBuffer
						parameterMap.addParameter(bufferDesc.Name, bindDesc.BindPoint, 0, 0);
					}
				}
				break;
			case D3D11_CT_RESOURCE_BIND_INFO:
				{
					parameterMap.addParameter(bindDesc.Name, bindDesc.BindPoint, 0, bindDesc.BindCount);
				}
				break;
			case D3D_SIT_TEXTURE:
				{
					parameterMap.addParameter(bindDesc.Name, bindDesc.BindPoint, 0, bindDesc.BindCount);
				}
				break;
			}
		}

		return true;
	}

	bool D3D11ShaderProgram::setupShaders(ShaderResourceInfo shaders[], int numShaders)
	{
		assert(ARRAY_SIZE(mShaders) >= numShaders);
		mNumShaders = numShaders;
		for (int i = 0; i < numShaders; ++i)
		{
			mShaders[i] = &static_cast< D3D11Shader& >(*shaders[i].resource);
			auto& shaderImpl = *mShaders[i];
			ShaderParameterMap parameterMap;
			D3D11Shader::GenerateParameterMap(shaderImpl.byteCode, parameterMap);
			addShaderParameterMap(shaderImpl.mResource.type, parameterMap);
		}
		finalizeParameterMap();
		return true;
	}

	bool D3D11ShaderProgram::getParameter(char const* name, ShaderParameter& outParam)
	{
		return outParam.bind(mParameterMap, name);
	}

	bool D3D11ShaderProgram::getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParam)
	{
		return outParam.bind(mParameterMap, name);
	}

	void D3D11ShaderProgram::releaseResource()
	{
		for (int i = 0; i < mNumShaders; ++i)
		{
			mShaders[i].release();
		}
	}

	void D3D11ShaderProgram::addShaderParameterMap(EShader::Type shaderType, ShaderParameterMap const& parameterMap)
	{
		for (auto const& pair : parameterMap.mMap)
		{
			auto& param = mParameterMap.mMap[pair.first];

			if (param.mLoc == -1)
			{
				param.mLoc = mParamEntryMap.size();
				ParameterEntry entry;
				entry.numParam = 0;
				mParamEntryMap.push_back(entry);
			}

			ParameterEntry& entry = mParamEntryMap[param.mLoc];

			entry.numParam += 1;
			ShaderParamEntry paramEntry;
			paramEntry.type = shaderType;
			paramEntry.param = pair.second;
			paramEntry.loc = param.mLoc;
			mParamEntries.push_back(paramEntry);
		}
	}

	void D3D11ShaderProgram::finalizeParameterMap()
	{
		std::sort(mParamEntries.begin(), mParamEntries.end(), [](ShaderParamEntry const& lhs, ShaderParamEntry const& rhs)
		{
			if (lhs.loc < rhs.loc)
				return true;
			else if (lhs.loc > rhs.loc)
				return false;
			return lhs.type < rhs.type;
		});

		int curLoc = -1;
		for (int index = 0; index < mParamEntries.size(); ++index)
		{
			auto& paramEntry = mParamEntries[index];
			if (paramEntry.loc != curLoc)
			{
				mParamEntryMap[paramEntry.loc].paramIndex = index;
				curLoc = paramEntry.loc;
			}
		}
	}

}