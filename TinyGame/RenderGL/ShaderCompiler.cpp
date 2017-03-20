#include "ShaderCompiler.h"

#include "FileSystem.h"

#include "CPreprocessor.h"
#include <fstream>
#include <sstream>
#include <iterator>

namespace GL
{

	ShaderManager::~ShaderManager()
	{
		for( auto& pair : mShaderCompileMap )
		{
			delete pair.second;
		}
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, char const* def /*= nullptr*/)
	{
		return loadFile(shaderProgram , fileName, SHADER_ENTRY(MainVS), SHADER_ENTRY(MainPS), def);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, char const* vertexEntryName, char const* pixelEntryName, char const* def /*= nullptr */, int version /*= -1 */)
	{
		ShaderCompileInfo* info = new ShaderCompileInfo;

		info->bSingle = true;
		info->fileName = fileName;

		if( version != -1 )
		{
			FixString< 128 > versionStr;
			versionStr.format("#version %d\n", version);
			info->defines[0] = versionStr;
			info->defines[1] = versionStr;
		}


		info->defines[0] += "#define ";
		info->defines[0] += vertexEntryName;
		info->defines[0] += +" main\n";
		info->defines[0] += "#define VERTEX_SHADER\n";

		info->defines[1] += "#define ";
		info->defines[1] += pixelEntryName;
		info->defines[1] += +" main\n";
		info->defines[1] += "#define PIXEL_SHADER\n";

		if( def )
		{
			info->defines[0] += def;
			info->defines[0] += '\n';
			info->defines[1] += def;
			info->defines[1] += '\n';
		}
		if( !updateShaderInternal(shaderProgram, *info) )
		{
			delete info;
			return false;
		}

		mShaderCompileMap.insert({ &shaderProgram , info });
		return true;
	}

	bool ShaderManager::loadMultiFile(ShaderProgram& shaderProgram, char const* fileName , char const* def, int version )
	{
		ShaderCompileInfo* info = new ShaderCompileInfo;

		info->bSingle = false;
		info->fileName = fileName;

		if( version != -1 )
		{
			FixString< 128 > versionStr;
			versionStr.format("#version %d\n", version);
			info->defines[0] = versionStr;
			info->defines[1] = versionStr;
		}

		if( def )
		{
			info->defines[0] += def;
			info->defines[0] += '\n';
			info->defines[1] += def;
			info->defines[1] += '\n';
		}
		if( !updateShaderMultiInternal(shaderProgram, *info) )
		{
			delete info;
			return false;
		}

		mShaderCompileMap.insert({ &shaderProgram , info });
		return true;
	}

	bool ShaderManager::reloadShader(ShaderProgram& shaderProgram)
	{
		auto iter = mShaderCompileMap.find(&shaderProgram);
		if( iter == mShaderCompileMap.end() )
			return false;

		if ( iter->second->bSingle )
			return updateShaderInternal(shaderProgram, *iter->second);

		return updateShaderMultiInternal(shaderProgram, *iter->second);
	}

	bool ShaderManager::updateShaderInternal(ShaderProgram& shaderProgram, ShaderCompileInfo& info)
	{
		if( !shaderProgram.isVaildate() )
		{
			if( !shaderProgram.create() )
				return false;
		}

		FixString< 256 > path;
		path.format("%s%s", info.fileName.c_str(), ".glsl");
		RHIShaderRef vertexShader = shaderProgram.mShaders[RHIShader::eVertex];
		if( vertexShader == nullptr )
		{
			vertexShader = new RHIShader;
			if( !mCompiler.compileSource( RHIShader::eVertex , *vertexShader , path, info.defines[0].c_str() ) )
				return false;
			shaderProgram.attachShader(*vertexShader);
		}
		else
		{
			if( !mCompiler.compileSource( RHIShader::eVertex ,  *vertexShader ,  path, info.defines[0].c_str()) )
				return false;
		}

		RHIShaderRef pixelShader = shaderProgram.mShaders[RHIShader::ePixel];
		if( pixelShader == nullptr )
		{
			pixelShader = new RHIShader;
			if( !mCompiler.compileSource(RHIShader::ePixel,  *pixelShader ,  path, info.defines[1].c_str()) )
				return false;
			shaderProgram.attachShader(*pixelShader);
		}
		else
		{
			if( !mCompiler.compileSource( RHIShader::ePixel, *pixelShader , path, info.defines[1].c_str()) )
				return false;
		}
		shaderProgram.updateShader(true);
		return true;
	}

	bool ShaderManager::updateShaderMultiInternal(ShaderProgram& shaderProgram, ShaderCompileInfo& info)
	{
		if( !shaderProgram.isVaildate() )
		{
			if( !shaderProgram.create() )
				return false;
		}

		FixString< 256 > path;
		path.format("%s%s", info.fileName.c_str(), "VS.glsl");
		RHIShaderRef vertexShader = shaderProgram.mShaders[RHIShader::eVertex];
		if( vertexShader == nullptr )
		{
			vertexShader = new RHIShader;
			if( !mCompiler.compileSource(RHIShader::eVertex, *vertexShader, path, info.defines[0].c_str()) )
				return false;
			shaderProgram.attachShader(*vertexShader);
		}
		else
		{
			if( !mCompiler.compileSource(RHIShader::eVertex, *vertexShader , path, info.defines[0].c_str()) )
				return false;
		}

		path.format("%s%s", info.fileName.c_str(), "FS.glsl");
		RHIShaderRef pixelShader = shaderProgram.mShaders[RHIShader::ePixel];
		if( pixelShader == nullptr )
		{
			pixelShader = new RHIShader;
			if( !mCompiler.compileSource(RHIShader::ePixel, *pixelShader, path, info.defines[1].c_str()) )
				return false;
			shaderProgram.attachShader(*pixelShader);
		}
		else
		{
			if( !mCompiler.compileSource(RHIShader::ePixel, *pixelShader , path, info.defines[1].c_str()) )
				return false;
		}

		shaderProgram.updateShader(true);
		return true;
	}

	bool ShaderCompiler::compileSource(RHIShader::Type type , RHIShader& shader, char const* path, char const* def)
	{
		bool bSuccess;
		do
		{
			bSuccess = false;
			std::vector< char > codeBuffer;
			if( !FileUtility::LoadToBuffer(path, codeBuffer) )
				return false;

			int numSourceCodes = 0;
			char const* sourceCodes[2];

			if( bUsePreprocess )
			{
				CPP::CodeInput sourceInput;

				if( def )
				{
					sourceInput.appendString(def);
				}
				sourceInput.appendString(&codeBuffer[0]);

				sourceInput.reset();

				CPP::Preprocessor preporcessor;

				std::stringstream oss;
				CPP::CodeOutput codeOutput(oss);
				preporcessor.setOutput(codeOutput);
				preporcessor.addSreachDir("Shader/");
				try
				{
					preporcessor.translate(sourceInput);
				}
				catch( std::exception& e )
				{
					e.what();
					return false;
				}
#if 1
				codeBuffer.assign(std::istreambuf_iterator< char >(oss), std::istreambuf_iterator< char >());
#else
				std::string code = oss.str();
				codeBuffer.assign(code.begin(), code.end());
#endif
				codeBuffer.push_back(0);
				sourceCodes[numSourceCodes] = &codeBuffer[0];
				++numSourceCodes;
			}
			else
			{
				if( def )
				{
					sourceCodes[numSourceCodes] = def;
					++numSourceCodes;
				}
				sourceCodes[numSourceCodes] = &codeBuffer[0];
				++numSourceCodes;
			}

			bSuccess = shader.compileSource(type, sourceCodes, numSourceCodes);

			if( !bSuccess && bUsePreprocess )
			{
				std::ofstream of("temp.glsl", std::ios::binary);
				if( of.is_open() )
				{
					of.write(&codeBuffer[0], codeBuffer.size());
				}

				int maxLength;
				glGetShaderiv(shader.mHandle, GL_INFO_LOG_LENGTH, &maxLength);
				std::vector< char > buf(maxLength);
				int logLength = 0;
				glGetShaderInfoLog(shader.mHandle, maxLength, &logLength, &buf[0]);
				::MessageBoxA(NULL, &buf[0], "Shader Compile Error", 0);
			}

		} 
		while( !bSuccess && bRecompile );

		return bSuccess;
	}

	std::string ShaderCompiler::getConfigDefine()
	{
		std::string reslut;
		for( int i = 0; i < mConfigVars.size(); ++i )
		{
			reslut += "#define";
			reslut += mConfigVars[i].name;
			reslut += mConfigVars[i].value;
			reslut += "\n";
		}
		return reslut;
	}

}//namespace GL

