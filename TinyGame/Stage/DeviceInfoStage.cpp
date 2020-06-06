#include "TestStageHeader.h"

#include "WindowsHeader.h"
#include "Hardware/GPUDeviceQuery.h"

class DeviceInfoStage : public StageBase
{
	using BaseClass = StageBase;
public:

	DeviceInfoStage() {}

	GPUDeviceQuery* query;
	GPUInfo gpuInfo;
	
	bool onInit() override
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

	void onEnd() override
	{
		BaseClass::onEnd();
	}

	void restart() {}
	void tick() {}
	void updateFrame(int frame) {}

	void onUpdate(long time) override
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		for( int i = 0; i < frame; ++i )
			tick();

		updateFrame(frame);
	}

	void onRender(float dFrame) override
	{
		Graphics2D& g = Global::GetGraphics2D();

		GPUStatus status;
		query->getGPUStatus(0, status);
		FixString<256> str;
		str.format("GPU %s Usage = %u , temp = %d , memory : free = %u , usage = %u",
			gpuInfo.name.c_str(), status.usage, status.temperature, 
			status.freeMemory, status.totalMemory - status.freeMemory);
		g.drawText( 200 , 100 ,  str);
	}

	bool onMouse(MouseMsg const& msg) override
	{
		if( !BaseClass::onMouse(msg) )
			return false;
		return true;
	}

	bool onKey(KeyMsg const& msg) override
	{
		if( !msg.isDown())
			return false;
		switch(msg.getCode())
		{
		case EKeyCode::R: restart(); break;
		}
		return false;
	}

	bool onWidgetEvent(int event, int id, GWidget* ui) override
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