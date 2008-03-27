#ifndef PATCHITEM_H_
#define PATCHITEM_H_

#include "textool/TexToolItem.h"

class Patch;

namespace textool {

class PatchItem :
	public TexToolItem
{
	// The patch this control is referring to
	Patch& _sourcePatch;

public:
	PatchItem(Patch& sourcePatch); 

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
	
} // namespace TexTool

#endif /*PATCHITEM_H_*/
