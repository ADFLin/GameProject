#include "Stage/TestStageHeader.h"

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

#if SYS_PLATFORM_WIN
		DISPLAY_DEVICE devices[4];
		for (int i = 0; i < ARRAY_SIZE(devices); ++i)
		{
			devices[i].cb = sizeof(DISPLAY_DEVICE);
		}

		if (EnumDisplayDevicesA(NULL, 0, devices, 0))
		{
			LogMsg("device name = %s", devices[0].DeviceString);
		}
#endif

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

	void onUpdate(GameTimeSpan deltaTime) override
	{
		BaseClass::onUpdate(deltaTime);
	}

	void onRender(float dFrame) override
	{
		Graphics2D& g = Global::GetGraphics2D();

		GPUStatus status;
		query->getGPUStatus(0, status);
		InlineString<256> str;
		str.format("GPU %s Usage = %u , temp = %d , memory : free = %u , usage = %u",
			gpuInfo.name.c_str(), status.usage, status.temperature, 
			status.freeMemory, status.totalMemory - status.freeMemory);
		g.drawText( 200 , 100 ,  str);
	}

	MsgReply onMouse(MouseMsg const& msg) override
	{
		return BaseClass::onMouse(msg);
	}

	MsgReply onKey(KeyMsg const& msg) override
	{
		if(msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			}
		}
		return BaseClass::onKey(msg);
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

REGISTER_STAGE_ENTRY("Device Info", DeviceInfoStage, EExecGroup::FeatureDev, "RHI");