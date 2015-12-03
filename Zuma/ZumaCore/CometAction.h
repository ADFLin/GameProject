#ifndef ParticleAction_h__
#define ParticleAction_h__

#include "CometBase.h"

namespace Comet 
{
	class MoveAct : public Action
	{
	public:
		MoveAct( Real multiplier = Real(1.0) );
		//Action
		void preUpdate( Emitter& e , TimeType time );
		void update( Emitter& e , Particle& p , TimeType time );
	private:
		Real mMultiplier;
		Real mFactor;
	};

	class DeadAct : public Action
	{
	public:
		DeadAct(){}

		//Action 
		void update( Emitter& e , Particle& p , TimeType time );
	};

	class AgeAct : public Action
	{
	public:
		AgeAct( Real multiplier = Real(1.0) );

		//Action 
		void preUpdate( Emitter& e , TimeType time );
		void update( Emitter& e , Particle& p , TimeType time );
	private:
		Real     mMultiplier;
		TimeType mFactor;
	};

	class DecayAct : public Action
	{
	public:
		DecayAct( Real multiplier = Real(1.0) );
		virtual void preUpdate( Emitter& e , TimeType time );
		virtual void update( Emitter& e , Particle& p , TimeType time );
	private:
		Real mMultiplier;
		TimeType mFactor;
	};


	class DumpAct : public Action
	{
	public:

		void update( Emitter& e , Particle& p , TimeType time );

		Real mDump;
		Real mDumpping;
	};

}//namespace Comet 

#endif // ParticleAction_h__
