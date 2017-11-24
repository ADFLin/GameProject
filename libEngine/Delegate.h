#pragma once
#ifndef Delegate_H_358AC534_8460_4BE1_9572_AF7CF1181980
#define Delegate_H_358AC534_8460_4BE1_9572_AF7CF1181980

#define  USE_FAST_DELEGATE 1

#if USE_FAST_DELEGATE
#include "FastDelegate/FastDelegate.h"
#define DECLARE_DELEGATE( NAME , ... )\
	typedef fastdelegate::FastDelegate< __VA_ARGS__ > NAME;
#else
#include <functional>
#define DECLARE_DELEGATE( NAME , ... )\
	typedef std::function< __VA_ARGS__ > NAME;
#endif

#endif // Delegate_H_358AC534_8460_4BE1_9572_AF7CF1181980
