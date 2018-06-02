#include "TestStageHeader.h"

#include "WindowsHeader.h"
#include "Hardware/GPUDeviceQuery.h"

class DeviceInfoStage : public StageBase
{
	typedef StageBase BaseClass;
public:

	DeviceInfoStage() {}

	GPUDeviceQuery* query;
	GPUInfo gpuInfo;
	
	virtual bool onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		query = GPUDeviceQuery::Create();
		if( query == nullptr )
			return false;

		query->getGPUInfo(0, gpuInfo);

		::Global::GUI().cleanupWidget();
		restart();
		return true;
	}

	virtual void onEnd()
	{
		BaseClass::onEnd();
	}

	void restart() {}
	void tick() {}
	void updateFrame(int frame) {}

	virtual void onUpdate(long time)
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		for( int i = 0; i < frame; ++i )
			tick();

		updateFrame(frame);
	}

	void onRender(float dFrame)
	{
		Graphics2D& g = Global::getGraphics2D();

		GPUStatus status;
		query->getGPUStatus(0, status);
		FixString<256> str;
		g.drawText( 200 , 100 , str.format( "GPU %s Usage = %u , temp = %d , memory : free = %u , usage = %u", 
										   gpuInfo.name.c_str() , status.usage , status.temperature , status.freeMemory , status.totalMemory - status.freeMemory ) );
	}

	bool onMouse(MouseMsg const& msg)
	{
		if( !BaseClass::onMouse(msg) )
			return false;
		return true;
	}

	bool onKey(unsigned key, bool isDown)
	{
		if( !isDown )
			return false;
		switch( key )
		{
		case Keyboard::eR: restart(); break;
		}
		return false;
	}

	virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
	{
		switch( id )
		{
		default:
			break;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	}
protected:
};

REGISTER_STAGE("Device Info", DeviceInfoStage, EStageGroup::FeatureDev);