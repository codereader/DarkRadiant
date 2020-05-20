#pragma once

#include "icommandsystem.h"

namespace ui
{

class CreateLayerDialog
{
public:
	/**
	 * Command target which adds a new layer to the currently loaded map.
	 * It will use the layer name as passed in the first argument, but will 
	 * keep displaying a TextEntry dialog for as long as the name is conflicting 
	 * with an existing one.
	 */
	static void CreateNewLayer(const cmd::ArgumentList& args);
};

}
