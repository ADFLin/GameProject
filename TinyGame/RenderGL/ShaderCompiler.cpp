#include "ShaderCompiler.h"

#include "FileSystem.h"

#include "CPreprocessor.h"
#include "BitUtility.h"

#include <fstream>
#include <sstream>
#include <iterator>

namespace RenderGL
{

	char const* ShaderPosfixNames[] =
	{
		"VS" SHADER_FILE_SUBNAME ,
		"PS" SHADER_FILE_SUBNAME ,
		"GS" SHADER_FILE_SUBNAME ,
		"CS" SHADER_FILE_SUBNAME ,
	};

	char const* shaderDefines[] =
	{
		"#define VERTEX_SHADER 1\n" ,
		"#define PIXEL_SHADER 1\n" ,
		"#define GEOMETRY_SHADER 1\n" ,
		"#define COMPUTE_SHADER 1\n" ,
	};


	ShaderManager::~ShaderManager()
	{
		for( auto& pair : mShaderCompileMap )
		{
			delete pair.second;
		}
	}

	bool ShaderManager::loadFileSimple(ShaderProgram& shaderProgram, char const* fileName, char const* def , char const* additionalCode )
	{
		return loadFile(shaderProgram , fileName, SHADER_ENTRY(MainVS), SHADER_ENTRY(MainPS), def, additionalCode);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, char const* vertexEntryName, char const* pixelEntryName, char const* def, char const* additionalCode)
	{
		char const* entryNames[] = { vertexEntryName , pixelEntryName };
		uint8 shaderMask = BIT(RHIShader::eVertex) | BIT(RHIShader::ePixel);
		return loadFile(shaderProgram, fileName, shaderMask, entryNames, def, additionalCode);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], char const* def, char const* additionalCode)
	{
		return loadInternal(shaderProgram, fileName, shaderMask, entryNames, def, additionalCode, true);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], ShaderCompileOption const& option, char const* additionalCode /*= nullptr*/)
	{
		return loadInternal(shaderProgram, fileName, shaderMask, entryNames, option, additionalCode, true);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, char const* vertexEntryName, char const* pixelEntryName, ShaderCompileOption const& option, char const* additionalCode /*= nullptr*/)
	{
		char const* entryNames[] = { vertexEntryName , pixelEntryName };
		uint8 shaderMask = BIT(RHIShader::eVertex) | BIT(RHIShader::ePixel);
		return loadFile(shaderProgram, fileName, shaderMask, entryNames, option, additionalCode);
	}

	bool ShaderManager::loadInternal(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], ShaderCompileOption const& option, char const* additionalCode, bool bSingle)
	{
		ShaderCompileInfo* info = new ShaderCompileInfo;

		info->shaderProgram = &shaderProgram;
		info->shaderMask = shaderMask;
		info->bSingle = bSingle;
		info->fileName = fileName;

		for( int i = 0; i < RHIShader::NUM_SHADER_TYPE; ++i )
		{
			if( (info->shaderMask & BIT(i)) == 0 )
				continue;

			std::string defCode;
			defCode += shaderDefines[i];
			if( entryNames )
			{
				defCode += "#define ";
				defCode += entryNames[i];
				defCode += " main\n";
			}

			std::string headCode = option.getCode(defCode.c_str(), additionalCode);
			info->headCodes.push_back(std::move(headCode));
		}

		if( !updateShaderInternal(shaderProgram, *info) )
		{
			delete info;
			return false;
		}

		mShaderCompileMap.insert({ &shaderProgram , info });
		return true;
	}

	bool ShaderManager::loadInternal(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], char const* def , char const* additionalCode, bool bSingle )
	{
		ShaderCompileInfo* info = new ShaderCompileInfo;

		info->shaderProgram = &shaderProgram;
		info->shaderMask = shaderMask;
		info->bSingle = bSingle;
		info->fileName = fileName;

		for( int i = 0; i < RHIShader::NUM_SHADER_TYPE; ++i )
		{
			if( (info->shaderMask & BIT(i)) == 0 )
				continue;

			std::string headCode;
			if( def )
			{
				headCode += def;
				headCode += '\n';
			}

			headCode += shaderDefines[i];
			if( entryNames )
			{
				headCode += "#define ";
				headCode += entryNames[i];
				headCode += " main\n";
			}

			if( additionalCode )
			{
				headCode += additionalCode;
				headCode += '\n';
			}

			info->headCodes.push_back(std::move(headCode));
		}

		if( !updateShaderInternal(shaderProgram, *info) )
		{
			delete info;
			return false;
		}

		mShaderCompileMap.insert({ &shaderProgram , info });
		return true;
	}

	bool ShaderManager::loadMultiFile(ShaderProgram& shaderProgram, char const* fileName, char const* def, char const* additionalCode)
	{
		return loadInternal(shaderProgram, fileName, BIT(RHIShader::eVertex) | BIT(RHIShader::ePixel), nullptr, def, additionalCode, false);
	}

	bool ShaderManager::reloadShader(ShaderProgram& shaderProgram)
	{
		auto iter = mShaderCompileMap.find(&shaderProgram);
		if( iter == mShaderCompileMap.end() )
			return false;

		return updateShaderInternal(shaderProgram, *iter->second);
	}


	void ShaderManager::reloadAll()
	{
		for( auto pair : mShaderCompileMap )
		{
			updateShaderInternal(*pair.first, *pair.second);
		}
	}

	bool ShaderManager::updateShaderInternal(ShaderProgram& shaderProgram, ShaderCompileInfo& info)
	{
		if( !shaderProgram.isVaildate() )
		{
			if( !shaderProgram.create() )
				return false;
		}

		for( int i = 0; i < RHIShader::NUM_SHADER_TYPE; ++i )
		{
			if ( ( info.shaderMask & BIT(i) ) == 0 )
				continue;

			FixString< 256 > path;
			if( info.bSingle )
			{
				path.format("%s%s", info.fileName.c_str(), SHADER_FILE_SUBNAME );
			}
			else
			{
				path.format("%s%s", info.fileName.c_str(), ShaderPosfixNames[i]);
			}
			
			RHIShaderRef shader = shaderProgram.mShaders[i];
			if( shader == nullptr )
			{
				shader = new RHIShader;
				if( !mCompiler.compileSource(RHIShader::Type(i), *shader, path, info.headCodes[i].c_str()) )
					return false;
				shaderProgram.attachShader(*shader);
			}
			else
			{
				if( !mCompiler.compileSource(RHIShader::Type(i), *shader, path, info.headCodes[i].c_str()) )
					return false;
			}
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

				char const* DefaultDir = "Shader";
				preporcessor.setOutput(codeOutput);
				preporcessor.addSreachDir(DefaultDir);
				char const* dirPathEnd = FileUtility::getDirPathPos(path);
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
				{
					std::ofstream of("temp" SHADER_FILE_SUBNAME, std::ios::binary);
					if( of.is_open() )
					{
						of.write(&codeBuffer[0], codeBuffer.size());
					}
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



	void ShaderManager::ShaderCompileInfo::getDependentFilePaths(std::vector<std::wstring>& paths)
	{
		FixString< 256 > path;
		if( bSingle )
		{
			path.format("%s%s", fileName.c_str(), SHADER_FILE_SUBNAME);
			paths.push_back(CharToWChar(path));
		}
		else
		{
			for( int i = 0; i < RHIShader::NUM_SHADER_TYPE; ++i )
			{
				if( ( shaderMask & BIT(i)) == 0 )
					continue;
				path.format("%s%s", fileName.c_str(), ShaderPosfixNames[i]);
				paths.push_back(CharToWChar(path));
			}
		}
	}

	void ShaderManager::ShaderCompileInfo::postFileModify(FileAction action)
	{
		if ( action == FileAction::Modify )
			ShaderManager::getInstance().updateShaderInternal(*shaderProgram, *this);
	}

}//namespace GL

