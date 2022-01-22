#pragma once

#include "iselection.h"
#include "render/Colour4b.h"

namespace selection
{

class ManipulatorBase :
	public ISceneManipulator
{
private:
	std::size_t _id;
public:
	ManipulatorBase() :
		_id(0)
	{}

	virtual ~ManipulatorBase() {}

	// By default, manipulators can operate on components too (Clipper can't)
	virtual bool supportsComponentManipulation() const override
	{
		return true;
	}

    virtual void onPreRender(const RenderSystemPtr& renderSystem, const VolumeTest& volume) override
    {}

	// No visual representation by default
	virtual void render(IRenderableCollector& collector, const VolumeTest& volume) override
	{}

    virtual void clearRenderables() override
    {}

public:
	std::size_t getId() const override
	{
		return _id;
	}

	void setId(std::size_t id) override
	{
		_id = id;
	}

	/* Static colours */
	static const Colour4b& COLOUR_X();
	static const Colour4b& COLOUR_Y();
	static const Colour4b& COLOUR_Z();

    static const Colour4b& COLOUR_SCREEN();
    static const Colour4b& COLOUR_SPHERE();
    static const Colour4b& COLOUR_SELECTED();
};

}

const Colour4b& colourSelected(const Colour4b& colour, bool selected);
