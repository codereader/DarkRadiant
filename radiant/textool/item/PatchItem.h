#ifndef PATCHITEM_H_
#define PATCHITEM_H_

#include "textool/TexToolItem.h"

class Patch;

namespace selection {
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
};
	
	} // namespace TexTool
} // namespace selection

#endif /*PATCHITEM_H_*/
