/** 
 * greebo: The main precompiled header file, as included by the different 
 * projects and modules. By setting the correct preprocessor definitions 
 * the various precompiled_xxxxx.h files are included in the chain.
 */
#pragma once

#ifdef _WIN32
//stgatilov: use winsock2.h instead of winsock.h (that's what ZeroMQ needs)
//must be defined before first include of windows.h
#include <winsock2.h>
#undef min
#undef max
#endif

#ifdef DR_PRECOMPILED_WXWIDGETS
	#include <wx/wx.h>
	#include <wx/artprov.h>
#endif

#ifdef DR_PRECOMPILED_INTERFACES
	// Add DarkRadiant interfaces
	#include "precompiled_interfaces.h"
#endif

#ifdef DR_PRECOMPILED_MATH
	// Add DarkRadiant math libraries
	#include "precompiled_math.h"
#endif

#include <memory>
#include <functional>
