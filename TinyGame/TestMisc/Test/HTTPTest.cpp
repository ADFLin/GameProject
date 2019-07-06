
#include "MiscTestRegister.h"

#include "NetSocket.h"

class HttpRespose
{


};
typedef std::function< HttpRespose > HttpResposeDeletgate;

class HttpRequest
{



	



	std::string url;

	NetSocket    mSocket;




};


void HttpTest()
{


}

REGISTER_MISC_TEST("Http Test", HttpTest);