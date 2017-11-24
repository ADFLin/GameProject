#include "XAudio2.h"
#include "X3DAudio.h"

#include "Audio/AudioDevice.h"

#define CHECK_RETRUN( EXPR , RT_VALUE )\
	if( HRESULT hr = EXPR < 0 )\
		return RT_VALUE;

struct ReleaseDeleter
{
	template< class T >
	static void Destroy(T* ptr) { ptr->Release(); }
};

template< class T, class Deleter = ReleaseDeleter >
class TComPtr
{
public:

	TComPtr()
	{
		mPtr = nullptr;
	}

	~TComPtr()
	{
		if( mPtr )
		{
			Deleter::Destroy(mPtr);
		}
	}

	void reset()
	{
		if( mPtr )
		{
			Deleter::Destroy(mPtr);
			mPtr = nullptr;
		}
	}

	bool operator == (void* ptr) const { return mPtr == ptr; }
	bool operator != (void* ptr) const { return mPtr != ptr; }

	operator T*  () { return mPtr; }
	operator bool() const { return mPtr; }
	T*  operator->() { return mPtr; }
	T** operator& () { return &mPtr; }

	T* mPtr;
};


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

	TComPtr<IXAudio2SourceVoice, VoiceDeleter > pSourceVoice;
	XAudio2Device* mDevice;

	virtual bool doInitialize(SoundInstance& instance) override;
	virtual void update(float deltaT) override;
	virtual void stop() override;
	virtual void pause() override;


	void notifyPlayEnd();
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
