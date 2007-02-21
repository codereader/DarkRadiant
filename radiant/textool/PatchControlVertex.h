#ifndef PATCHCONTROLVERTEX_H_
#define PATCHCONTROLVERTEX_H_

#include "TexToolItem.h"

namespace selection {
	namespace textool {

class PatchControlVertex :
	public TexToolItem
{
	// The list of children of this object
	TexToolItemVec _children;

public:
	// Adds the given TexToolItem as child of this object
	void addChild(TexToolItemPtr child);
	
	/** greebo: Returns the vector reference of this object's children.
	 */
	TexToolItemVec& getChildren();
	
	// Gets the AABB of this object in texture space
	AABB getExtents();

	// ========== Renderable implementation ================
	
	// Renders this object's visual representation.
	void render();

	// ========== Transformable implementation ================
	
	// Transforms this object with the given transformation matrix
	void transform(const Matrix4& transform);

	// ========== Selectable implementation ================
	
	/** greebo: Returns true if the object can be selected at the given coords.
	 */
	virtual bool testSelect(const float s, const float& t);
};
	
	} // namespace TexTool
} // namespace selection

#endif /*PATCHCONTROLVERTEX_H_*/
