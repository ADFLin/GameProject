#ifndef CmtDeviceFactory_h__
#define CmtDeviceFactory_h__

#include "CmtBase.h"
#include "CmtDevice.h"

namespace Chromatron
{
	class DeviceFactory
	{
	public:
		static Device*     create( DeviceId id , Dir dir , Color color );
		static void        destroy( Device* dc );
		static DeviceInfo& getInfo( DeviceId id );
	};

}//namespace Chromatron

#endif // CmtDeviceFactory_h__
