#ifndef GLCommon_h__
#define GLCommon_h__

#include "RHIType.h"
#include "RHIDefine.h"
#include "RHICommon.h"

#include "OpenGLHeader.h"
#include "CppVersion.h"
#include "LogSystem.h"
#include "RefCount.h"

#include <functional>
#include <type_traits>

#define IGNORE_NSIGHT_UNSUPPORTED_CODE 0
#define GL_NULL_HANDLE 0

namespace Render
{
	bool VerifyOpenGLStatus();

	template< class TFactory >
	class TOpenGLObject
	{
	public:
		TOpenGLObject()
		{
			mHandle = GL_NULL_HANDLE;
		}

		~TOpenGLObject()
		{
			destroyHandle();
		}

		TOpenGLObject(TOpenGLObject&& other)
		{
			mHandle = other.mHandle;
			other.mHandle = GL_NULL_HANDLE;
		}

		TOpenGLObject(TOpenGLObject const& other) { assert(0); }
		TOpenGLObject& operator = (TOpenGLObject const& rhs) = delete;

		bool isValid() { return mHandle != GL_NULL_HANDLE; }

#if CPP_VARIADIC_TEMPLATE_SUPPORT
		template< class ...Args >
		bool fetchHandle(Args&& ...args)
		{
			if( mHandle == GL_NULL_HANDLE )
				TFactory::Create(mHandle, std::forward<Args>(args)...);
			return isValid();
		}

#else
		bool fetchHandle()
		{
			if( !mHandle )
				TFactory::Create(mHandle);
			return isValid();
		}

		template< class P1 >
		bool fetchHandle(P1 p1)
		{
			if( !mHandle )
				TFactory::Create(mHandle, p1);
			return isValid();
		}

#endif

		bool destroyHandle()
		{
			if( mHandle )
			{
				TFactory::Destroy(mHandle);
				mHandle = GL_NULL_HANDLE;

				if (!VerifyOpenGLStatus())
				{
					return false;
				}
			}
			return true;
		}

		GLuint mHandle;
	};

	namespace GLFactory
	{
		struct Texture
		{
			static void Create(GLuint& handle) { glGenTextures(1, &handle); }
			static void Destroy(GLuint& handle) { glDeleteTextures(1, &handle); }
		};

		struct RenderBuffer
		{
			static void Create(GLuint& handle) { glGenRenderbuffers(1, &handle); }
			static void Destroy(GLuint& handle) { glDeleteRenderbuffers(1, &handle); }
		};

		struct Buffer
		{
			static void Create(GLuint& handle) { glCreateBuffers(1, &handle); }
			static void Destroy(GLuint& handle) { glDeleteBuffers(1, &handle); }
		};

		struct Sampler
		{
			static void Create(GLuint& handle) { glGenSamplers(1, &handle); }
			static void Destroy(GLuint& handle) { glDeleteSamplers(1, &handle); }
		};

	}

	template< class RHIResourceType , class TFactory >
	class TOpenGLResource : public  TRefcountResource< RHIResourceType > 
	{
	public:
		using TRefcountResource< RHIResourceType >::TRefcountResource;

		GLuint getHandle() const { return mGLObject.mHandle; }

		virtual void releaseResource() 
		{
			if (!mGLObject.destroyHandle())
			{
#if RHI_USE_RESOURCE_TRACE
				LogError("Can't Destory GL Object : %s %s", mTypeName.c_str(), mTrace.toString().c_str());
#else
				LogError("Can't Destory GL Object");
#endif
			}
		}

		TOpenGLObject< TFactory > mGLObject;
	};

	template< class RHITextureType >
	struct OpenGLTextureTraits {};

	template<>
	struct OpenGLTextureTraits< RHITexture1D >
	{
		static GLenum constexpr EnumValue = GL_TEXTURE_1D; 
		static GLenum constexpr EnumValueMultisample = GL_TEXTURE_1D;
	};
	template<>
	struct OpenGLTextureTraits< RHITexture2D >
	{
		static GLenum constexpr EnumValue = GL_TEXTURE_2D; 
		static GLenum constexpr EnumValueMultisample = GL_TEXTURE_2D_MULTISAMPLE;
	};
	template<>
	struct OpenGLTextureTraits< RHITexture3D >
	{
		static GLenum constexpr EnumValue = GL_TEXTURE_3D;
		static GLenum constexpr EnumValueMultisample = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
	};
	template<>
	struct OpenGLTextureTraits< RHITextureCube >
	{
		static GLenum constexpr EnumValue = GL_TEXTURE_CUBE_MAP; 
		static GLenum constexpr EnumValueMultisample = GL_TEXTURE_CUBE_MAP;
	};
	template<>
	struct OpenGLTextureTraits< RHITexture2DArray >
	{
		static GLenum constexpr EnumValue = GL_TEXTURE_2D_ARRAY;
		static GLenum constexpr EnumValueMultisample = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
	};

	class OpenGLShaderResourceView : public TPersistentResource< RHIShaderResourceView >
	{
	public:
		GLuint handle;
		GLenum typeEnum;
	};

	template< class RHITextureType >
	class TOpengGLTexture : public TOpenGLResource< RHITextureType , GLFactory::Texture >
	{
	public:
		TOpengGLTexture(TextureDesc const& desc)
			:TOpenGLResource< RHITextureType, GLFactory::Texture >()
		{
			mDesc = desc;
		}

		GLenum getGLTypeEnum() const
		{
			return (mDesc.numSamples > 1) ? TypeEnumGLMultisample : TypeEnumGL;
		}
		void bind() const
		{
			glBindTexture(getGLTypeEnum(), getHandle());
		}

		void unbind() const
		{
			glBindTexture(getGLTypeEnum(), 0);
		}

		RHIShaderResourceView* getBaseResourceView()
		{
			mView.handle = mGLObject.mHandle;
			mView.typeEnum = getGLTypeEnum();
			return &mView;
		}
	protected:
		static GLenum const TypeEnumGL = OpenGLTextureTraits< RHITextureType >::EnumValue;
		static GLenum const TypeEnumGLMultisample = OpenGLTextureTraits< RHITextureType >::EnumValueMultisample;


		OpenGLShaderResourceView mView;
	};


	class OpenGLTexture1D : public TOpengGLTexture< RHITexture1D >
	{
	public:
		using TOpengGLTexture< RHITexture1D >::TOpengGLTexture;

		bool create(void* data);
		bool update(int offset, int length, ETexture::Format format, void* data, int level );
	};


	class OpenGLTexture2D : public TOpengGLTexture< RHITexture2D >
	{
	public:
		using TOpengGLTexture< RHITexture2D >::TOpengGLTexture;

		bool create(void* data, int alignment);
		bool createDepth();

		bool update(int ox, int oy, int w, int h, ETexture::Format format, void* data, int level);
		bool update(int ox, int oy, int w, int h, ETexture::Format format, int dataImageWidth, void* data, int level);
	};


	class OpenGLTexture3D : public TOpengGLTexture< RHITexture3D >
	{
	public:
		using TOpengGLTexture< RHITexture3D >::TOpengGLTexture;

		bool create(void* data);
	};

	class OpenGLTextureCube : public TOpengGLTexture< RHITextureCube >
	{
	public:
		using TOpengGLTexture< RHITextureCube >::TOpengGLTexture;

		bool create(void* data[]);
		bool update(ETexture::Face face, int ox, int oy, int w, int h, ETexture::Format format, void* data, int level );
		bool update(ETexture::Face face, int ox, int oy, int w, int h, ETexture::Format format, int dataImageWidth, void* data, int level );
	};

	class OpenGLTexture2DArray : public TOpengGLTexture< RHITexture2DArray >
	{
	public:
		using TOpengGLTexture<RHITexture2DArray>::TOpengGLTexture;

		bool create(void* data);
	};

	class OpenGLTranslate
	{
	public:
		static GLenum To(EPrimitive type, int& outPatchPointCount);
		static GLenum To(EAccessOperator op);
		static GLenum To(ETexture::Format format);
		static GLenum To(EShader::Type type);
		static GLbitfield ToStageBit(EShader::Type type);
		static GLenum To(ELockAccess access);
		static GLenum To(EBlend::Factor factor);
		static GLenum To(EBlend::Operation op);
		static GLenum To(ECompareFunc func);
		static GLenum To(EStencil op);
		static GLenum To(ECullMode mode);
		static GLenum To(EFillMode mode);
		static GLenum To(EComponentType type );
		static GLenum To(ESampler::Filter filter);
		static GLenum To(ESampler::AddressMode mode);

		static GLenum DepthFormat(ETexture::Format format);

		static GLenum BaseFormat(ETexture::Format format);
		static GLenum PixelFormat(ETexture::Format format);
		static GLenum TextureComponentType(ETexture::Format format);
		static GLenum Image2DType(ETexture::Format format);
		static GLenum TexureType(ETexture::Face face);
		static GLenum BufferUsageEnum(uint32 creationFlags);

		static GLenum VertexComponentType(uint8 format)
		{
			return To(EVertex::GetComponentType(EVertex::Format(format)));
		}
		static GLenum IndexType(RHIBuffer* indexBuffer)
		{  
			return indexBuffer->getElementSize() == 4 ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT; 
		}
	};

#define USE_DepthRenderBuffer 0

#if USE_DepthRenderBuffer
	class RHIDepthRenderBuffer : public TRHIResource< GLFactory::RenderBuffer >
	{
	public:
		bool create( int w , int h , ETexture::Format format );
		ETexture::Format getFormat() { return mFormat;  }
		ETexture::Format mFormat;
	};


	typedef TRefCountPtr< RHIDepthRenderBuffer > RHIDepthRenderBufferRef;
#endif

	namespace GLFactory
	{
		struct FrameBuffer
		{
			static void Create(GLuint& handle) { glGenFramebuffers(1, &handle); }
			static void Destroy(GLuint& handle) { glDeleteFramebuffers(1, &handle); }
		};
	}


	class OpenGLFrameBuffer : public TOpenGLResource< RHIFrameBuffer , GLFactory::FrameBuffer >
	{
	public:
		OpenGLFrameBuffer();
		~OpenGLFrameBuffer();

		bool create();

		void setupTextureLayer(RHITextureCube& target, int level = 0);

		int  addTexture(RHITexture2D& target, int level = 0);
		int  addTexture(RHITextureCube& target, ETexture::Face face, int level = 0);
		int  addTexture(RHITexture2DArray& target, int indexLayer, int level = 0);
		void setTexture(int idx, RHITexture2D& target, int level = 0);
		void setTexture(int idx, RHITextureCube& target, ETexture::Face face, int level = 0);
		void setTexture(int idx, RHITexture2DArray& target, int indexLayer, int level = 0);

		
		void bindDepthOnly();
		void bind();
		void unbind();
#if USE_DepthRenderBuffer
		void setDepth( RHIDepthRenderBuffer& buffer );
#endif
		void setDepth( RHITexture2D& target );
		void removeDepth();

		CORE_API void blitToBackBuffer(int index = 0);

		
		struct BufferInfo
		{
			RHIResourceRef bufferRef;
			GLenum typeEnumGL;
			uint8  level;
			union
			{
				uint8  idxFace;
				int    indexLayer;
			};
		};
		void setRenderBufferInternal(GLuint handle);
		void setTexture2DInternal(int idx, GLuint handle, GLenum texType, int level);
		void setTexture3DInternal(int idx, GLuint handle, GLenum texType, int level, int idxLayer);
		void setTextureLayerInternal(int idx, GLuint handle, GLenum texType, int level, int idxLayer);
		void setDepthInternal(RHIResource& resource, GLuint handle, ETexture::Format format, GLenum typeEnumGL);

		TArray< BufferInfo > mTextures;
		BufferInfo  mDepth;
	};


	class OpenGLBuffer : public TOpenGLResource< RHIBuffer , GLFactory::Buffer >
	{
	public:

		void  bind(GLenum bufferType) const { glBindBuffer(bufferType, getHandle()); }
		void  unbind(GLenum bufferType) const { glBindBuffer(bufferType, 0); }

		void* lock(ELockAccess access)
		{
			void* result = glMapNamedBuffer(getHandle(), OpenGLTranslate::To(access));
			return result;
		}
		void* lock(ELockAccess access, uint32 offset, uint32 length)
		{
			GLbitfield accessBits = 0;
			switch( access )
			{
			case ELockAccess::ReadOnly:
				accessBits = GL_MAP_READ_BIT;
				break;
			case ELockAccess::ReadWrite:
				accessBits = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
				break;
			case ELockAccess::WriteDiscard:
			case ELockAccess::WriteOnly:
				accessBits = GL_MAP_WRITE_BIT;
				break;
			}
			void* result = glMapNamedBufferRange(getHandle(), offset, length, accessBits);
			return result;
		}

		void unlock()
		{
			glUnmapNamedBuffer(getHandle());
		}

		bool create(BufferDesc const& desc , void* initData)
		{
			if( !mGLObject.fetchHandle() )
				return false;

			return resetData(desc, initData);
		}

		bool resetData(BufferDesc const& desc, void* initData)
		{
			mDesc = desc;
			glNamedBufferData(getHandle(), mDesc.elementSize * mDesc.numElements, initData, OpenGLTranslate::BufferUsageEnum(mDesc.creationFlags));

			return true;
		}

		void updateData(uint32 start, uint32 numElements, void* data)
		{
			CHECK((start + numElements) * mDesc.elementSize < getSize());
			glNamedBufferSubData(getHandle(), start * mDesc.elementSize, mDesc.elementSize * numElements, data);
		}
	};

	class OpenGLSamplerState : public TOpenGLResource< RHISamplerState, GLFactory::Sampler >
	{
	public:
		bool create(SamplerStateInitializer const& initializer);
	};

	struct GLRasterizerStateValue
	{
		GLenum fillMode;
		GLenum cullFace;
		GLenum frontFace;
		bool   bEnableCull;
		bool   bEnableScissor;
		bool   bEnableMultisample;

		GLRasterizerStateValue() {}
		GLRasterizerStateValue(EForceInit)
		{
			bEnableCull = false;
			bEnableScissor = false;
			bEnableMultisample = false;
			fillMode = GL_FILL;
			cullFace = GL_BACK;
			frontFace = GL_CW;
		}
	};

	class OpenGLRasterizerState : public TRefcountResource< RHIRasterizerState >
	{
	public:
		OpenGLRasterizerState(RasterizerStateInitializer const& initializer);
		GLRasterizerStateValue mStateValue;
	};

	struct GLDepthStencilStateValue
	{
		bool   bEnableDepthTest;
		bool   bEnableStencilTest;
		bool   bWriteDepth;
		bool   bUseSeparateStencilFun;
		bool   bUseSeparateStencilOp;

		GLenum depthFun;

		GLenum stencilFunc;
		GLenum stencilFailOp;
		GLenum stencilZFailOp;
		GLenum stencilZPassOp;

		GLenum stencilFunBack;
		GLenum stencilFailOpBack;
		GLenum stencilZFailOpBack;
		GLenum stencilZPassOpBack;

		uint32 stencilReadMask;
		uint32 stencilWriteMask;
		uint32 stencilRef;

		GLDepthStencilStateValue() {}

		GLDepthStencilStateValue(EForceInit)
		{
			bEnableDepthTest = false;

			bEnableStencilTest = false;
			bWriteDepth = false;
			bUseSeparateStencilOp = false;
			bUseSeparateStencilFun = false;

			depthFun = GL_LESS;

			stencilFunc = stencilFunBack = GL_ALWAYS;
			stencilFailOp = stencilFailOpBack = GL_KEEP;
			stencilZFailOp = stencilZFailOpBack = GL_KEEP;
			stencilZPassOp = stencilZPassOpBack = GL_KEEP;

			stencilReadMask = -1;
			stencilWriteMask = -1;
			stencilRef = -1;

		}
	};

	class OpenGLDepthStencilState : public TRefcountResource< RHIDepthStencilState >
	{
	public:
		OpenGLDepthStencilState(DepthStencilStateInitializer const& initializer);
		GLDepthStencilStateValue mStateValue;
	};


	struct GLTargetBlendValue
	{
		ColorWriteMask writeMask;

		GLenum srcColor;
		GLenum srcAlpha;
		GLenum destColor;
		GLenum destAlpha;
		GLenum op;
		GLenum opAlpha;

		bool bEnable;
		bool bSeparateBlend;

		GLTargetBlendValue() {}
		GLTargetBlendValue(EForceInit)
		{
			bEnable = false;
			bSeparateBlend = false;
			srcColor = GL_ONE;
			srcAlpha = GL_ONE;
			destColor = GL_ZERO;
			destAlpha = GL_ZERO;
			writeMask = CWM_RGBA;
			op = GL_FUNC_ADD;
			opAlpha = GL_FUNC_ADD;
		}
	};

	struct GLBlendStateValue
	{
		bool bEnableAlphaToCoverage;
		bool bEnableIndependent;
		GLTargetBlendValue targetValues[MaxBlendStateTargetCount];
		GLBlendStateValue() {}
		GLBlendStateValue(EForceInit)
		{
			bEnableAlphaToCoverage = false;
			bEnableIndependent = false;
			for (int i = 0; i < MaxBlendStateTargetCount; ++i)
			{
				targetValues[i] = GLTargetBlendValue(ForceInit);
			}
		}
	};

	class OpenGLBlendState : public TRefcountResource< RHIBlendState >
	{
	public:
		OpenGLBlendState(BlendStateInitializer const& initializer);
		GLBlendStateValue mStateValue;
	};


	struct RMPVertexArrayObject
	{
		static void Create(GLuint& handle) { glGenVertexArrays(1, &handle); }
		static void Destroy(GLuint& handle) { glDeleteVertexArrays(1, &handle); }
	};

	class OpenGLInputLayout : public TRefcountResource< RHIInputLayout >
	{
	public:
		OpenGLInputLayout( InputLayoutDesc const& desc );

		void bindAttrib(InputStreamState const& state);
		void bindAttribUP(InputStreamState const& state);
		void unbindAttrib(int numInputStream);
	
		void bindPointer();
		void bindPointer(InputStreamState const& state);
		void bindPointerUP(InputStreamState const& state);
		void unbindPointer();

		GLuint bindVertexArray(InputStreamState const& state, bool bBindPointer = false);

		struct Element
		{
			GLenum componentType;
			uint32 stride;
			uint16 instanceStepRate;
			uint8  offset;
			uint8  streamIndex;
			uint8  attribute;
			uint8  componentNum;
			bool   bNormalized;
			bool   bInstanceData;	
		};

		typedef TOpenGLObject< RMPVertexArrayObject > VertexArrayObject;
		std::unordered_map< InputStreamState, VertexArrayObject, MemberFuncHasher > mVAOMap;
		TArray< Element > mElements;
		uint32 mAttributeMask;

		virtual void releaseResource() override;

	};

	struct PrimitiveDebugInfo
	{
		int32 numVertices;
		int32 numIndex;
		int32 numElement;
	};

	class OpenGLShader;
	class OpenGLShaderProgram;
	class ShaderProgram;

	template< class TRHIResource >
	struct TOpengGLTypeTraits {};

	template<> struct TOpengGLTypeTraits< RHITexture1D > { typedef OpenGLTexture1D CastType; };
	template<> struct TOpengGLTypeTraits< RHITexture2D > { typedef OpenGLTexture2D CastType; };
	template<> struct TOpengGLTypeTraits< RHITexture3D > { typedef OpenGLTexture3D CastType; };
	template<> struct TOpengGLTypeTraits< RHITextureCube > { typedef OpenGLTextureCube CastType; };
	template<> struct TOpengGLTypeTraits< RHITexture2DArray > { typedef OpenGLTexture2DArray CastType; };
	template<> struct TOpengGLTypeTraits< RHIBuffer > { typedef OpenGLBuffer CastType; };
	template<> struct TOpengGLTypeTraits< RHIInputLayout > { typedef OpenGLInputLayout CastType; };
	template<> struct TOpengGLTypeTraits< RHISamplerState > { typedef OpenGLSamplerState CastType; };
	template<> struct TOpengGLTypeTraits< RHIBlendState > { typedef OpenGLBlendState CastType; };
	template<> struct TOpengGLTypeTraits< RHIRasterizerState > { typedef OpenGLRasterizerState CastType; };
	template<> struct TOpengGLTypeTraits< RHIDepthStencilState > { typedef OpenGLDepthStencilState CastType; };
	template<> struct TOpengGLTypeTraits< RHIShaderProgram > { typedef OpenGLShaderProgram CastType; };
	template<> struct TOpengGLTypeTraits< RHIShader > { typedef OpenGLShader CastType; };
	template<> struct TOpengGLTypeTraits< RHIFrameBuffer > { typedef OpenGLFrameBuffer CastType; };

	struct OpenGLCast
	{
		template< class TRHIResource >
		static auto To(TRHIResource* resource)
		{
			return static_cast<TOpengGLTypeTraits< TRHIResource >::CastType*>(resource);
		}
		template< class TRHIResource >
		static auto To(TRHIResource const* resource)
		{
			return static_cast<TOpengGLTypeTraits< TRHIResource >::CastType const*>(resource);
		}
		template< class TRHIResource >
		static auto& To(TRHIResource& resource)
		{
			return static_cast<TOpengGLTypeTraits< TRHIResource >::CastType&>(resource);
		}

		template< class TRHIResource >
		static auto& To(TRHIResource const& resource)
		{
			return static_cast<TOpengGLTypeTraits< TRHIResource >::CastType const&>(resource);
		}

		template < class TRHIResource >
		static auto To(TRefCountPtr<TRHIResource>& ptr) { return To(ptr.get()); }

		template < class TRHIResource >
		static auto To(TRefCountPtr<TRHIResource> const& ptr) { return To(ptr.get()); }

		static auto To(OpenGLFrameBuffer* buffer) { return buffer; }

		template < class TRHIResource >
		static GLuint GetHandle(TRHIResource& RHIObject)
		{
			return OpenGLCast::To(RHIObject).getHandle();
		}

		template < class TRHIResource >
		static GLuint GetHandle(TRHIResource const& RHIObject)
		{
			return OpenGLCast::To(RHIObject).getHandle();
		}

		template < class TRHIResource >
		static GLuint GetHandle(TRefCountPtr<TRHIResource>& refPtr)
		{
			return OpenGLCast::To(refPtr)->getHandle();
		}
		template < class TRHIResource >
		static GLuint GetHandle(TRefCountPtr<TRHIResource> const& refPtr)
		{
			return OpenGLCast::To(refPtr)->getHandle();
		}
	};

	template< class T >
	struct TOpenGLScopedBind
	{
		TOpenGLScopedBind(T& obj)
			:mObj(obj)
		{
			OpenGLCast::To(&mObj)->bind();
		}

		~TOpenGLScopedBind()
		{
			OpenGLCast::To(&mObj)->unbind();
		}

		T& mObj;
	};



	template< class T >
	struct TOpenGLScopedBind< T* >
	{
		TOpenGLScopedBind(T* obj)
			:mObj(obj)
		{
			assert(mObj);
			OpenGLCast::To(mObj)->bind();
		}

		~TOpenGLScopedBind()
		{
			OpenGLCast::To(mObj)->unbind();
		}

		T* mObj;
	};

	template< class T >
	struct TOpenGLScopedBind< TRefCountPtr< T > >
	{
		TOpenGLScopedBind(TRefCountPtr< T >& obj)
			:mObj(obj)
		{
			assert(mObj);
			OpenGLCast::To(mObj.get())->bind();
		}

		~TOpenGLScopedBind()
		{
			OpenGLCast::To(mObj.get())->unbind();
		}

		TRefCountPtr< T >& mObj;
	};

#define GL_SCOPED_BIND_OBJECT( obj )\
	Render::TOpenGLScopedBind< std::remove_reference< decltype(obj) >::type >  ANONYMOUS_VARIABLE( BoundObject )( obj );



}

#endif // GLCommon_h__
