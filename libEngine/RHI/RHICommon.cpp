#include "RHICommon.h"

//#TODO : remove
#include "Material.h"

#include "DrawUtility.h"
#include "ShaderCompiler.h"
#include "VertexFactory.h"

#include "OpenGLCommon.h"
#include "RHICommand.h"

namespace Render
{
	DeviceVendorName gRHIDeviceVendorName = DeviceVendorName::Unknown;

	InputLayoutDesc::InputLayoutDesc()
	{
		std::fill_n(mVertexSizes, MAX_INPUT_STREAM_NUM, 0);
	}

	static Vertex::Semantic AttributeToSemantic(uint8 attribute, uint8& idx)
	{
		switch( attribute )
		{
		case Vertex::ATTRIBUTE_POSITION: idx = 0; return Vertex::ePosition;
		case Vertex::ATTRIBUTE_COLOR: idx = 0; return Vertex::eColor;
		case Vertex::ATTRIBUTE_COLOR2: idx = 1; return Vertex::eColor;
		case Vertex::ATTRIBUTE_NORMAL: idx = 0; return Vertex::eNormal;
		case Vertex::ATTRIBUTE_TANGENT: idx = 0; return Vertex::eTangent;
		}

		idx = attribute - Vertex::ATTRIBUTE_TEXCOORD;
		return Vertex::eTexcoord;
	}

	static uint8 SemanticToAttribute(Vertex::Semantic s, uint8 idx)
	{
		switch( s )
		{
		case Vertex::ePosition: return Vertex::ATTRIBUTE_POSITION;
		case Vertex::eColor:    assert(idx < 2); return Vertex::ATTRIBUTE_COLOR + idx;
		case Vertex::eNormal:   return Vertex::ATTRIBUTE_NORMAL;
		case Vertex::eTangent:  return Vertex::ATTRIBUTE_TANGENT;
		case Vertex::eTexcoord: assert(idx < 7); return Vertex::ATTRIBUTE_TEXCOORD + idx;
		}
		return 0;
	}

	InputLayoutDesc& InputLayoutDesc::addElement(Vertex::Semantic s, Vertex::Format f, uint8 idx /*= 0 */)
	{
		InputElement element;
		element.idxStream = 0;
		element.attribute = SemanticToAttribute(s, idx);
		element.format = f;
		element.offset = mVertexSizes[element.idxStream];
		element.semantic = s;
		element.idx = idx;
		element.bNormalize = false;
		mElements.push_back(element);
		mVertexSizes[element.idxStream] += Vertex::GetFormatSize(f);
		return *this;
	}

	InputLayoutDesc& InputLayoutDesc::addElement(uint8 attribute, Vertex::Format f, bool bNormailze)
	{
		InputElement element;
		element.idxStream = 0;
		element.attribute = attribute;
		element.format = f;
		element.offset = mVertexSizes[element.idxStream];
		element.semantic = AttributeToSemantic(attribute, element.idx);
		element.bNormalize = bNormailze;
		mElements.push_back(element);
		mVertexSizes[element.idxStream] += Vertex::GetFormatSize(f);
		return *this;
	}

	InputLayoutDesc& InputLayoutDesc::addElement(uint8 idxStream, Vertex::Semantic s, Vertex::Format f, uint8 idx /*= 0 */)
	{
		InputElement element;
		element.idxStream = idxStream;
		element.attribute = SemanticToAttribute(s, idx);
		element.format = f;
		element.offset = mVertexSizes[element.idxStream];
		element.semantic = s;
		element.idx = idx;
		element.bNormalize = false;
		mElements.push_back(element);
		mVertexSizes[element.idxStream] += Vertex::GetFormatSize(f);
		return *this;
	}

	InputLayoutDesc& InputLayoutDesc::addElement(uint8 idxStream, uint8 attribute, Vertex::Format f, bool bNormailze)
	{
		InputElement element;
		element.idxStream = idxStream;
		element.attribute = attribute;
		element.format = f;
		element.offset = mVertexSizes[element.idxStream];
		element.semantic = AttributeToSemantic(attribute, element.idx);
		element.bNormalize = bNormailze;
		mElements.push_back(element);
		mVertexSizes[element.idxStream] += Vertex::GetFormatSize(f);
		return *this;
	}


	InputElement const* InputLayoutDesc::findBySematic(Vertex::Semantic s, int idx) const
	{
		for( auto const& decl : mElements )
		{
			if( decl.semantic == s && decl.idx == idx )
				return &decl;
		}
		return nullptr;
	}

	InputElement const* InputLayoutDesc::findBySematic(Vertex::Semantic s) const
	{
		for( auto const& decl : mElements )
		{
			if( decl.semantic == s )
				return &decl;
		}
		return nullptr;
	}

	void InputLayoutDesc::updateVertexSize()
	{
		std::fill_n(mVertexSizes, MAX_INPUT_STREAM_NUM, 0);
		for( auto const& e : mElements )
		{
			mVertexSizes[e.idxStream] += Vertex::GetFormatSize(e.format);
		}
	}

	int InputLayoutDesc::getSematicOffset(Vertex::Semantic s) const
	{
		InputElement const* info = findBySematic(s);
		return (info) ? info->offset : -1;
	}

	int InputLayoutDesc::getSematicOffset(Vertex::Semantic s, int idx) const
	{
		InputElement const* info = findBySematic(s, idx);
		return (info) ? info->offset : -1;
	}

	Vertex::Format InputLayoutDesc::getSematicFormat(Vertex::Semantic s) const
	{
		InputElement const* info = findBySematic(s);
		return (info) ? Vertex::Format(info->format) : Vertex::eUnknowFormat;
	}

	Vertex::Format InputLayoutDesc::getSematicFormat(Vertex::Semantic s, int idx) const
	{
		InputElement const* info = findBySematic(s, idx);
		return (info) ? Vertex::Format(info->format) : Vertex::eUnknowFormat;
	}

	RHITexture2DRef GDefaultMaterialTexture2D;
	RHITexture2DRef GWhiteTexture2D;
	RHITexture2DRef GBlackTexture2D;
	RHITextureCubeRef GWhiteTextureCube;
	RHITextureCubeRef GBlackTextureCube;

	MaterialMaster* GDefalutMaterial = nullptr;

	MaterialShaderProgram GSimpleBasePass;

	bool InitGlobalRHIResource()
	{
		uint32 colorW[4] = { 0xffffffff , 0xffffffff , 0xffffffff , 0xffffffff };
		uint32 colorB[4] = { 0, 0 , 0 , 0 };

		GWhiteTexture2D = RHICreateTexture2D(Texture::eRGBA8, 2, 2, 0, BCF_DefalutValue , colorW);
		if( !GWhiteTexture2D.isValid() )
			return false;
		GBlackTexture2D = RHICreateTexture2D(Texture::eRGBA8, 2, 2, 0, BCF_DefalutValue, colorB);
		if( !GBlackTexture2D.isValid() )
			return false;
		GWhiteTextureCube = RHICreateTextureCube();
		if( !GWhiteTextureCube->create(Texture::eRGBA8, 2, 2, colorW) )
			return false;
		GBlackTextureCube = RHICreateTextureCube();
		if( !GBlackTextureCube->create(Texture::eRGBA8, 2, 2, colorB) )
			return false;

		OpenGLCast::To(GWhiteTextureCube)->bind();
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		OpenGLCast::To(GWhiteTextureCube)->unbind();

		OpenGLCast::To(GWhiteTexture2D)->bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		OpenGLCast::To(GWhiteTexture2D)->unbind();

		GDefalutMaterial = new MaterialMaster;
		if( !GDefalutMaterial->loadFile("EmptyMaterial") )
			return false;

		GDefaultMaterialTexture2D = RHIUtility::LoadTexture2DFromFile("Texture/Gird.png");
		if( !GDefaultMaterialTexture2D.isValid() )
			return false;

		ShaderCompileOption option;
		option.version = 430;
		VertexFactoryType::DefaultType->getCompileOption(option);

		if( !ShaderManager::Get().loadFile( 
			GSimpleBasePass ,
			"Shader/SimpleBasePass",
			SHADER_ENTRY(BassPassVS), SHADER_ENTRY(BasePassPS), option , nullptr) )
			return false;

		return true;
	}

	void ReleaseGlobalRHIResource()
	{
		GDefaultMaterialTexture2D.release();
		GWhiteTexture2D.release();
		GBlackTexture2D.release();
		GWhiteTextureCube.release();
		GBlackTextureCube.release();

		GDefalutMaterial->releaseRHI();

		delete GDefalutMaterial;
	}

	class CopyTextureProgram : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(CopyTextureProgram)

		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/CopyTexture";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(CopyTexturePS) },
			};
			return entries;
		}
	public:
		void bindParameters(ShaderParameterMap& parameterMap)
		{
			parameterMap.bind( mParamCopyTexture , SHADER_PARAM(CopyTexture));
		}

		void setParameters(RHITexture2D& copyTexture)
		{
			setTexture(mParamCopyTexture, copyTexture);
		}

		ShaderParameter mParamCopyTexture;

	};


	class CopyTextureMaskProgram : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(CopyTextureMaskProgram)
		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/CopyTexture";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(CopyTextureMaskPS) },
			};
			return entries;
		}
	public:
		void bindParameters(ShaderParameterMap& parameterMap)
		{
			parameterMap.bind(mParamCopyTexture, SHADER_PARAM(CopyTexture));
			parameterMap.bind(mParamColorMask, SHADER_PARAM(ColorMask));
		}
		void setParameters(RHITexture2D& copyTexture, Vector4 const& colorMask)
		{
			setTexture(mParamCopyTexture, copyTexture);
			setParam(mParamColorMask, colorMask);
		}

		ShaderParameter mParamColorMask;
		ShaderParameter mParamCopyTexture;
	};


	class CopyTextureBiasProgram : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(CopyTextureBiasProgram)

		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/CopyTexture";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(CopyTextureBaisPS) },
			};
			return entries;
		}
	public:


		void bindParameters(ShaderParameterMap& parameterMap)
		{
			parameterMap.bind(mParamCopyTexture, SHADER_PARAM(CopyTexture));
			parameterMap.bind(mParamColorBais, SHADER_PARAM(ColorBais));
		}
		void setParameters(RHITexture2D& copyTexture, float colorBais[2])
		{
			setTexture(mParamCopyTexture, copyTexture);
			setParam(mParamColorBais, Vector2( colorBais[0], colorBais[1] ));
		}

		ShaderParameter mParamColorBais;
		ShaderParameter mParamCopyTexture;

	};

	class MappingTextureColorProgram : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(MappingTextureColorProgram)

		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/CopyTexture";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(MappingTextureColorPS) },
			};
			return entries;
		}
	public:
		void bindParameters(ShaderParameterMap& parameterMap)
		{
			parameterMap.bind(mParamCopyTexture, SHADER_PARAM(CopyTexture));
			parameterMap.bind(mParamColorMask, SHADER_PARAM(ColorMask));
			parameterMap.bind(mParamValueFactor, SHADER_PARAM(ValueFactor));
		}

		void setParameters(RHITexture2D& copyTexture, Vector4 const& colorMask, float valueFactor[2])
		{
			setTexture(mParamCopyTexture, copyTexture);
			setParam(mParamColorMask, colorMask);
			setParam(mParamValueFactor, Vector2( valueFactor[0], valueFactor[1] ));
		}

		ShaderParameter mParamCopyTexture;
		ShaderParameter mParamColorMask;
		ShaderParameter mParamValueFactor;
	};

	IMPLEMENT_GLOBAL_SHADER(CopyTextureProgram);
	IMPLEMENT_GLOBAL_SHADER(CopyTextureMaskProgram);
	IMPLEMENT_GLOBAL_SHADER(CopyTextureBiasProgram);
	IMPLEMENT_GLOBAL_SHADER(MappingTextureColorProgram);

	bool ShaderHelper::init()
	{
		ShaderCompileOption option;
		option.version = 430;

		mProgCopyTexture = ShaderManager::Get().getGlobalShaderT<CopyTextureProgram>(true);
		if( mProgCopyTexture == nullptr )
			return false;
		mProgCopyTextureMask = ShaderManager::Get().getGlobalShaderT<CopyTextureMaskProgram>(true);
		if( mProgCopyTextureMask == nullptr )
			return false;
		mProgCopyTextureBias = ShaderManager::Get().getGlobalShaderT<CopyTextureBiasProgram>(true);
		if( mProgCopyTextureBias == nullptr )
			return false;
		mProgMappingTextureColor = ShaderManager::Get().getGlobalShaderT<MappingTextureColorProgram>(true);
		if( mProgMappingTextureColor == nullptr )
			return false;

		if( !mFrameBuffer.create() )
			return false;

		mFrameBuffer.addTexture(*GWhiteTexture2D);
		return true;
	}

	void ShaderHelper::clearBuffer(RHITexture2D& texture, float clearValue[])
	{
		mFrameBuffer.setTexture(0, texture);
		GL_BIND_LOCK_OBJECT(mFrameBuffer);
		glClearBufferfv(GL_COLOR, 0, (float const*)clearValue);
	}

	void ShaderHelper::clearBuffer(RHITexture2D& texture, uint32 clearValue[])
	{
		mFrameBuffer.setTexture(0, texture);
		GL_BIND_LOCK_OBJECT(mFrameBuffer);
		glClearBufferuiv(GL_COLOR, 0, clearValue);
	}

	void ShaderHelper::clearBuffer(RHITexture2D& texture, int32 clearValue[])
	{
		mFrameBuffer.setTexture(0, texture);
		GL_BIND_LOCK_OBJECT( mFrameBuffer );
		glClearBufferiv(GL_COLOR, 0, clearValue);
	}


	void ShaderHelper::copyTextureToBuffer(RHITexture2D& copyTexture)
	{
		GL_BIND_LOCK_OBJECT(mProgCopyTexture);
		mProgCopyTexture->setParameters(copyTexture);
		DrawUtility::ScreenRectShader();
	}

	void ShaderHelper::copyTextureMaskToBuffer(RHITexture2D& copyTexture, Vector4 const& colorMask)
	{
		GL_BIND_LOCK_OBJECT(mProgCopyTextureMask);
		mProgCopyTextureMask->setParameters( copyTexture , colorMask );
		DrawUtility::ScreenRectShader();
	}

	void ShaderHelper::copyTextureBiasToBuffer(RHITexture2D& copyTexture, float colorBais[2])
	{
		GL_BIND_LOCK_OBJECT(mProgCopyTextureBias);
		mProgCopyTextureBias->setParameters(copyTexture, colorBais);
		DrawUtility::ScreenRectShader();
	}

	void ShaderHelper::mapTextureColorToBuffer(RHITexture2D& copyTexture, Vector4 const& colorMask, float valueFactor[2])
	{
		GL_BIND_LOCK_OBJECT(mProgMappingTextureColor);
		mProgMappingTextureColor->setParameters(copyTexture, colorMask, valueFactor);
		DrawUtility::ScreenRectShader();
	}

	void ShaderHelper::copyTexture(RHITexture2D& destTexture, RHITexture2D& srcTexture)
	{
		mFrameBuffer.setTexture(0, destTexture);
		mFrameBuffer.bind();
		copyTextureToBuffer(srcTexture);
		mFrameBuffer.unbind();
	}

	void ShaderHelper::reload()
	{
		ShaderManager::Get().reloadShader(*mProgCopyTexture);
		ShaderManager::Get().reloadShader(*mProgCopyTextureMask);
		ShaderManager::Get().reloadShader(*mProgMappingTextureColor);
		ShaderManager::Get().reloadShader(*mProgCopyTextureBias);
	}




}//namespace Render
