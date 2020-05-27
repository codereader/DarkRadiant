#pragma once

#include "debugging/debugging.h"
#include "ManipulatorBase.h"

namespace selection
{

class ClipManipulator : 
	public ManipulatorBase
{
public:
	Component* getActiveComponent() override
	{
		ERROR_MESSAGE("clipper is not manipulatable");
		return nullptr;
	}

	Type getType() const override
	{
		return Clip;
	}

	void setSelected(bool select) override
	{}

	bool isSelected() const override
	{
		return false;
	}

	bool supportsComponentManipulation() const override
	{
		return false;
	}
};

}
