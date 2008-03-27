#ifndef BRUSHITEM_H_
#define BRUSHITEM_H_

#include "brush/Brush.h"
#include "textool/TexToolItem.h"

namespace textool {

class BrushItem :
	public TexToolItem
{
	// The patch this control is referring to
	Brush& _sourceBrush;

public:
	// Constructor, allocates all child FacItems
	BrushItem(Brush& sourceBrush); 

	/** greebo: Saves the undoMemento of this brush,
	 * 			so that the operation can be undone later.
	 */
	virtual void beginTransformation();

	/** greebo: Selects all faces if a child face is selected.
	 */
	virtual void selectRelated();

}; // class BrushItem
	
} // namespace TexTool

#endif /*BRUSHITEM_H_*/
