#ifndef BRUSHITEM_H_
#define BRUSHITEM_H_

#include "brush/Brush.h"
#include "TexToolItem.h"

namespace selection {
	namespace textool {

class BrushItem :
	public TexToolItem
{
	// The patch this control is referring to
	Brush& _sourceBrush;

public:
	BrushItem(Brush& sourceBrush); 

	// Gets the AABB of this object in texture space
	virtual AABB getExtents();

	// Renders this object's visual representation.
	virtual void render();

	// Transforms this object with the given transformation matrix
	virtual void transform(const Matrix4& matrix);

	/** greebo: Saves the current undo state.
	 */
	virtual void beginTransformation();
};
	
	} // namespace TexTool
} // namespace selection

#endif /*BRUSHITEM_H_*/
