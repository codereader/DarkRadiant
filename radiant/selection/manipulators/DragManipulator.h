#pragma once

#include "ManipulatorBase.h"
#include "ManipulatorComponents.h"

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
};

}
