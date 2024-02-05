#ifndef Texture_h__
#define Texture_h__

#include "Base.h"
#include "Dependence.h"
#include "RHI/RHICommon.h"
#include "HashString.h"

class Texture
{
public:
	Texture();
	~Texture();

	Render::RHITexture2DRef resource;
	HashString fileName;	
};

#endif // Texture_h__
