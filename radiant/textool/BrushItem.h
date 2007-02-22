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
	// Constructor, allocates all child FacItems
	BrushItem(Brush& sourceBrush); 

}; // class BrushItem
	
	} // namespace TexTool
} // namespace selection

#endif /*BRUSHITEM_H_*/
