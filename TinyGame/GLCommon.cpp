#include "TinyGamePCH.h"
#include "GLCommon.h"

#include "stb/stb_image.h"

#include <fstream>

namespace GL
{

	bool Shader::loadFile( Type type , char const* path , char const* def )
	{
		std::ifstream fs( path );

		if ( !fs.is_open() )
			return false;

		int64 size = 0;

		fs.seekg(0, std::ios::end); 
		size = fs.tellg();
		fs.seekg(0, std::ios::beg);
		size -= fs.tellg();

		std::vector< char > buf(size + 1);
		fs.read( &buf[0] , size );

		buf[ size ]='\0';
		fs.close();

		int num = 0;
		char const* src[2];
		if ( def )
		{
			src[num] = def;
			++num;
		}
		src[num] = &buf[0];
		++num;

		bool result = compileSource( type , src , num );
		return result;

	}

	Shader::Type Shader::getType()
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

	bool Shader::compileSource( Type type , char const* src[] , int num )
	{
		if ( !create( type ) )
			return false;

		glShaderSource( mHandle , num , src , 0 );
		glCompileShader( mHandle );

		if ( getParam( GL_COMPILE_STATUS ) == GL_FALSE )
		{
			int maxLength;
			glGetShaderiv( mHandle ,GL_INFO_LOG_LENGTH,&maxLength);
			std::vector< char > buf( maxLength + 1 );
			int logLength = 0;
			glGetShaderInfoLog( mHandle , maxLength, &logLength, &buf[0] );
			::MessageBoxA( NULL , &buf[0] , "Error" , 0 );
			return false;
		}
		return true;
	}

	bool Shader::create(Type type)
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

	void Shader::destroy()
	{
		if ( mHandle )
		{
			glDeleteShader( mHandle );
			mHandle = 0;
		}
	}

	GLuint Shader::getParam(GLuint val)
	{
		GLint status;
		glGetShaderiv( mHandle , val , &status );
		return status;
	}


	ShaderProgram::ShaderProgram()
	{
		mHandle = 0;
		mNeedLink = true;
		std::fill_n( mShader , NumShader , (Shader*)0 );
	}

	ShaderProgram::~ShaderProgram()
	{
		if ( mHandle )
			glDeleteProgram( mHandle );
	}

	bool ShaderProgram::create()
	{
		if ( mHandle == 0 )
			mHandle = glCreateProgram();
		return mHandle != 0;
	}

	Shader* ShaderProgram::removeShader( Shader::Type type )
	{
		Shader* out = mShader[ type ];
		if ( mShader[ type ] )
		{
			glDetachShader( mHandle , mShader[ type ]->mHandle );
			mShader[ type ] = NULL;
			mNeedLink = true;
		}
		return out;
	}

	void ShaderProgram::attachShader(Shader& shader)
	{
		Shader::Type type = shader.getType();
		if ( mShader[ type ] )
			glDetachShader( mHandle , mShader[ type ]->mHandle );
		glAttachShader( mHandle , shader.mHandle );
		mShader[ type ] = &shader;
		mNeedLink = true;
	}

	void ShaderProgram::updateShader()
	{
		if ( mNeedLink )
		{
			glLinkProgram( mHandle );
			mNeedLink = false;
		}
	}

	void ShaderProgram::bind()
	{
		updateShader();
		glUseProgram( mHandle );
	}

	void ShaderProgram::unbind()
	{
		glUseProgram( 0 );
	}

	Mesh::Mesh()
	{
		mType = eTriangleList;
		mVboVertex = 0;
		mVboIndex = 0;
		mNumIndices = 0;
		mNumVertices = 0;
		mbIntIndex = false;
	}

	Mesh::~Mesh()
	{
		if ( mVboVertex )
		{
			glDeleteBuffers( 1 , &mVboVertex );
		}
		if ( mVboIndex )
		{
			glDeleteBuffers( 1 , &mVboIndex );
		}
	}

	bool Mesh::createBuffer(void* pVertex , int nV , void* pIdx , int nIndices , bool bIntIndex)
	{
		glGenBuffers( 1 , &mVboVertex );
		glBindBuffer( GL_ARRAY_BUFFER , mVboVertex );
		glBufferData( GL_ARRAY_BUFFER , mDecl.getSize() * nV , pVertex , GL_STATIC_DRAW );
		glBindBuffer( GL_ARRAY_BUFFER , 0 );

		glGenBuffers( 1 , &mVboIndex );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER , mVboIndex );
		glBufferData( GL_ELEMENT_ARRAY_BUFFER , ( (bIntIndex)?sizeof(GLuint):sizeof(GLushort) ) * nIndices , pIdx , GL_STATIC_DRAW );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER , 0 );

		mNumIndices  = nIndices;
		mNumVertices = nV;
		mbIntIndex   = bIntIndex;
		return true;
	}

	void Mesh::draw()
	{
		glBindBuffer( GL_ARRAY_BUFFER , mVboVertex );
		mDecl.bindArray();

		if ( mVboIndex )
		{
			glBindBuffer( GL_ELEMENT_ARRAY_BUFFER , mVboIndex );
			glDrawElements( convert( mType ) , mNumIndices , mbIntIndex ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT , (void*)0 );
			glBindBuffer( GL_ELEMENT_ARRAY_BUFFER_ARB , 0 );
		}
		else
		{
			glDrawArrays( convert( mType ) , 0 , mNumVertices );
		}

		mDecl.unbindArray();
		glBindBuffer( GL_ARRAY_BUFFER , 0 );
	}


	VertexDecl::VertexDecl()
	{
		mVertexSize = 0;
	}

	void VertexDecl::bindArray()
	{
		bool haveTex = false;
		for ( InfoVec::iterator iter = mInfoVec.begin() , itEnd = mInfoVec.end(); 
			iter != itEnd; ++iter )
		{
			Info& info = *iter;
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
			case Vertex::eTexcoord:
				glClientActiveTexture( GL_TEXTURE0 + info.idx );
				glEnableClientState( GL_TEXTURE_COORD_ARRAY );
				glTexCoordPointer( getElementSize( info.format ) , getFormatType( info.format ) , mVertexSize , (void*)info.offset );
				haveTex = true;
				break;
			}
		}
	}

	void VertexDecl::unbindArray()
	{
		bool haveTex = false;
		for ( InfoVec::iterator iter = mInfoVec.begin() , itEnd = mInfoVec.end(); 
			iter != itEnd; ++iter )
		{
			Info& info = *iter;
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
			case Vertex::eTexcoord:
				haveTex = false;
				glClientActiveTexture( GL_TEXTURE0 + info.idx );
				glDisableClientState( GL_TEXTURE_COORD_ARRAY );
				break;
			}
		}
		if ( haveTex )
			glClientActiveTexture( GL_TEXTURE0 );
	}

	VertexDecl& VertexDecl::addElement(Vertex::Semantic s , Vertex::Format f , uint8 idx /*= 0 */)
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


	bool TextureBase::loadFileInternal(char const* path , GLenum texType)
	{
		int w;
		int h;
		int comp;
		unsigned char* image = stbi_load( path , &w, &h, &comp, STBI_default );

		if( !image )
			return false;

		glTexParameteri( texType, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( texType, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		//TODO
		switch( comp )
		{
		case 3:
			glTexImage2D(texType, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image); 
			break;
		case 4:
			glTexImage2D(texType, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
			break;
		}
		//glGenerateMipmap( texType);
		stbi_image_free(image);
		return true;
	}

	bool Texture2D::create(Texture::Format format , int width , int height)
	{
		if ( !fetchHandle() )
			return false;
		glBindTexture( GL_TEXTURE_2D , mHandle );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexImage2D(GL_TEXTURE_2D, 0, Texture::convert( format ) , width , height , 0, GL_RGBA , GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);
		return true;
	}


	bool Texture2D::loadFile(char const* path)
	{
		if ( !fetchHandle() )
			return false;

		glBindTexture( GL_TEXTURE_2D , mHandle );
		bool result = loadFileInternal( path , GL_TEXTURE_2D );
		glBindTexture(GL_TEXTURE_2D, 0);
		return result;
	}

	void Texture2D::bind()
	{
		glBindTexture( GL_TEXTURE_2D , mHandle );
	}

	void Texture2D::unbind()
	{
		glBindTexture( GL_TEXTURE_2D , 0 );
	}

	bool TextureCube::create(Texture::Format format , int width , int height)
	{
		if ( !fetchHandle() )
			return false;

		glBindTexture( GL_TEXTURE_CUBE_MAP , mHandle );
		for( int i = 0 ; i < 6 ; ++i )
		{
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i , 0, Texture::convert( format ) , width , height , 0, GL_RGBA , GL_FLOAT, NULL );
		}
		glBindTexture( GL_TEXTURE_CUBE_MAP , 0 );
		return true;
	}

	bool TextureCube::loadFile(char const* path[])
	{
		if ( !fetchHandle() )
			return false;

		glBindTexture( GL_TEXTURE_2D , mHandle );

		bool result = true; 
		for( int i = 0 ; i < 6 ; ++i )
		{
			if ( !loadFileInternal( path[i] , GL_TEXTURE_CUBE_MAP_POSITIVE_X + i ) )
			{
				result = false;
			}
		}

		glBindTexture(GL_TEXTURE_2D, 0);
		return result;
	}

	void TextureCube::bind()
	{
		glBindTexture( GL_TEXTURE_CUBE_MAP , mHandle );
	}

	void TextureCube::unbind()
	{
		glBindTexture( GL_TEXTURE_CUBE_MAP , 0 );
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
				Info& info = mTextures[i];
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

	int FrameBuffer::addTexture( TextureCube& target , Texture::Face face , bool beManaged )
	{
		int idx = mTextures.size();

		Info info;
		info.handle   = target.mHandle;
		info.idx      = face;
		info.bManaged = ( beManaged ) ? target.mbManaged : false;
		if ( info.bManaged )
			target.release();
		mTextures.push_back( info );
		
		setTextureInternal( idx , info , GL_TEXTURE_CUBE_MAP_POSITIVE_X + info.idx );
		return idx;
	}

	int FrameBuffer::addTexture( Texture2D& target , bool beManaged )
	{
		int idx = mTextures.size();

		Info info;
		info.handle   = target.mHandle;
		info.idx      = 0;
		info.bManaged = ( beManaged ) ? target.mbManaged : false;
		if ( info.bManaged )
			target.release();

		mTextures.push_back( info );
		setTextureInternal( idx , info , GL_TEXTURE_2D );
		
		return idx;
	}

	void FrameBuffer::setTexture( int idx , Texture2D& target , bool beManaged )
	{
		assert( idx < mTextures.size() );
		Info info = mTextures[idx];
		info.handle   = target.mHandle;
		info.idx      = 0;
		info.bManaged = ( beManaged ) ? target.mbManaged : false;
		if ( info.bManaged )
			target.release();
		setTextureInternal( idx , info , GL_TEXTURE_2D );
	}

	void FrameBuffer::setTexture( int idx , TextureCube& target , Texture::Face face , bool beManaged )
	{
		assert( idx < mTextures.size() );
		Info info = mTextures[idx];
		info.handle   = target.mHandle;
		info.idx      = face;
		info.bManaged = ( beManaged ) ? target.mbManaged : false;
		if ( info.bManaged )
			target.release();
		setTextureInternal( idx , info , GL_TEXTURE_CUBE_MAP_POSITIVE_X + info.idx );
	}

	void FrameBuffer::setTextureInternal( int idx , Info& info , GLenum texType )
	{
		assert( mFBO );
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER , mFBO );
		glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER , GL_COLOR_ATTACHMENT0 + idx , texType , info.handle , 0 );


		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0 );
	}

	void FrameBuffer::bind()
	{
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, mFBO );

		GLenum DrawBuffers[] =
		{ 
			GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 , GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4,
			GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7 , GL_COLOR_ATTACHMENT8, GL_COLOR_ATTACHMENT9
		};
		glDrawBuffers( mTextures.size() , DrawBuffers );
	}

	void FrameBuffer::unbind()
	{
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
	}

	bool FrameBuffer::create()
	{
		glGenFramebuffers( 1, &mFBO ); 
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER , mFBO );
		return mFBO != 0;
	}

	void FrameBuffer::setBuffer( DepthBuffer& buffer , bool beManaged )
	{
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, mFBO );
		mDepthBuffer.mHandle = buffer.mHandle;
		mDepthBuffer.mbManaged = ( beManaged ) ? buffer.mbManaged : false;
		if ( mDepthBuffer.mbManaged )
			buffer.release();

		if ( mDepthBuffer.mHandle )
		{
			glFramebufferRenderbuffer( GL_FRAMEBUFFER , GL_DEPTH_STENCIL_ATTACHMENT , GL_RENDERBUFFER , mDepthBuffer.mHandle ); 
		}

		GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (Status != GL_FRAMEBUFFER_COMPLETE) 
		{

		}
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
	}


	bool DepthBuffer::create(int w , int h , Texture::DepthFormat format)
	{
		assert( mHandle == 0 );
		glGenRenderbuffers(1, &mHandle );
		glBindRenderbuffer( GL_RENDERBUFFER , mHandle );
		glRenderbufferStorage( GL_RENDERBUFFER , Texture::convert( format ), w , h );
		//glRenderbufferStorage( GL_RENDERBUFFER , GL_DEPTH_COMPONENT , w , h );
		glBindRenderbuffer( GL_RENDERBUFFER , 0 );

		return true;
	}


}//namespace GL