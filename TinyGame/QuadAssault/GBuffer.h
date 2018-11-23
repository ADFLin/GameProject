#ifndef GBuffer_h__
#define GBuffer_h__

#include "Base.h"
#include "Dependence.h"

#include "RHI/RHICommon.h"

#include <iostream>

class GBuffer
{
public:
	GBuffer();

	~GBuffer();

	bool create( int w , int h );
	void bind();

	void unbind();

	Render::RHITexture2DRef& getTexture( int idx ){ return mTexBuffers[idx]; }

	enum BufferType
	{
		eBASE_COLOR ,
		eNORMAL  ,
		eLIGHTING ,
		NUM_BUFFER_TYPE ,
	};

	Render::RHITexture2DRef mTexBuffers[ NUM_BUFFER_TYPE ];

	GLuint mFBO;
	GLuint mRBODepth;
};

#endif // GBuffer_h__
