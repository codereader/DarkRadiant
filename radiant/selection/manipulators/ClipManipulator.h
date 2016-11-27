#pragma once

#include "debugging/debugging.h"
#include "../Manipulator.h"

namespace selection
{

class ClipManipulator : 
	public ManipulatorBase
{
public:
	Component* getActiveComponent()
	{
		ERROR_MESSAGE("clipper is not manipulatable");
		return nullptr;
	}

	Type getType() const
	{
		return Clip;
	}

	void setSelected(bool select)
	{}

	bool isSelected() const
	{
		return false;
	}
};

}
