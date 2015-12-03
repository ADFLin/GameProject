#ifndef GateServer_h__
#define GateServer_h__

#define GATE_SERVER_IP_DEFAULT "140.112.233.54"
#define GATE_SERVER_TCP_PORT 

#include "GameNet.h"
#include "GameWorker.h"

class GateClient
{

};

class GateServerWorker : public NetWorker
{
public:

	void procGameSvList( IComPacket* cp )
	{

	}
};

#endif // GateServer_h__