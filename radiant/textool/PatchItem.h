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

	// Gets the AABB of this object in texture space
	virtual AABB getExtents();

	// ========== Renderable implementation ================
	
	// Renders this object's visual representation.
	virtual void render();

	// ========== Transformable implementation ================
	
	// Transforms this object with the given transformation matrix
	virtual void transform(const Matrix4& matrix);
	
	// Transforms this object if it's selected only and calls the
	// Patch::controlPointsChanged() routine afterwards.
	virtual void transformSelected(const Matrix4& matrix);

};
	
	} // namespace TexTool
} // namespace selection

#endif /*PATCHITEM_H_*/
