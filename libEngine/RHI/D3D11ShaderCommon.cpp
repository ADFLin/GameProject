#include "D3D11ShaderCommon.h"

#include "D3D11Common.h"
#include "ShaderProgram.h"

#include "FileSystem.h"
#include "CPreprocessor.h"

#include <streambuf>
#include <fstream>
#include <sstream>


namespace Render
{

	void ShaderFormatHLSL::setupParameters(ShaderProgram& shaderProgram)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(*shaderProgram.getRHIResource());
		shaderProgram.bindParameters(shaderProgramImpl.mParameterMap);
	}

	bool ShaderFormatHLSL::compileCode(Shader::Type type, RHIShader& shader, char const* path, ShaderCompileInfo* compileInfo, char const* def)
	{
		bool bSuccess;
		do
		{
			bSuccess = true;
			std::vector< char > codeBuffer;

			if( bUsePreprocess )
			{
				if( !FileUtility::LoadToBuffer(path, codeBuffer, true) )
				{
					LogWarning(0, "Can't load shader file %s", path);
					return false;
				}


				CPP::CodeInput sourceInput;

				if( def )
				{
					sourceInput.appendString(def);
				}
				sourceInput.appendString(&codeBuffer[0]);

				sourceInput.resetSeek();

				CPP::Preprocessor preporcessor;

				std::stringstream oss;
				CPP::CodeOutput codeOutput(oss);

				char const* DefaultDir = "Shader";
				preporcessor.setOutput(codeOutput);
				preporcessor.addSreachDir(DefaultDir);
				char const* dirPathEnd = FileUtility::GetFileName(path);
				if( dirPathEnd != path )
				{
					--dirPathEnd;
				}
				if( strncmp(DefaultDir, path, dirPathEnd - path) != 0 )
				{
					std::string dir(path, dirPathEnd);
					preporcessor.addSreachDir(dir.c_str());
				}

				try
				{
					preporcessor.translate(sourceInput);
				}
				catch( std::exception& e )
				{
					e.what();
					return false;
				}

				if( compileInfo )
				{
					preporcessor.getIncludeFiles(compileInfo->includeFiles);
				}
#if 1
				codeBuffer.assign(std::istreambuf_iterator< char >(oss), std::istreambuf_iterator< char >());
#else
				std::string code = oss.str();
				codeBuffer.assign(code.begin(), code.end());
#endif
				codeBuffer.push_back(0);
			}
			else
			{
				if( def )
				{
					int lenDef = strlen(def);
					codeBuffer.resize(lenDef);
					codeBuffer.assign(def, def + lenDef);
				}

				if( !FileUtility::LoadToBuffer(path, codeBuffer, true, true) )
				{
					LogWarning(0, "Can't load shader file %s", path);
					return false;
				}
			}

			TComPtr< ID3D10Blob > errorCode;
			TComPtr< ID3D10Blob > byteCode;
			FixString<32> profileName = FD3D11Utility::GetShaderProfile( mDevice , type);

			uint32 compileFlag = 0 /*| D3D10_SHADER_PACK_MATRIX_ROW_MAJOR*/;
			VERIFY_D3D11RESULT(
				D3DX11CompileFromMemory(codeBuffer.data(), codeBuffer.size(), "ShaderCode", NULL, NULL, compileInfo->entryName.c_str(), 
										profileName, compileFlag, 0, NULL, &byteCode, &errorCode, NULL),
				{
					bSuccess = false;
				}
			);

			if( !bSuccess )
			{
				if( bUsePreprocess )
				{
					std::ofstream of("temp" SHADER_FILE_SUBNAME, std::ios::binary);
					if( of.is_open() )
					{
						of.write(&codeBuffer[0], codeBuffer.size());
					}
				}

				if( bRecompile )
				{
					::MessageBoxA(NULL, (LPCSTR) errorCode->GetBufferPointer(), "Shader Compile Error", 0);
				}

			}

			auto& shaderImpl = static_cast<D3D11Shader&>(shader);

			if( !shaderImpl.initialize(type, mDevice, byteCode) )
			{
				LogWarning(0, "Can't create shader resource");
				return false;
			}

		} while( !bSuccess && bRecompile );

		return bSuccess;
	}

	bool D3D11Shader::initialize(Shader::Type type, TComPtr<ID3D11Device>& device, TComPtr<ID3D10Blob>& inByteCode)
	{
		switch( type )
		{
#define CASE_SHADER(  TYPE , FUN ,VAR )\
				case TYPE:\
					VERIFY_D3D11RESULT_RETURN_FALSE( device->FUN(inByteCode->GetBufferPointer(), inByteCode->GetBufferSize(), NULL, &VAR) );\
					break;

			CASE_SHADER(Shader::eVertex, CreateVertexShader, mResource.vertex);
			CASE_SHADER(Shader::ePixel, CreatePixelShader, mResource.pixel);
			CASE_SHADER(Shader::eGeometry, CreateGeometryShader, mResource.geometry);
			CASE_SHADER(Shader::eCompute, CreateComputeShader, mResource.compute);
			CASE_SHADER(Shader::eHull, CreateHullShader, mResource.hull);
			CASE_SHADER(Shader::eDomain, CreateDomainShader, mResource.domain);

#undef CASE_SHADER
		default:
			assert(0);
		}
		byteCode.assign((uint8*)inByteCode->GetBufferPointer() , (uint8*)(inByteCode->GetBufferPointer()) + inByteCode->GetBufferSize());
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

}