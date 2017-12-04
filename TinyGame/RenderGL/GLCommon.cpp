#include "GLCommon.h"

#include "GpuProfiler.h"
#include "MarcoCommon.h"
#include "Core/IntegerType.h"
#include "FileSystem.h"

#include "stb/stb_image.h"

#include <fstream>
#include <sstream>

namespace RenderGL
{
	int const GLDefalutUnpackAlignment = 4;

	static bool checkGLError()
	{
		GLenum error = glGetError();
		if( error != GL_NO_ERROR )
		{
			int i = 1;
			return false;
		}

		return true;
	}

	bool RHIShader::loadFile( Shader::Type type , char const* path , char const* def )
	{
		std::vector< char > codeBuffer;
		if( !FileUtility::LoadToBuffer(path, codeBuffer, true) )
			return false;
		int num = 0;
		char const* src[2];
		if ( def )
		{
			src[num] = def;
			++num;
		}
		src[num] = &codeBuffer[0];
		++num;

		bool result = compileCode( type , src , num );
		return result;

	}

	Shader::Type RHIShader::getType()
	{
		if ( mHandle )
		{
			switch( getParam( GL_SHADER_TYPE ) )
			{
			case GL_VERTEX_SHADER:   return Shader::eVertex;
			case GL_FRAGMENT_SHADER: return Shader::ePixel;
			case GL_GEOMETRY_SHADER: return Shader::eGeometry;
			case GL_COMPUTE_SHADER:  return Shader::eCompute;
			case GL_TESS_CONTROL_SHADER: return Shader::eHull;
			case GL_TESS_EVALUATION_SHADER: return Shader::eDomain;
			}
		}
		return Shader::eUnknown;
	}

	bool RHIShader::compileCode(Shader::Type type , char const* src[] , int num )
	{
		if ( !create( type ) )
			return false;

		glShaderSource(mHandle, num, src, 0);
		glCompileShader(mHandle);

		if( getParam(GL_COMPILE_STATUS) == GL_FALSE )
		{
			return false;
		}
		return true;
	}

	bool RHIShader::create(Shader::Type type)
	{
		if ( mHandle )
		{
			if ( getType() == type )
				return true;
			destroy();
		}

		mHandle = glCreateShader(GLConvert::To(type));
		return mHandle != 0;
	}

	void RHIShader::destroy()
	{
		if ( mHandle )
		{
			glDeleteShader( mHandle );
			mHandle = 0;
		}
	}

	GLuint RHIShader::getParam(GLuint val)
	{
		GLint status;
		glGetShaderiv( mHandle , val , &status );
		return status;
	}


	ShaderProgram::ShaderProgram()
	{
		mNeedLink = true;
		std::fill_n( mShaders , NumShader , (RHIShader*)0 );
	}

	ShaderProgram::~ShaderProgram()
	{

	}

	bool ShaderProgram::create()
	{
		return fetchHandle();
	}

	RHIShader* ShaderProgram::removeShader( Shader::Type type )
	{
		RHIShader* out = mShaders[ type ];
		if ( mShaders[ type ] )
		{
			glDetachShader( mHandle , mShaders[ type ]->mHandle );
			mShaders[ type ] = NULL;
			mNeedLink = true;
		}
		return out;
	}

	void ShaderProgram::attachShader(RHIShader& shader)
	{
		Shader::Type type = shader.getType();
		if ( mShaders[ type ] )
			glDetachShader( mHandle , mShaders[ type ]->mHandle );
		glAttachShader( mHandle , shader.mHandle );
		mShaders[ type ] = &shader;
		mNeedLink = true;
	}

	void ShaderProgram::updateShader(bool bForce)
	{
		if ( mNeedLink || bForce )
		{
			glLinkProgram( mHandle );

			GLint value;
			glGetProgramiv(mHandle, GL_LINK_STATUS, &value);
			if( value != GL_TRUE )
			{
				GLchar buffer[40960];
				GLsizei size;
				glGetProgramInfoLog(mHandle, ARRAY_SIZE(buffer), &size, buffer);
				//::Msg("Can't Link Program : %s", buffer);
				int i = 1;
			}

			glValidateProgram(mHandle);
			checkProgramStatus();
			mNeedLink = false;
		}

		bindParameters();
	}

	void ShaderProgram::checkProgramStatus()
	{
		GLint value;
		glGetProgramiv(mHandle, GL_LINK_STATUS, &value);
		if( value != GL_TRUE )
		{
			GLchar buffer[40960];
			GLsizei size;
			glGetProgramInfoLog(mHandle, ARRAY_SIZE(buffer), &size, buffer);
			//::Msg("Can't Link Program : %s", buffer);
			int i = 1;
		}

		glGetProgramiv(mHandle, GL_VALIDATE_STATUS, &value);
		if( value != GL_TRUE )
		{
			GLchar buffer[40960];
			GLsizei size;
			glGetProgramInfoLog(mHandle, ARRAY_SIZE(buffer), &size, buffer);
			//::Msg("Can't Link Program : %s", buffer);
			int i = 1;
		}
	}

	void ShaderProgram::bind()
	{
		updateShader();
		glUseProgram( mHandle );
		resetTextureAutoBindIndex();
	}

	void ShaderProgram::unbind()
	{
		glUseProgram( 0 );
	}

	Mesh::Mesh()
	{
		mType = PrimitiveType::eTriangleList;
		mVAO = 0;
	}

	Mesh::~Mesh()
	{
		if( mVAO )
		{
			glDeleteVertexArrays(1, &mVAO);
		}
	}

	bool Mesh::createBuffer(void* pVertex , int nV , void* pIdx , int nIndices , bool bIntIndex)
	{
		mVertexBuffer = new RHIVertexBuffer;
		if( !mVertexBuffer->create(mDecl.getVertexSize() , nV , pVertex ) )
			return false;

		if ( nIndices )
		{
			mIndexBuffer = new RHIIndexBuffer;
			if( !mIndexBuffer->create(nIndices, bIntIndex, pIdx) )
				return false;
		}
		return true;
	}

	void Mesh::draw(bool bUseVAO)
	{
		if( mVertexBuffer == nullptr )
			return;

		drawInternal(0, (mIndexBuffer) ? mIndexBuffer->mNumIndices : mVertexBuffer->mNumVertices, bUseVAO);
	}


	void Mesh::drawSection( int idx , bool bUseVAO )
	{
		if( mVertexBuffer == nullptr )
			return;
		Section& section = mSections[idx];
		drawInternal( section.start , section.num , bUseVAO );
	}

	void Mesh::drawInternal(int idxStart, int num, bool bUseVAO)
	{
		assert(mVertexBuffer != nullptr);

		if( bUseVAO )
		{
			bindVAO();

			if( mIndexBuffer )
			{
				glDrawElements(GLConvert::To(mType), num, mIndexBuffer->mbIntIndex ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT, (void*)idxStart);
			}
			else
			{
				glDrawArrays(GLConvert::To(mType), idxStart, num);
			}

			checkGLError();

			unbindVAO();
		}
		else
		{
			mVertexBuffer->bind();
			mDecl.bind();

			if( mIndexBuffer )
			{
				GL_BIND_LOCK_OBJECT(mIndexBuffer);
				glDrawElements(GLConvert::To(mType), num, mIndexBuffer->mbIntIndex ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT, (void*)idxStart);
			}
			else
			{
				glDrawArrays(GLConvert::To(mType), idxStart, num);
			}

			checkGLError();

			mDecl.unbind();
			mVertexBuffer->unbind();
		}
	}

	void Mesh::bindVAO()
	{
		if( mVAO == 0 )
		{
			glGenVertexArrays(1, &mVAO);
			glBindVertexArray(mVAO);

			mVertexBuffer->bind();
			mDecl.setupVAO();
			if( mIndexBuffer )
				mIndexBuffer->bind();
			glBindVertexArray(0);
			mVertexBuffer->unbind();
			if( mIndexBuffer )
				mIndexBuffer->unbind();
			mDecl.setupVAOEnd();
		}
		glBindVertexArray(mVAO);
	}

	VertexDecl::VertexDecl()
	{
		mVertexSize = 0;
	}

	void VertexDecl::bind()
	{
		bool haveTex = false;
		for( VertexElement& info : mInfoVec )
		{
			switch( info.semantic )
			{
			case Vertex::ePosition:
				glEnableClientState( GL_VERTEX_ARRAY );
				glVertexPointer( getElementSize( info.format ) , getFormatType( info.format ) , mVertexSize , (void*)info.offset );
				break;
			case Vertex::eNormal:
				assert( getElementSize( info.format ) == 3 );
				glEnableClientState( GL_NORMAL_ARRAY );
				glNormalPointer( getFormatType( info.format ) , mVertexSize , (void*)info.offset );
				break;
			case Vertex::eColor:
				if ( info.idx == 0 )
				{
					glEnableClientState( GL_COLOR_ARRAY );
					glColorPointer( getElementSize( info.format ) , getFormatType( info.format ) , mVertexSize , (void*)info.offset );
				}
				else
				{
					glEnableClientState( GL_SECONDARY_COLOR_ARRAY );
					glSecondaryColorPointer( getElementSize( info.format ) , getFormatType( info.format ) , mVertexSize , (void*)info.offset );
				}
				break;
			case Vertex::eTangent:
				glClientActiveTexture(GL_TEXTURE5);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(getElementSize(info.format), getFormatType(info.format), mVertexSize, (void*)info.offset);
				haveTex = true;
				break;
			case Vertex::eTexcoord:
				glClientActiveTexture( GL_TEXTURE0 + info.idx );
				glEnableClientState( GL_TEXTURE_COORD_ARRAY );
				glTexCoordPointer( getElementSize( info.format ) , getFormatType( info.format ) , mVertexSize , (void*)info.offset );
				haveTex = true;
				break;
			}
		}
	}

	void VertexDecl::unbind()
	{
		bool haveTex = false;
		for( VertexElement& info : mInfoVec )
		{
			switch( info.semantic )
			{
			case Vertex::ePosition:
				glDisableClientState( GL_VERTEX_ARRAY );
				break;
			case Vertex::eNormal:
				glDisableClientState( GL_NORMAL_ARRAY );
				break;
			case Vertex::eColor:
				glDisableClientState( ( info.idx == 0) ? GL_COLOR_ARRAY:GL_SECONDARY_COLOR_ARRAY );
				break;
			case Vertex::eTangent:
				haveTex = true;
				glClientActiveTexture(GL_TEXTURE0);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				break;
			case Vertex::eTexcoord:
				haveTex = true;
				glClientActiveTexture( GL_TEXTURE1 + info.idx );
				glDisableClientState( GL_TEXTURE_COORD_ARRAY );
				break;
			}
		}
		if ( haveTex )
			glClientActiveTexture( GL_TEXTURE0 );
	}

	void VertexDecl::setupVAO()
	{
		for( VertexElement& info : mInfoVec )
		{
			glEnableVertexAttribArray(info.semantic);
			glVertexAttribPointer(info.semantic, getElementSize(info.format), getFormatType(info.format), GL_FALSE, mVertexSize, (void*)info.offset);
		}
	}

	void VertexDecl::setupVAOEnd()
	{
		for( VertexElement& info : mInfoVec )
		{
			glDisableVertexAttribArray(info.attribute);
		}
	}
	static Vertex::Semantic AttributeToSemantic(uint8 attribute, uint8& idx)
	{
		switch( attribute )
		{
		case Vertex::ATTRIBUTE_POSITION: idx = 0; return Vertex::ePosition;
		case Vertex::ATTRIBUTE_COLOR: idx = 0; return Vertex::eColor;
		case Vertex::ATTRIBUTE_NORMAL: idx = 0; return Vertex::eNormal;
		case Vertex::ATTRIBUTE_TANGENT: idx = 0; return Vertex::eTangent;
		}

		idx = attribute - Vertex::ATTRIBUTE_TEXCOORD;
		return Vertex::eTexcoord;
	}

	static uint8 SemanticToAttribute(Vertex::Semantic s , uint8 idx)
	{
		return uint8(s) + idx;
	}

	VertexDecl& VertexDecl::addElement(Vertex::Semantic s, Vertex::Format f, uint8 idx /*= 0 */)
	{
		VertexElement info;
		info.attribute = SemanticToAttribute(s, idx);
		info.format   = f;
		info.offset   = mVertexSize;
		info.semantic = s;
		info.idx      = idx;
		info.bNormalize = false;
		mInfoVec.push_back( info );
		mVertexSize += getFormatSize( f );
		return *this;
	}

	VertexDecl& VertexDecl::addElement(uint8 attribute, Vertex::Format f, bool bNormailze)
	{
		VertexElement info;
		info.attribute = attribute;
		info.format = f;
		info.offset = mVertexSize;
		info.semantic = AttributeToSemantic(attribute, info.idx);
		info.bNormalize = bNormailze;
		mInfoVec.push_back(info);
		mVertexSize += getFormatSize(f);
		return *this;
	}

	uint8 VertexDecl::getFormatSize(uint8 format)
	{
		switch( format )
		{
		case Vertex::eFloat1: return 1 * sizeof( float );
		case Vertex::eFloat2: return 2 * sizeof( float );
		case Vertex::eFloat3: return 3 * sizeof( float );
		case Vertex::eFloat4: return 4 * sizeof( float );
		case Vertex::eUInt1: return 1 * sizeof( uint32 );
		case Vertex::eUInt2: return 2 * sizeof( uint32 );
		case Vertex::eUInt3: return 3 * sizeof( uint32 );
		case Vertex::eUInt4: return 4 * sizeof( uint32 );
		case Vertex::eInt1: return 1 * sizeof(int32);
		case Vertex::eInt2: return 2 * sizeof(int32);
		case Vertex::eInt3: return 3 * sizeof(int32);
		case Vertex::eInt4: return 4 * sizeof(int32);
		case Vertex::eUShort1: return 1 * sizeof(uint16);
		case Vertex::eUShort2: return 2 * sizeof(uint16);
		case Vertex::eUShort3: return 3 * sizeof(uint16);
		case Vertex::eUShort4: return 4 * sizeof(uint16);
		case Vertex::eShort1: return 1 * sizeof(int16);
		case Vertex::eShort2: return 2 * sizeof(int16);
		case Vertex::eShort3: return 3 * sizeof(int16);
		case Vertex::eShort4: return 4 * sizeof(int16);
		case Vertex::eUByte1: return 1 * sizeof(uint8);
		case Vertex::eUByte2: return 2 * sizeof(uint8);
		case Vertex::eUByte3: return 3 * sizeof(uint8);
		case Vertex::eUByte4: return 4 * sizeof(uint8);
		case Vertex::eByte1: return 1 * sizeof(int8);
		case Vertex::eByte2: return 2 * sizeof(int8);
		case Vertex::eByte3: return 3 * sizeof(int8);
		case Vertex::eByte4: return 4 * sizeof(int8);
		}
		return 0;
	}

	GLenum VertexDecl::getFormatType(uint8 format)
	{
		switch( format )
		{
		case Vertex::eFloat1:case Vertex::eFloat2:
		case Vertex::eFloat3:case Vertex::eFloat4:
			return GL_FLOAT;
		case Vertex::eUInt1:case Vertex::eUInt2:
		case Vertex::eUInt3:case Vertex::eUInt4:
			return GL_UNSIGNED_INT;
		case Vertex::eInt1:case Vertex::eInt2:
		case Vertex::eInt3:case Vertex::eInt4:
			return GL_INT;
		case Vertex::eUShort1:case Vertex::eUShort2:
		case Vertex::eUShort3:case Vertex::eUShort4:
			return GL_UNSIGNED_SHORT;
		case Vertex::eShort1:case Vertex::eShort2:
		case Vertex::eShort3:case Vertex::eShort4:
			return GL_SHORT;
		case Vertex::eUByte1:case Vertex::eUByte2:
		case Vertex::eUByte3:case Vertex::eUByte4:
			return GL_UNSIGNED_BYTE;
		case Vertex::eByte1:case Vertex::eByte2:
		case Vertex::eByte3:case Vertex::eByte4:
			return GL_BYTE;
		}
		return GL_FLOAT;
	}

	int VertexDecl::getElementSize(uint8 format)
	{
		switch( format )
		{
		case Vertex::eFloat1: 
		case Vertex::eUInt1: case Vertex::eInt1: 
		case Vertex::eUShort1: case Vertex::eShort1:
		case Vertex::eUByte1: case Vertex::eByte1:
			return 1;
		case Vertex::eFloat2: 
		case Vertex::eUInt2: case Vertex::eInt2:
		case Vertex::eUShort2: case Vertex::eShort2:
		case Vertex::eUByte2: case Vertex::eByte2:
			return 2;
		case Vertex::eFloat3:
		case Vertex::eUInt3: case Vertex::eInt3:
		case Vertex::eUShort3: case Vertex::eShort3:
		case Vertex::eUByte3: case Vertex::eByte3:
			return 3;
		case Vertex::eFloat4: 
		case Vertex::eUInt4: case Vertex::eInt4:
		case Vertex::eUShort4: case Vertex::eShort4:
		case Vertex::eUByte4: case Vertex::eByte4:
			return 4;
		}
		return 0;
	}

	VertexElement const* VertexDecl::findBySematic(Vertex::Semantic s , int idx) const
	{
		for( InfoVec::const_iterator iter = mInfoVec.begin(),itEnd = mInfoVec.end() ;
			iter != itEnd ; ++iter )
		{
			VertexElement const& info = *iter;
			if ( iter->semantic == s && iter->idx == idx )
				return &info;
		}
		return NULL;
	}

	VertexElement const* VertexDecl::findBySematic(Vertex::Semantic s) const
	{
		for( InfoVec::const_iterator iter = mInfoVec.begin(),itEnd = mInfoVec.end() ;
			iter != itEnd ; ++iter )
		{
			VertexElement const& info = *iter;
			if ( iter->semantic == s )
				return &info;
		}
		return NULL;
	}

	int VertexDecl::getSematicOffset(Vertex::Semantic s) const
	{
		VertexElement const* info = findBySematic( s );
		return ( info ) ? info->offset : -1;
	}

	int VertexDecl::getSematicOffset(Vertex::Semantic s , int idx) const
	{
		VertexElement const* info = findBySematic( s ,idx );
		return ( info ) ? info->offset : -1;
	}

	Vertex::Format VertexDecl::getSematicFormat(Vertex::Semantic s) const
	{
		VertexElement const* info = findBySematic( s );
		return ( info ) ? Vertex::Format( info->format ) : Vertex::eUnknowFormat;
	}

	Vertex::Format VertexDecl::getSematicFormat(Vertex::Semantic s , int idx) const
	{
		VertexElement const* info = findBySematic( s , idx );
		return ( info ) ? Vertex::Format( info->format ) : Vertex::eUnknowFormat;
	}



	bool RHITextureBase::loadFileInternal(char const* path, GLenum texType , GLenum texImageType, Vec2i& outSize , Texture::Format& outFormat)
	{
		int w;
		int h;
		int comp;
		unsigned char* image = stbi_load( path , &w, &h, &comp, STBI_default );

		if( !image )
			return false;

		outSize.x = w;
		outSize.y = h;

		glTexParameteri( texType, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( texType, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

		//#TODO
		switch( comp )
		{
		case 3:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(texImageType, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
			glPixelStorei(GL_UNPACK_ALIGNMENT, GLDefalutUnpackAlignment);
			outFormat = Texture::eRGB8;
			break;
		case 4:
			glTexImage2D(texImageType, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
			outFormat = Texture::eRGBA8;
			break;
		}
		//glGenerateMipmap( texType);
		stbi_image_free(image);
		return true;
	}


	bool RHITexture2D::create(Texture::Format format, int width, int height, void* data , int alignment )
	{
		if( !fetchHandle() )
			return false;
		mSizeX = width;
		mSizeY = height;
		mFormat = format;

		glBindTexture(GL_TEXTURE_2D, mHandle);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if ( alignment && alignment != GLDefalutUnpackAlignment )
		{
			glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
			glTexImage2D(GL_TEXTURE_2D, 0, GLConvert::To(format), width, height, 0,
						 Texture::GetBaseFormat(format), Texture::GetComponentType(format), data);
			glPixelStorei(GL_UNPACK_ALIGNMENT, GLDefalutUnpackAlignment);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GLConvert::To(format), width, height, 0,
						 Texture::GetBaseFormat(format), Texture::GetComponentType(format), data);
		}
		checkGLError();
		glBindTexture(GL_TEXTURE_2D, 0);
		return true;
	}

	bool RHITexture2D::loadFromFile(char const* path)
	{
		if ( !fetchHandle() )
			return false;

		glBindTexture( GL_TEXTURE_2D , mHandle );
		Vec2i size;
		bool result = loadFileInternal( path , GL_TEXTURE_2D , GL_TEXTURE_2D , size , mFormat );
		if( result )
		{
			mSizeX = size.x;
			mSizeY = size.y;
		}
		glBindTexture(GL_TEXTURE_2D, 0);
		return result;
	}

	void RHITexture2D::bind()
	{
		glBindTexture( GL_TEXTURE_2D , mHandle );
	}

	void RHITexture2D::unbind()
	{
		glBindTexture( GL_TEXTURE_2D , 0 );
	}

	bool RHITexture2D::update(int ox, int oy, int w, int h, Texture::Format format , void* data , int level )
	{
		bind();
		glTexSubImage2D(GL_TEXTURE_2D, level, ox, oy, w, h, Texture::GetPixelFormat(format) , Texture::GetComponentType(format), data);
		bool result = checkGLError();
		unbind();
		return result;
	}

	bool RHITexture3D::create(Texture::Format format, int sizeX, int sizeY, int sizeZ)
	{
		if( !fetchHandle() )
			return false;
		mSizeX = sizeX;
		mSizeY = sizeY;
		mSizeZ = sizeZ;

		glBindTexture(GL_TEXTURE_3D, mHandle);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage3D(GL_TEXTURE_3D, 0, GLConvert::To(format), sizeX, sizeY , sizeZ, 0, 
					 Texture::GetBaseFormat(format), Texture::GetComponentType(format), NULL);
		glBindTexture(GL_TEXTURE_3D, 0);
		return true;
	}

	void RHITexture3D::bind()
	{
		glBindTexture(GL_TEXTURE_3D, mHandle);
	}

	void RHITexture3D::unbind()
	{
		glBindTexture(GL_TEXTURE_3D, 0);
	}


	bool RHITextureCube::create(Texture::Format format, int width, int height , void* data )
	{
		if( !fetchHandle() )
			return false;

		glBindTexture(GL_TEXTURE_CUBE_MAP, mHandle);
		for( int i = 0; i < 6; ++i )
		{
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 
						 GLConvert::To(format), width, height, 0,
						 Texture::GetBaseFormat(format), Texture::GetComponentType(format), data );
		}
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		return true;
	}

	bool RHITextureCube::loadFile(char const* path[])
	{
		if ( !fetchHandle() )
			return false;

		glBindTexture(GL_TEXTURE_CUBE_MAP, mHandle );

		bool result = true; 
		for( int i = 0 ; i < 6 ; ++i )
		{
			Vec2i size;
			Texture::Format format;
			if ( !loadFileInternal( path[i] , GL_TEXTURE_CUBE_MAP , GL_TEXTURE_CUBE_MAP_POSITIVE_X + i , size , format) )
			{
				result = false;
			}
		}

		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		return result;
	}

	void RHITextureCube::bind()
	{
		glBindTexture( GL_TEXTURE_CUBE_MAP , mHandle );
	}

	void RHITextureCube::unbind()
	{
		glBindTexture( GL_TEXTURE_CUBE_MAP , 0 );
	}

	bool RHITextureDepth::create(Texture::DepthFormat format, int width, int height)
	{
		if( !fetchHandle() )
			return false;
		
		mFromat = format;

		glBindTexture(GL_TEXTURE_2D, mHandle);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0,  Texture::Convert(format), width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		GLenum error = glGetError();
		if( error != GL_NO_ERROR )
		{
		}
		glBindTexture(GL_TEXTURE_2D, 0);
		return true;	
	}

	void RHITextureDepth::bind()
	{
		glBindTexture(GL_TEXTURE_2D, mHandle);
	}

	void RHITextureDepth::unbind()
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	FrameBuffer::FrameBuffer()
	{
		mFBO = 0;
	}

	FrameBuffer::~FrameBuffer()
	{
		if( mFBO )
		{
			glDeleteFramebuffers(1, &mFBO);
			mFBO = 0;
		}
		mTextures.clear();
	}

	int FrameBuffer::addTexture( RHITextureCube& target , Texture::Face face  )
	{
		int idx = mTextures.size();

		BufferInfo info;
		info.bufferRef = &target;
		info.idxFace   = face;
		mTextures.push_back( info );
		
		setTextureInternal( idx , target.mHandle , GL_TEXTURE_CUBE_MAP_POSITIVE_X + info.idxFace );
		return idx;
	}

	int FrameBuffer::addTexture( RHITexture2D& target )
	{
		int idx = mTextures.size();

		BufferInfo info;
		info.bufferRef = &target;
		info.idxFace  = 0;
		mTextures.push_back( info );
		setTextureInternal( idx , target.mHandle , GL_TEXTURE_2D );
		
		return idx;
	}

	void FrameBuffer::setTexture( int idx , RHITexture2D& target )
	{
		assert( idx < mTextures.size() );
		BufferInfo& info = mTextures[idx];
		info.bufferRef = &target;
		info.idxFace  = 0;
		setTextureInternal( idx , target.mHandle , GL_TEXTURE_2D );
	}

	void FrameBuffer::setTexture( int idx , RHITextureCube& target , Texture::Face face )
	{
		assert( idx < mTextures.size() );
		BufferInfo& info = mTextures[idx];
		info.bufferRef = &target;
		info.idxFace   = face;
		setTextureInternal( idx , target.mHandle , GL_TEXTURE_CUBE_MAP_POSITIVE_X + info.idxFace );
	}


	void FrameBuffer::setTextureInternal(int idx, GLuint handle , GLenum texType)
	{
		assert( mFBO );
		glBindFramebuffer( GL_FRAMEBUFFER , mFBO );
		glFramebufferTexture2D( GL_FRAMEBUFFER , GL_COLOR_ATTACHMENT0 + idx , texType , handle , 0 );
		GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if( Status != GL_FRAMEBUFFER_COMPLETE )
		{
			::Msg("Texture Can't Attach to FrameBuffer");
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0 );
	}

	void FrameBuffer::bindDepthOnly()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	}


	void FrameBuffer::bind()
	{
		glBindFramebuffer( GL_FRAMEBUFFER, mFBO );
		GLenum DrawBuffers[] =
		{ 
			GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 , GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4,
			GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7 , GL_COLOR_ATTACHMENT8, GL_COLOR_ATTACHMENT9
		};
		glDrawBuffers( mTextures.size() , DrawBuffers );


		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if( status != GL_FRAMEBUFFER_COMPLETE )
		{
			
		}
	}

	void FrameBuffer::unbind()
	{
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	}

	bool FrameBuffer::create()
	{
		glGenFramebuffers( 1, &mFBO ); 
		//glBindFramebuffer( GL_DRAW_FRAMEBUFFER , mFBO );
		return mFBO != 0;
	}

	int FrameBuffer::addTextureLayer(RHITextureCube& target)
	{
		int idx = mTextures.size();
		BufferInfo info;
		info.bufferRef = &target;
		info.idxFace = 0;
		mTextures.push_back(info);
		glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target.mHandle, 0);
		GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if( Status != GL_FRAMEBUFFER_COMPLETE )
		{
			::Msg("Texture Can't Attach to FrameBuffer");
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return idx;
	}

	void FrameBuffer::clearBuffer(Vector4 const* colorValue, float const* depthValue, uint8 stencilValue)
	{
		if( colorValue )
		{
			for( int i = 0; i < mTextures.size(); ++i )
			{
				BufferInfo& bufferInfo = mTextures[i];
				glClearBufferfv(GL_COLOR, i, (float const*)colorValue);
			}
		}
		if( depthValue )
		{
			//FIXME
			glClearBufferfv(GL_DEPTH, 0, depthValue);
		}
	}

	void FrameBuffer::setDepthInternal( RHIResource& resource , GLuint handle, Texture::DepthFormat format, bool bTexture )
	{
		removeDepthBuffer();

		mDepth.bufferRef = &resource;
		mDepth.bTexture = bTexture;

		if( handle )
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFBO);

			GLenum attachType = GL_DEPTH_ATTACHMENT;
			if( Texture::ContainStencil(format) )
			{
				if( Texture::ContainDepth(format) )
					attachType = GL_DEPTH_STENCIL_ATTACHMENT;
				else
					attachType = GL_STENCIL_ATTACHMENT;
			}

			if( bTexture )
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, attachType , GL_TEXTURE_2D, handle, 0);
			}
			else
			{
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachType, GL_RENDERBUFFER, handle);
			}


			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if( status != GL_FRAMEBUFFER_COMPLETE )
			{
				::Msg("DepthBuffer Can't Attach to FrameBuffer");
			}
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}
	}

	void FrameBuffer::setDepth( RHIDepthRenderBuffer& buffer)
	{
		setDepthInternal( buffer , buffer.mHandle , buffer.getFormat() , false );
	}

	void FrameBuffer::setDepth(RHITextureDepth& target)
	{
		setDepthInternal( target , target.mHandle, target.getFormat(), true );
	}

	void FrameBuffer::removeDepthBuffer()
	{
		if ( mDepth.bufferRef )
		{
			mDepth.bufferRef = nullptr;
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFBO);
			if( mDepth.bTexture )
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
			}
			else
			{
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
			}
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}

	}

	bool RHIDepthRenderBuffer::create(int w, int h, Texture::DepthFormat format)
	{
		assert( mHandle == 0 );
		mFormat = format;
		glGenRenderbuffers(1, &mHandle );
		glBindRenderbuffer( GL_RENDERBUFFER , mHandle );
		glRenderbufferStorage( GL_RENDERBUFFER , Texture::Convert( format ), w , h );
		//glRenderbufferStorage( GL_RENDERBUFFER , GL_DEPTH_COMPONENT , w , h );
		glBindRenderbuffer( GL_RENDERBUFFER , 0 );

		return true;
	}
	struct TextureConvInfo
	{
#if _DEBUG
		Texture::Format formatCheck;
#endif
		GLenum foramt;
		int    compNum;
		GLenum compType;
		
		//GLenum GLBaseFormat;
	};

	constexpr TextureConvInfo gTexConvMap[] =
	{
#if _DEBUG
#define TEXTURE_INFO( FORMAT_CHECK , FORMAT , COMP_NUM , COMP_TYPE )\
	{ FORMAT_CHECK , FORMAT , COMP_NUM , COMP_TYPE},
#else
#define TEXTURE_INFO( FORMAT_CHECK , FORMAT , COMP_TYPE )\
	{ FORMAT , COMP_NUM ,COMP_TYPE},
#endif
		TEXTURE_INFO(Texture::eRGBA8   ,GL_RGBA8   ,4,GL_UNSIGNED_BYTE)
		TEXTURE_INFO(Texture::eRGB8    ,GL_RGB8    ,3,GL_UNSIGNED_BYTE)

		TEXTURE_INFO(Texture::eR16     ,GL_R16     ,1,GL_UNSIGNED_SHORT)
		TEXTURE_INFO(Texture::eR8      ,GL_R8      ,1,GL_UNSIGNED_BYTE)
		
		TEXTURE_INFO(Texture::eR32F    ,GL_R32F    ,1,GL_FLOAT)
		TEXTURE_INFO(Texture::eRGB32F  ,GL_RGB32F  ,3,GL_FLOAT)
		TEXTURE_INFO(Texture::eRGBA32F ,GL_RGBA32F ,4,GL_FLOAT)
		TEXTURE_INFO(Texture::eRGB16F  ,GL_RGB16F  ,3,GL_FLOAT)
		TEXTURE_INFO(Texture::eRGBA16F ,GL_RGBA16F ,4,GL_FLOAT)

		TEXTURE_INFO(Texture::eR32I    ,GL_R32I    ,1,GL_INT)
		TEXTURE_INFO(Texture::eR16I    ,GL_R16I    ,1,GL_SHORT)
		TEXTURE_INFO(Texture::eR8I     ,GL_R8I     ,1,GL_BYTE)
		TEXTURE_INFO(Texture::eR32U    ,GL_R32UI   ,1,GL_UNSIGNED_INT)
		TEXTURE_INFO(Texture::eR16U    ,GL_R16UI   ,1,GL_UNSIGNED_SHORT)
		TEXTURE_INFO(Texture::eR8U     ,GL_R8UI    ,1,GL_UNSIGNED_BYTE)
		
		TEXTURE_INFO(Texture::eRG32I   ,GL_RG32I   ,2,GL_INT)
		TEXTURE_INFO(Texture::eRG16I   ,GL_RG16I   ,2,GL_SHORT)
		TEXTURE_INFO(Texture::eRG8I    ,GL_RG8I    ,2,GL_BYTE)
		TEXTURE_INFO(Texture::eRG32U   ,GL_RG32UI  ,2,GL_UNSIGNED_INT)
		TEXTURE_INFO(Texture::eRG16U   ,GL_RG16UI  ,2,GL_UNSIGNED_SHORT)
		TEXTURE_INFO(Texture::eRG8U    ,GL_RG8UI   ,2,GL_UNSIGNED_BYTE)
		
		TEXTURE_INFO(Texture::eRGB32I  ,GL_RGB32I  ,3,GL_INT)
		TEXTURE_INFO(Texture::eRGB16I  ,GL_RGB16I  ,3,GL_SHORT)
		TEXTURE_INFO(Texture::eRGB8I   ,GL_RGB8I   ,3,GL_BYTE)
		TEXTURE_INFO(Texture::eRGB32U  ,GL_RGB32UI ,3,GL_UNSIGNED_INT)
		TEXTURE_INFO(Texture::eRGB16U  ,GL_RGB16UI ,3,GL_UNSIGNED_SHORT)
		TEXTURE_INFO(Texture::eRGB8U   ,GL_RGB8UI  ,3,GL_UNSIGNED_BYTE)
		
		TEXTURE_INFO(Texture::eRGBA32I ,GL_RGBA32I ,4,GL_INT)
		TEXTURE_INFO(Texture::eRGBA16I ,GL_RGBA16I ,4,GL_SHORT)
		TEXTURE_INFO(Texture::eRGBA8I  ,GL_RGBA8I  ,4,GL_BYTE)
		TEXTURE_INFO(Texture::eRGBA32U ,GL_RGBA32UI,4,GL_UNSIGNED_INT)
		TEXTURE_INFO(Texture::eRGBA16U ,GL_RGBA16UI,4,GL_UNSIGNED_SHORT)
		TEXTURE_INFO(Texture::eRGBA8U  ,GL_RGBA8UI ,4,GL_UNSIGNED_BYTE)
			
		
#undef TEXTURE_INFO
	};
#if _DEBUG
	constexpr bool CheckTexConvMapValid_R(int i, int size)
	{
		return (i == size) ? true : ((i == (int)gTexConvMap[i].formatCheck) && CheckTexConvMapValid_R(i + 1, size));
	}
	constexpr bool CheckTexConvMapValid()
	{
		return CheckTexConvMapValid_R(0, sizeof(gTexConvMap) / sizeof(gTexConvMap[0]));
	}
	static_assert(CheckTexConvMapValid(), "CheckTexConvMapValid Error");
#endif

	GLenum Texture::Convert(DepthFormat format)
	{
		switch( format )
		{
		case eDepth16: return GL_DEPTH_COMPONENT16;
		case eDepth24: return GL_DEPTH_COMPONENT24;
		case eDepth32: return GL_DEPTH_COMPONENT32;
		case eDepth32F:return GL_DEPTH_COMPONENT32F;
		case eD24S8:   return GL_DEPTH24_STENCIL8;
		case eD32FS8:  return GL_DEPTH32F_STENCIL8;
		case eStencil1: return GL_STENCIL_INDEX1;
		case eStencil4: return GL_STENCIL_INDEX4;
		case eStencil8: return GL_STENCIL_INDEX8;
		case eStencil16: return GL_STENCIL_INDEX16;
		}
		return 0;
	}

	GLenum GLConvert::To(Texture::Format format)
	{
		return gTexConvMap[(int)format].foramt;
	}

	GLenum Texture::GetComponentType(Format format)
	{
		return gTexConvMap[format].compType;
	}

	GLenum Texture::GetPixelFormat(Format format)
	{
		switch( format )
		{
		case eR32F: case eR16: case eR8:
		case eR8I:case eR16I:case eR32I:
		case eR8U:case eR16U:case eR32U:
			return GL_RED;
		case eRG8I:case eRG16I:case eRG32I:
		case eRG8U:case eRG16U:case eRG32U:
			return GL_RG;
		case eRGB8:
		case eRGB32F:case eRGB16F:
		case eRGB8I:case eRGB16I:case eRGB32I:
		case eRGB8U:case eRGB16U:case eRGB32U:
			return GL_RGB;
		case eRGBA8:
		case eRGBA32F:case eRGBA16F:
		case eRGBA8I:case eRGBA16I:case eRGBA32I:
		case eRGBA8U:case eRGBA16U:case eRGBA32U:
			return GL_RGBA;
		}
		return 0;
	}

	GLenum Texture::GetBaseFormat(Format format)
	{
		switch( format )
		{
		case eRGB8: case eRGB32F:case eRGB16F:
			return GL_RGB;
		case eRGBA8:case eRGBA32F:case eRGBA16F:
			return GL_RGBA;
		case eR32F: case eR16: case eR8:
			return GL_RED;
		case eR8I:case eR16I:case eR32I:
		case eR8U:case eR16U:case eR32U:
			return GL_RED_INTEGER;
		case eRG8I:case eRG16I:case eRG32I:
		case eRG8U:case eRG16U:case eRG32U:
			return GL_RG_INTEGER;
		case eRGB8I:case eRGB16I:case eRGB32I:
		case eRGB8U:case eRGB16U:case eRGB32U:
			return GL_RGB_INTEGER;
		case eRGBA8I:case eRGBA16I:case eRGBA32I:
		case eRGBA8U:case eRGBA16U:case eRGBA32U:
			return GL_RGBA_INTEGER;
		}
		return 0;

	}

	GLenum Texture::GetImage2DType(Format format)
	{
		switch( format )
		{
		case eRGBA8:
		case eRGB8:
		case eR32F:case eR16:case eR8:
		case eRGB32F:
		case eRGBA32F:
		case eRGB16F:
		case eRGBA16F:
			return GL_IMAGE_2D;
		case eR8I:case eR16I:case eR32I:
		case eRG8I:case eRG16I:case eRG32I:
		case eRGB8I:case eRGB16I:case eRGB32I:
		case eRGBA8I:case eRGBA16I:case eRGBA32I:
			return GL_INT_IMAGE_2D;
		case eR8U:case eR16U:case eR32U:
		case eRG8U:case eRG16U:case eRG32U:
		case eRGB8U:case eRGB16U:case eRGB32U:
		case eRGBA8U:case eRGBA16U:case eRGBA32U:
			return GL_UNSIGNED_INT_IMAGE_2D;
		}
		return 0;
	}

	uint32 Texture::GetFormatSize(Format format)
	{
		uint32 result = 0;
		switch( gTexConvMap[format].compType )
		{
		case GL_FLOAT:
		case GL_INT:
		case GL_UNSIGNED_INT:
			result = 4;
			break;
		case GL_HALF_FLOAT:
		case GL_SHORT:
		case GL_UNSIGNED_SHORT:
			result = 2;
			break;
		case GL_BYTE:
		case GL_UNSIGNED_BYTE:
			result = 1;
			break;
		}

		result *= gTexConvMap[format].compNum;
		return result;
	}



	GLenum GLConvert::To(AccessOperator op)
	{
		switch( op )
		{
		case AO_READ_ONLY: return GL_READ_ONLY;
		case AO_WRITE_ONLY: return GL_WRITE_ONLY;
		case AO_READ_AND_WRITE: return GL_READ_WRITE;
		}
		assert(0);
		return GL_READ_WRITE;
	}

	GLenum GLConvert::To(PrimitiveType type)
	{
		switch( type )
		{
		case PrimitiveType::eTriangleList:  return GL_TRIANGLES;
		case PrimitiveType::eTriangleStrip: return GL_TRIANGLE_STRIP;
		case PrimitiveType::eTriangleFan:   return GL_TRIANGLE_FAN;
		case PrimitiveType::eLineList:      return GL_LINES;
		case PrimitiveType::eLineStrip:     return GL_LINE_STRIP;
		case PrimitiveType::eQuad:          return GL_QUADS;
		case PrimitiveType::ePoints:        return GL_POINTS;
		}
		return GL_POINTS;
	}

	GLenum GLConvert::To(Shader::Type type)
	{
		switch( type )
		{
		case Shader::eVertex:   return GL_VERTEX_SHADER;
		case Shader::ePixel:    return GL_FRAGMENT_SHADER;
		case Shader::eGeometry: return GL_GEOMETRY_SHADER;
		case Shader::eCompute:  return GL_COMPUTE_SHADER;
		case Shader::eHull:     return GL_TESS_CONTROL_SHADER;
		case Shader::eDomain:   return GL_TESS_EVALUATION_SHADER;
		}
		return 0;
	}

	bool ShaderParameter::bind(ShaderProgram& program, char const* name)
	{
		mLoc = program.getParamLoc(name);
		return mLoc != -1;
	}

}//namespace GL