#include "Audio/AudioDevice.h"

#include "XAudio2.h"
#include "X3DAudio.h"

#include "Platform/Windows/ComUtility.h"
#include "DataStructure/Array.h"
#include "DataStructure/CycleQueue.h"

struct VoiceDeleter
{
	template< class T >
	static void Destroy(T* ptr) { ptr->DestroyVoice(); }
};

class XAudio2Device;


class XAudio2Source : public AudioSource,
					  public IXAudio2VoiceCallback
{
public:
	bool doInitialize(XAudio2Device& device)
	{

	}

	virtual bool doInitialize(SoundInstance& instance) override;
	virtual void update(float deltaT) override;
	virtual void endPlay() override;
	virtual void pause() override;

	TComPtr<IXAudio2SourceVoice, VoiceDeleter > pSourceVoice;
	XAudio2Device* mDevice;

	bool commitStreamingData( bool bInit = false);
	// pos = INDEX_NONE, no data need commit;
	int64 mNextStreamSampleFrame = INDEX_NONE;

	//TCycleQueue< uint32 > mUsedSampleHandles;
	TArray< uint32 >      mUsedSampleHandles;

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

	bool initializePlatform();

	TComPtr<IXAudio2> pXAudio2;
	TComPtr<IXAudio2MasteringVoice, VoiceDeleter > pMasterVoice;

	virtual AudioSource* createSource() override;
};
