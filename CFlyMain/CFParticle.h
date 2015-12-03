#ifndef CFParticle_h__
#define CFParticle_h__

#include "CFBase.h"
#include "CFSceneNode.h"

namespace CFly
{


	class ParticleFactory
	{

	};
	class ParticleClass
	{
		ParticleFactory* factory;


	};



	class ParticleEmitter : public SceneNode
	{
	public:
		void setParticleClass( ParticleClass* pClass );





	};


}//namespace CFly
#endif // CFParticle_h__
