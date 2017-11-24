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



	bool RHIShader::loadFile( Type type , char const* path , char const* def )
	{

		std::vector< char > codeBuffer;
		if( !FileUtility::LoadToBuffer(path, codeBuffer) )
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

	RHIShader::Type RHIShader::getType()
	{
		if ( mHandle )
		{
			switch( getParam( GL_SHADER_TYPE ) )
			{
			case GL_VERTEX_SHADER:   return eVertex;
			case GL_FRAGMENT_SHADER: return ePixel;
			case GL_GEOMETRY_SHADER: return eGeometry;
			case GL_COMPUTE_SHADER:  return eCompute;
			case GL_TESS_CONTROL_SHADER: return eHull;
			case GL_TESS_EVALUATION_SHADER: return eDomain;
			}
		}
		return eUnknown;
	}

	bool RHIShader::compileCode( Type type , char const* src[] , int num )
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

	bool RHIShader::create(Type type)
	{
		if ( mHandle )
		{
			if ( getType() == type )
				return true;
			destroy();
		}
		switch( type )
		{
		case eVertex:   mHandle = glCreateShader( GL_VERTEX_SHADER ); break;
		case ePixel:    mHandle = glCreateShader( GL_FRAGMENT_SHADER ); break;
		case eGeometry: mHandle = glCreateShader( GL_GEOMETRY_SHADER ); break;
		case eCompute:  mHandle = glCreateShader( GL_COMPUTE_SHADER ); break;
		case eHull:     mHandle = glCreateShader( GL_TESS_CONTROL_SHADER ); break;
		case eDomain:   mHandle = glCreateShader( GL_TESS_EVALUATION_SHADER ); break;
		}
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

	RHIShader* ShaderProgram::removeShader( RHIShader::Type type )
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
		RHIShader::Type type = shader.getType();
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
		for( Info& info : mInfoVec )
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
		for( Info& info : mInfoVec )
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
		for( Info& info : mInfoVec )
		{
			glEnableVertexAttribArray(info.semantic);
			glVertexAttribPointer(info.semantic, getElementSize(info.format), getFormatType(info.format), GL_FALSE, mVertexSize, (void*)info.offset);
		}
	}

	void VertexDecl::setupVAOEnd()
	{
		for( Info& info : mInfoVec )
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
		Info info;
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
		Info info;
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

	VertexDecl::Info const* VertexDecl::findBySematic(Vertex::Semantic s , int idx) const
	{
		for( InfoVec::const_iterator iter = mInfoVec.begin(),itEnd = mInfoVec.end() ;
			iter != itEnd ; ++iter )
		{
			Info const& info = *iter;
			if ( iter->semantic == s && iter->idx == idx )
				return &info;
		}
		return NULL;
	}

	VertexDecl::Info const* VertexDecl::findBySematic(Vertex::Semantic s) const
	{
		for( InfoVec::const_iterator iter = mInfoVec.begin(),itEnd = mInfoVec.end() ;
			iter != itEnd ; ++iter )
		{
			Info const& info = *iter;
			if ( iter->semantic == s )
				return &info;
		}
		return NULL;
	}

	int VertexDecl::getSematicOffset(Vertex::Semantic s) const
	{
		Info const* info = findBySematic( s );
		return ( info ) ? info->offset : -1;
	}

	int VertexDecl::getSematicOffset(Vertex::Semantic s , int idx) const
	{
		Info const* info = findBySematic( s ,idx );
		return ( info ) ? info->offset : -1;
	}

	Vertex::Format VertexDecl::getSematicFormat(Vertex::Semantic s) const
	{
		Info const* info = findBySematic( s );
		return ( info ) ? Vertex::Format( info->format ) : Vertex::eUnknowFormat;
	}

	Vertex::Format VertexDecl::getSematicFormat(Vertex::Semantic s , int idx) const
	{
		Info const* info = findBySematic( s , idx );
		return ( info ) ? Vertex::Format( info->format ) : Vertex::eUnknowFormat;
	}

	bool RHITextureBase::loadFileInternal(char const* path, GLenum texType, Vec2i& outSize)
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
			glTexImage2D(texType, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image); 
			break;
		case 4:
			glTexImage2D(texType, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
			break;
		}
		//glGenerateMipmap( texType);
		stbi_image_free(image);
		return true;
	}


	bool RHITexture2D::create(Texture::Format format, int width, int height, void* data)
	{
		if( !fetchHandle() )
			return false;
		mSizeX = width;
		mSizeY = height;
		mFormat = format;

		glBindTexture(GL_TEXTURE_2D, mHandle);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GLConvert::To(format), width, height, 0, 
					 Texture::GetBaseFormat(format) , Texture::GetFormatType(format), data );
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
		bool result = loadFileInternal( path , GL_TEXTURE_2D , size );
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

	bool RHITexture3D::create(Texture::Format format, int sizeX , int sizeY , int sizeZ )
	{
		if( !fetchHandle() )
			return false;
		mSizeX = sizeX;
		mSizeY = sizeY;
		mSizeZ = sizeZ;

		glBindTexture(GL_TEXTURE_2D, mHandle);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage3D(GL_TEXTURE_2D, 0, GLConvert::To(format), sizeX, sizeY , sizeZ, 0, 
					 Texture::GetBaseFormat(format), Texture::GetFormatType(format), NULL);
		glBindTexture(GL_TEXTURE_2D, 0);
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
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 
						 GLConvert::To(format), width, height, 0,
						 Texture::GetBaseFormat(format), Texture::GetFormatType(format), data );
		}
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		return true;
	}

	bool RHITextureCube::loadFile(char const* path[])
	{
		if ( !fetchHandle() )
			return false;

		glBindTexture( GL_TEXTURE_2D , mHandle );

		bool result = true; 
		for( int i = 0 ; i < 6 ; ++i )
		{
			Vec2i size;
			if ( !loadFileInternal( path[i] , GL_TEXTURE_CUBE_MAP_POSITIVE_X + i , size ) )
			{
				result = false;
			}
		}

		glBindTexture(GL_TEXTURE_2D, 0);
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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_POINT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_POINT);
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


	GLenum GLConvert::To(Texture::Format format)
	{
		switch( format )
		{
		case Texture::eRGBA8:   return GL_RGBA8;
		case Texture::eRGB8:    return GL_RGB8;
		case Texture::eR32F:    return GL_R32F;
		case Texture::eRGB32F:  return GL_RGB32F;
		case Texture::eRGBA32F: return GL_RGBA32F;
		case Texture::eRGB16F:  return GL_RGB16F;
		case Texture::eRGBA16F:  return GL_RGBA16F;
		case Texture::eR8I: return GL_R8I;
		case Texture::eR16I:  return GL_R16I;
		case Texture::eR32I:  return GL_R32I;
		case Texture::eR8U: return GL_R8UI;
		case Texture::eR16U: return GL_R16UI;
		case Texture::eR32U: return GL_R32UI;
		case Texture::eRG8I: return GL_RG8I;
		case Texture::eRG16I:  return GL_RG16I;
		case Texture::eRG32I:  return GL_RG32I;
		case Texture::eRG8U: return GL_RG8UI;
		case Texture::eRG16U: return GL_RG16UI;
		case Texture::eRG32U: return GL_RG32UI;
		case Texture::eRGB8I: return GL_RGB8I;
		case Texture::eRGB16I:  return GL_RGB16I;
		case Texture::eRGB32I:  return GL_RGB32I;
		case Texture::eRGB8U: return GL_RGB8UI;
		case Texture::eRGB16U: return GL_RGB16UI;
		case Texture::eRGB32U: return GL_RGB32UI;
		case Texture::eRGBA8I: return GL_RGBA8I;
		case Texture::eRGBA16I:  return GL_RGBA16I;
		case Texture::eRGBA32I:  return GL_RGBA32I;
		case Texture::eRGBA8U: return GL_RGBA8UI;
		case Texture::eRGBA16U: return GL_RGBA16UI;
		case Texture::eRGBA32U: return GL_RGBA32UI;
		}
		return 0;
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
		case PrimitiveType::eQuad:          return GL_QUADS;
		case PrimitiveType::ePoints:        return GL_POINTS;
		}
		return GL_POINTS;
	}

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

	GLenum Texture::GetBaseFormat(Format format)
	{
		switch( format )
		{
		case eRGB8:
		case eRGB32F:
		case eRGB16F:
			return GL_RGB;
		case eRGBA8:
		case eRGBA32F:
		case eRGBA16F:
			return GL_RGBA;
		case eR32F:
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

	GLenum Texture::GetFormatType(Format format)
	{
		switch( format )
		{
		case eR16I:case eRG16I:case eRGB16I:case eRGBA16I:
			return GL_SHORT;
		case eR32I:case eRG32I:case eRGB32I:case eRGBA32I:
			return GL_INT;
		case eR16U:case eRG16U:case eRGB16U:case eRGBA16U:
			return GL_UNSIGNED_SHORT;
		case eR32U:case eRG32U:case eRGB32U:case eRGBA32U:
			return GL_UNSIGNED_INT;
		case eR8I:
			return GL_BYTE;
		case eR8U:
		case eRGB8:
		case eRGBA8:
			return GL_UNSIGNED_BYTE;
		case eRGB32F:
		case eRGB16F:
		case eRGBA32F:
		case eRGBA16F:
		case eR32F:
			return GL_FLOAT;
		}
		return 0;
	}

	GLenum Texture::GetImage2DType(Format format)
	{
		switch( format )
		{
		case eRGBA8:
		case eRGB8:
		case eR32F:
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

	bool ShaderParameter::bind(ShaderProgram& program, char const* name)
	{
		mLoc = program.getParamLoc(name);
		return mLoc != -1;
	}

}//namespace GL