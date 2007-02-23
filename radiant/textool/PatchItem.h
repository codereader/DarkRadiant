#ifndef PATCHITEM_H_
#define PATCHITEM_H_

#include "TexToolItem.h"

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

	// Transforms this object if it's selected only and calls the
	// Patch::controlPointsChanged() routine afterwards.
	virtual void transformSelected(const Matrix4& matrix);

	/** greebo: Snaps all the patch texcoords to the grid.
	 * 			This calls the TexToolItem base method and
	 * 			calls Patch::controlPointsChanged() afterwards.
	 */
	virtual void snapSelectedToGrid(float grid);
};
	
	} // namespace TexTool
} // namespace selection

#endif /*PATCHITEM_H_*/
