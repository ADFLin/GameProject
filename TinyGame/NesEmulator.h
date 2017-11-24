#pragma once
#ifndef NesEmulator_H_81C456EE_828C_409A_8999_A180FF580D60
#define NesEmulator_H_81C456EE_828C_409A_8999_A180FF580D60

namespace Nes
{
	class IIODevice
	{
	public:
		virtual ~IIODevice(){}
		virtual void setPixel(int x, int y, uint32 value) {};
		virtual void flush(){}
		virtual void scanInput( uint32 input[2] ){}
	};

	class IMechine
	{
	public:
		static IMechine* Create();

	public:
		virtual ~IMechine() {}
		virtual bool init() = 0;
		virtual void reset() = 0;
		virtual void run( int numCycle ) = 0;
		virtual void setIODevice(IIODevice& device) = 0;
		virtual bool loadRom(char const* path) = 0;
		virtual void release() = 0;

		
	};



}//namespace Nes

#endif // NesEmulator_H_81C456EE_828C_409A_8999_A180FF580D60