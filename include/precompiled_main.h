/** 
 * greebo: The main precompiled header file, as included by the different 
 * projects and modules. By setting the correct preprocessor definitions 
 * the various precompiled_xxxxx.h files are included in the chain.
 */
#pragma once

#ifdef DR_PRECOMPILED_WXWIDGETS
	#include <wx/wx.h>
	#include <wx/artprov.h>
#endif

#ifdef DR_PRECOMPILED_INTERFACES
	// Add DarkRadiant interfaces
	#include "precompiled_interfaces.h"
#endif

#ifdef DR_PRECOMPILED_RENDER_INTERFACES
	// Add render-specific interfaces
	#include "precompiled_render_interfaces.h"
#endif

#ifdef DR_PRECOMPILED_UI_INTERFACES
    // Add DarkRadiant UI interfaces
    #include "precompiled_ui_interfaces.h"
#endif

#ifdef DR_PRECOMPILED_MATH
	// Add DarkRadiant math libraries
	#include "precompiled_math.h"
#endif

#include <memory>
#include <functional>
