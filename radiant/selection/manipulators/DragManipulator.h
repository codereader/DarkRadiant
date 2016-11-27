#pragma once

#include "ManipulatorBase.h"
#include "../Manipulatables.h"

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

	Type getType() const
	{
		return Drag;
	}

	Component* getActiveComponent();
	void testSelect(const render::View& view, const Matrix4& pivot2world);
	void setSelected(bool select);
	bool isSelected() const;
};

}
