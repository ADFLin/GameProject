#pragma once
#ifndef Editor_H_54F7990F_F10C_4F74_A15A_6823A63F68F4
#define Editor_H_54F7990F_F10C_4F74_A15A_6823A63F68F4

#include "CompilerConfig.h"

#ifndef EDITOR_EXPORT
#define EDITOR_EXPORT 0
#endif

#if EDITOR_EXPORT
#	define EDITOR_API DLLEXPORT
#else
#	define EDITOR_API DLLIMPORT
#endif


class IEditor
{
public:
	static EDITOR_API IEditor* Create();

	virtual ~IEditor(){}
	virtual void release() = 0;
	virtual bool initializeRender() = 0;
	virtual void update() = 0;
	virtual void render() = 0;
};

#endif // Editor_H_54F7990F_F10C_4F74_A15A_6823A63F68F4