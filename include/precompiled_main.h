/** 
 * greebo: The main precompiled header file, as included by the different 
 * projects and modules. By setting the correct preprocessor definitions 
 * the various precompiled_xxxxx.h files are included in the chain.
 */
#pragma once

#ifdef DR_PRECOMPILED_BOOST
	// Include common boost headers that don't have link dependencies
	#include "precompiled_boost.h"
#endif

#ifdef DR_PRECOMPILED_GTKMM
	// Include GTKmm
	#include <gtkmm.h>
#endif

#ifdef DR_PRECOMPILED_INTERFACES
	// Add DarkRadiant interfaces
	#include "precompiled_interfaces.h"
#endif

#ifdef DR_PRECOMPILED_MATH
	// Add DarkRadiant interfaces
	#include "precompiled_interfaces.h"
#endif
