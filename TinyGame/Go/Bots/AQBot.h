#pragma once
#ifndef AQBot_H_A357B1B9_0793_4CDC_ADB8_DBB6E84F3C1C
#define AQBot_H_A357B1B9_0793_4CDC_ADB8_DBB6E84F3C1C

#include "GTPBotBase.h"

namespace Go
{
	class AQAppRun : public GTPLikeAppRun
	{
	public:
		static char const* InstallDir;

		bool buildPlayGame();

	};


	class AQBot : public TGTPLikeBot< AQAppRun >
	{
	public:
		bool initialize(IBotSetting* setting) override;
		bool isGPUBased() const override { return true; }
	};


}//namespace Go

#endif // AQBot_H_A357B1B9_0793_4CDC_ADB8_DBB6E84F3C1C