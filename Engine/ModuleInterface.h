#pragma once
#ifndef ModuleInterface_H_1DBA3278_2821_437A_A721_E2AFF00CA0BF
#define ModuleInterface_H_1DBA3278_2821_437A_A721_E2AFF00CA0BF

class IModuleInterface
{
public:
	virtual ~IModuleInterface(){}
	virtual void startupModule(){}
	virtual void shutdownModule(){}
	virtual bool isGameModule() const { return false; }

	void release() { doRelease(); }
	virtual void  doRelease() = 0;
};

using CreateModuleFunc = IModuleInterface* (*)();


#define CREATE_MODULE CreateModule
#define CREATE_MODULE_STR MAKE_STRING(CREATE_MODULE)

#define EXPORT_MODULE( CLASS )\
	extern "C" DLLEXPORT IModuleInterface* CREATE_MODULE()\
	{\
		class ModuleImpl : public CLASS \
		{ \
		public: \
			void  doRelease() final { delete this;  } \
		}; \
		return new ModuleImpl; \
	}\

#define EXPORT_GAME_MODULE( CLASS )\
	EXPORT_MODULE(CLASS)

#endif // ModuleInterface_H_1DBA3278_2821_437A_A721_E2AFF00CA0BF