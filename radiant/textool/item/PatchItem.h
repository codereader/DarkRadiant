#pragma once

#include "textool/TexToolItem.h"

class IPatch;

namespace textool
{

class PatchItem :
	public TexToolItem
{
	// The patch this control is referring to
	IPatch& _sourcePatch;

public:
	PatchItem(IPatch& sourcePatch);

	// Renders this object's visual representation.
	virtual void render();

	/** greebo: Saves the undoMemento for later undo.
	 */
	virtual void beginTransformation();

	/** greebo: Calls Patch::controlPointsChanged()
	 */
	virtual void update();

	/** greebo: Selects all patch vertices if on of them is selected.
	 */
	virtual void selectRelated();
};

} // namespace
