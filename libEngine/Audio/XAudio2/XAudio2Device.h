#include "XAudio2.h"
#include "X3DAudio.h"

#include "Audio/AudioDevice.h"

#include "Platform/Windows/ComUtility.h"

struct VoiceDeleter
{
	template< class T >
	static void Destroy(T* ptr) { ptr->DestroyVoice(); }
};

class XAudio2Device;


class XAudio2Source : public AudioSource
{
public:
	bool doInitialize(XAudio2Device& device)
	{

	}

	virtual bool doInitialize(SoundInstance& instance) override;
	virtual void update(float deltaT) override;
	virtual void endPlay() override;
	virtual void pause() override;


	void notifyBufferStart();
	void notifyBufferEnd();
	void noitfyStreamEnd();
	
	TComPtr<IXAudio2SourceVoice, VoiceDeleter > pSourceVoice;
	XAudio2Device* mDevice;


	bool commitStreamingData( bool bInit = false);
	// pos = -1 , no data need commit;
	int64 mNextStreamSampleFrame = -1;
	std::vector< uint32 > mUsedSampleHandles;
};

class XAudio2SourceCallback : public IXAudio2VoiceCallback
{
public:
	STDMETHOD_(void , OnVoiceProcessingPassStart)(UINT32 BytesRequired) override;
	STDMETHOD_(void , OnVoiceProcessingPassEnd)() override;
	STDMETHOD_(void , OnStreamEnd)() override;
	STDMETHOD_(void , OnBufferStart)(void* pBufferContext) override;
	STDMETHOD_(void , OnBufferEnd)(void* pBufferContext) override;
	STDMETHOD_(void , OnLoopEnd)(void* pBufferContext) override;
	STDMETHOD_(void , OnVoiceError)(void* pBufferContext, HRESULT Error) override;

};

class XAudio2Device : public AudioDevice
{
	typedef AudioDevice BaseClass;
public:
	virtual bool initialize() override;
	virtual void shutdown() override;

	bool bCoInitialized = false;

	TComPtr<IXAudio2> pXAudio2;
	TComPtr<IXAudio2MasteringVoice, VoiceDeleter > pMasterVoice;

	XAudio2SourceCallback sourceCallback;

	virtual AudioSource* createSource() override;
};
