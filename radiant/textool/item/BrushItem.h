#pragma once

#include "ibrush.h"
#include "textool/TexToolItem.h"

namespace textool
{

class BrushItem :
	public TexToolItem
{
	// The brush this control is referring to
	IBrush& _sourceBrush;

public:
	// Constructor, allocates all child FacItems
	BrushItem(IBrush& sourceBrush);

    // destructor
	virtual ~BrushItem() {}

	/** greebo: Saves the undoMemento of this brush,
	 * 			so that the operation can be undone later.
	 */
	virtual void beginTransformation();

	/** greebo: Selects all faces if a child face is selected.
	 */
	virtual void selectRelated();

}; // class BrushItem

} // namespace
