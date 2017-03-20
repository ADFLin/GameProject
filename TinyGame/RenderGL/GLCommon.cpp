#include "GLCommon.h"

#include "GpuProfiler.h"


#include "CommonMarco.h"
#include "FileSystem.h"

#include "stb/stb_image.h"

#include <fstream>
#include <sstream>

namespace GL
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

		bool result = compileSource( type , src , num );
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
			case GL_GEOMETRY_SHADER: return eGemotery;
			}
		}
		return eUnknown;
	}

	bool RHIShader::compileSource( Type type , char const* src[] , int num )
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
		case eGemotery: mHandle = glCreateShader( GL_GEOMETRY_SHADER ); break;
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
			glValidateProgram(mHandle);
			checkProgramStatus();
			mNeedLink = false;
		}
	}

	void ShaderProgram::checkProgramStatus()
	{
		GLint value;
		glGetProgramiv(mHandle, GL_LINK_STATUS, &value);
		if( value != GL_TRUE )
		{
			GLchar buffer[4096];
			GLsizei size;
			glGetProgramInfoLog(mHandle, ARRAY_SIZE(buffer), &size, buffer);
			//::Msg("Can't Link Program : %s", buffer);
			int i = 1;
		}

		glGetProgramiv(mHandle, GL_VALIDATE_STATUS, &value);
		if( value != GL_TRUE )
		{
			GLchar buffer[4096];
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
		mIdxTextureAutoBind = IdxTextureAutoBindStart;
	}

	void ShaderProgram::unbind()
	{
		glUseProgram( 0 );
	}

	Mesh::Mesh()
	{
		mType = eTriangleList;
		mVAO = 0;
	}

	Mesh::~Mesh()
	{

	}

	bool Mesh::create(void* pVertex , int nV , void* pIdx , int nIndices , bool bIntIndex)
	{
		//glGenVertexArrays(1, &mVAO);
		//glBindVertexArray(mVAO);
		mVertexBuffer = new VertexBufferRHI;
		if( !mVertexBuffer->create(mDecl.getVertexSize() , nV , pVertex ) )
			return false;

		if ( nIndices )
		{
			mIndexBuffer = new IndexBufferRHI;
			if( !mIndexBuffer->create(nIndices, bIntIndex, pIdx) )
				return false;
		}
		
		//glBindVertexArray(0);

		mDecl.unbind();
		return true;
	}

	void Mesh::draw(bool bUseVAO)
	{
		if( mVertexBuffer == nullptr )
			return;

		//glBindVertexArray(mVAO);
		//if( bUseVAO )
			//mDecl.bindVAO();

		GL_BIND_LOCK_OBJECT(mVertexBuffer);
		
		if( !bUseVAO )
			mDecl.bind();

		if ( mIndexBuffer )
		{
			GL_BIND_LOCK_OBJECT(mIndexBuffer);
			glDrawElements( convert( mType ) , mIndexBuffer->mNumIndices , mIndexBuffer->mbIntIndex ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT , (void*)0 );
		}
		else
		{
			glDrawArrays( convert( mType ) , 0 , mVertexBuffer->mNumVertices );
		}
		checkGLError();

		//if( bUseVAO )
			//mDecl.unbindVAO();
		//else
			mDecl.unbind();
		//glBindVertexArray(0);
	}


	void Mesh::drawSection(int idx)
	{
		Section& section = mSections[idx];
		drawInternal( section.start , section.num);
	}

	void Mesh::drawInternal(int idxStart, int num)
	{
		if( mVertexBuffer == nullptr )
			return;

		GL_BIND_LOCK_OBJECT(mVertexBuffer);

		mDecl.bind();

		{
			if( mIndexBuffer )
			{
				GL_BIND_LOCK_OBJECT(mIndexBuffer);
				glDrawElements(convert(mType), num, mIndexBuffer->mbIntIndex ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT, (void*)idxStart);
			}
			else
			{
				glDrawArrays(convert(mType), idxStart, num);
			}
		}
		checkGLError();
		mDecl.unbind();
		
	}

	VertexDecl::VertexDecl()
	{
		mVertexSize = 0;
		mVAO = 0;
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

	enum
	{
		ATTRIBUTE0,
		ATTRIBUTE1,
		ATTRIBUTE2,
		ATTRIBUTE3,
		ATTRIBUTE4,
		ATTRIBUTE5,
		ATTRIBUTE6,
		ATTRIBUTE7,
		ATTRIBUTE8,
		ATTRIBUTE9,
		ATTRIBUTE10,
		ATTRIBUTE11,
		ATTRIBUTE12,
		ATTRIBUTE13,
		ATTRIBUTE14,
		ATTRIBUTE15,

		ATTRIBUTE_POSITION = ATTRIBUTE0,
		ATTRIBUTE_COLOR = ATTRIBUTE1,
		ATTRIBUTE_NORMAL = ATTRIBUTE2,
		ATTRIBUTE_TANGENT = ATTRIBUTE3,
		ATTRIBUTE_TEXCOORD = ATTRIBUTE4,
	};
	
	void VertexDecl::setupVAO()
	{
		for( Info& info : mInfoVec )
		{
			switch( info.semantic )
			{
			case Vertex::ePosition:
				glEnableVertexAttribArray(ATTRIBUTE_POSITION);
				glVertexAttribPointer(ATTRIBUTE_POSITION, getElementSize(info.format), getFormatType(info.format), GL_FALSE, mVertexSize, (void*)info.offset);
				break;
			case Vertex::eNormal:
				glEnableVertexAttribArray(ATTRIBUTE_NORMAL);
				glVertexAttribPointer(ATTRIBUTE_NORMAL, getElementSize(info.format), getFormatType(info.format), GL_FALSE, mVertexSize, (void*)info.offset);
				break;
			case Vertex::eTangent:
				glEnableVertexAttribArray(ATTRIBUTE_TANGENT);
				glVertexAttribPointer(ATTRIBUTE_TANGENT, getElementSize(info.format), getFormatType(info.format), GL_FALSE, mVertexSize, (void*)info.offset);
				break;
			case Vertex::eColor:
				glEnableVertexAttribArray(ATTRIBUTE_COLOR);
				glVertexAttribPointer(ATTRIBUTE_COLOR, getElementSize(info.format), getFormatType(info.format), GL_FALSE, mVertexSize, (void*)info.offset);
				break;
			case Vertex::eTexcoord:
				glEnableVertexAttribArray(ATTRIBUTE_TEXCOORD + info.idx);
				glVertexAttribPointer(ATTRIBUTE_TEXCOORD + info.idx , getElementSize(info.format), getFormatType(info.format), GL_FALSE, mVertexSize, (void*)info.offset);
				break;
			}
		}
	}

	VertexDecl& VertexDecl::addElement(Vertex::Semantic s, Vertex::Format f, uint8 idx /*= 0 */)
	{
		Info info;
		info.semantic = s;
		info.format   = f;
		info.offset   = mVertexSize;
		info.idx      = idx;
		mInfoVec.push_back( info );
		mVertexSize += getFormatSize( f );
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
		}
		return 0;
	}

	GLenum VertexDecl::getFormatType(uint8 format)
	{
		switch( format )
		{
		case Vertex::eFloat1:
		case Vertex::eFloat2:
		case Vertex::eFloat3:
		case Vertex::eFloat4:
			return GL_FLOAT;
		}
		return GL_FLOAT;
	}

	int VertexDecl::getElementSize(uint8 format)
	{
		switch( format )
		{
		case Vertex::eFloat1: return 1;
		case Vertex::eFloat2: return 2;
		case Vertex::eFloat3: return 3;
		case Vertex::eFloat4: return 4;
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

	bool RHITexture2D::create(Texture::Format format , int width , int height)
	{
		if ( !fetchHandle() )
			return false;
		mSizeX = width;
		mSizeY = height;

		glBindTexture( GL_TEXTURE_2D , mHandle );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexImage2D(GL_TEXTURE_2D, 0, Texture::Convert( format ) , width , height , 0, GL_RGBA , GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);
		return true;
	}

	bool RHITexture2D::create(Texture::Format format, int width, int height, void* data)
	{
		if( !fetchHandle() )
			return false;
		mSizeX = width;
		mSizeY = height;

		glBindTexture(GL_TEXTURE_2D, mHandle);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, Texture::Convert(format), width, height, 0, Texture::GetBaseFormat(format) , Texture::GetFormatType(format), data );
		glBindTexture(GL_TEXTURE_2D, 0);
		return true;
	}

	bool RHITexture2D::loadFile(char const* path)
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
		glTexImage3D(GL_TEXTURE_2D, 0, Texture::Convert(format), sizeX, sizeY , sizeZ, 0, GL_RGBA, GL_FLOAT, NULL);
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

	bool RHITextureCube::create(Texture::Format format , int width , int height)
	{
		if ( !fetchHandle() )
			return false;

		glBindTexture( GL_TEXTURE_CUBE_MAP , mHandle );
		for( int i = 0 ; i < 6 ; ++i )
		{
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i , 0, Texture::Convert( format ) , width , height , 0, GL_RGBA , GL_FLOAT, NULL );
		}
		glBindTexture( GL_TEXTURE_CUBE_MAP , 0 );
		return true;
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
						 Texture::Convert(format), width, height, 0, 
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
		if ( !mTextures.empty() )
		{
			for( int i = 0 ; i < mTextures.size() ; ++i )
			{
				BufferInfo& info = mTextures[i];
				if ( info.handle && info.bManaged )
					glDeleteTextures( 1 , &info.handle );
			}
			mTextures.clear();
		}
		if ( mFBO )
		{
			glDeleteFramebuffers( 1 , &mFBO );
			mFBO = 0;
		}
	}

	int FrameBuffer::addTexture( RHITextureCube& target , Texture::Face face , bool beManaged )
	{
		int idx = mTextures.size();

		BufferInfo info;
		info.handle   = target.mHandle;
		info.idxFace      = face;
		info.bManaged = ( beManaged ) ? target.mbManaged : false;
		if ( info.bManaged )
			target.releaseResource();
		mTextures.push_back( info );
		
		setTextureInternal( idx , info , GL_TEXTURE_CUBE_MAP_POSITIVE_X + info.idxFace );
		return idx;
	}

	int FrameBuffer::addTexture( RHITexture2D& target , bool beManaged )
	{
		int idx = mTextures.size();

		BufferInfo info;
		info.handle   = target.mHandle;
		info.idxFace      = 0;
		info.bManaged = ( beManaged ) ? target.mbManaged : false;
		if ( info.bManaged )
			target.releaseResource();

		mTextures.push_back( info );
		setTextureInternal( idx , info , GL_TEXTURE_2D );
		
		return idx;
	}

	void FrameBuffer::setTexture( int idx , RHITexture2D& target , bool beManaged )
	{
		assert( idx < mTextures.size() );
		BufferInfo& info = mTextures[idx];
		info.handle   = target.mHandle;
		info.idxFace  = 0;
		info.bManaged = ( beManaged ) ? target.mbManaged : false;
		if ( info.bManaged )
			target.releaseResource();
		setTextureInternal( idx , info , GL_TEXTURE_2D );
	}

	void FrameBuffer::setTexture( int idx , RHITextureCube& target , Texture::Face face , bool beManaged )
	{
		assert( idx < mTextures.size() );
		BufferInfo& info = mTextures[idx];
		info.handle   = target.mHandle;
		info.idxFace      = face;
		info.bManaged = ( beManaged ) ? target.mbManaged : false;
		if ( info.bManaged )
			target.releaseResource();
		setTextureInternal( idx , info , GL_TEXTURE_CUBE_MAP_POSITIVE_X + info.idxFace );
	}


	void FrameBuffer::setTextureInternal(int idx, BufferInfo& info, GLenum texType)
	{
		assert( mFBO );
		glBindFramebuffer( GL_FRAMEBUFFER , mFBO );
		glFramebufferTexture2D( GL_FRAMEBUFFER , GL_COLOR_ATTACHMENT0 + idx , texType , info.handle , 0 );
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
#if 0
		for( int i = 0; i < mTextures.size(); ++i )
		{
			BufferInfo& info = mTextures[i];
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, info.handle, 0);
		}
#endif
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

	void FrameBuffer::setDepthInternal(GLuint handle, Texture::DepthFormat format, bool bTexture, bool bManaged)
	{
		removeDepthBuffer();

		mDepth.handle = handle;
		mDepth.bManaged = bManaged;
		mDepth.bTexture = bTexture;

		if( mDepth.handle )
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
				glFramebufferTexture2D(GL_FRAMEBUFFER, attachType , GL_TEXTURE_2D, mDepth.handle, 0);
			}
			else
			{
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachType, GL_RENDERBUFFER, mDepth.handle);
			}


			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if( status != GL_FRAMEBUFFER_COMPLETE )
			{
				::Msg("DepthBuffer Can't Attach to FrameBuffer");
			}
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}
	}

	void FrameBuffer::setDepth( DepthRenderBuffer& buffer , bool bManaged )
	{
		bManaged = (bManaged) ? buffer.mbManaged : false;
		setDepthInternal( buffer.mHandle , buffer.getFormat() , false , bManaged );
		if ( bManaged )
			buffer.releaseResource();
	}

	void FrameBuffer::setDepth(RHITextureDepth& target, bool bManaged /*= false */)
	{
		bManaged = (bManaged) ? target.mbManaged : false;
		setDepthInternal(target.mHandle, target.getFormat(), true , bManaged);
		if( bManaged )
			target.releaseResource();
	}

	void FrameBuffer::removeDepthBuffer()
	{
		if ( mDepth.handle )
		{
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

			if( mDepth.bManaged )
			{
				if( mDepth.bTexture )
				{
					glDeleteTextures(1, &mDepth.handle);
				}
				else
				{
					glDeleteRenderbuffers(1, &mDepth.handle);
				}
				mDepth.bManaged = false;
			}
			mDepth.handle = 0;
		}

	}

	bool DepthRenderBuffer::create(int w, int h, Texture::DepthFormat format)
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


}//namespace GL