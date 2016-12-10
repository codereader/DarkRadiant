#pragma once

#include "ManipulatorBase.h"
#include "ManipulatorComponents.h"

#include "../SelectionTest.h"
#include "../SelectionPool.h"
#include "../BasicSelectable.h"

namespace selection
{

class DragManipulator : 
	public ManipulatorBase
{
	TranslateFree _freeResize;
	TranslateFree _freeDrag;
	ResizeTranslatable _resizeTranslatable;
	DragTranslatable _dragTranslatable;
	BasicSelectable _dragSelectable;
public:
	bool _selected;

	DragManipulator() : 
		_freeResize(_resizeTranslatable), 
		_freeDrag(_dragTranslatable), 
		_selected(false)
	{}

	Type getType() const override
	{
		return Drag;
	}

	Component* getActiveComponent() override;
	void testSelect(const render::View& view, const Matrix4& pivot2world) override;
	void setSelected(bool select) override;
	bool isSelected() const override;

private:
	void testSelectPrimitiveMode(const render::View& view, SelectionVolume& test, SelectionPool& selector);
	void testSelectGroupPartMode(const render::View& view, SelectionVolume& test, SelectionPool& selector);
	void testSelectEntityMode(const render::View& view, SelectionVolume& test, SelectionPool& selector);
	void testSelectComponentMode(const render::View& view, SelectionVolume& test, SelectionPool& selector);
};

}
