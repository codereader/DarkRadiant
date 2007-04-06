#ifndef RESPONSEEDITOR_H_
#define RESPONSEEDITOR_H_

#include "ClassEditor.h"

namespace ui {

class ResponseEditor :
	public ClassEditor
{

public:
	/** greebo: Constructor creates all the widgets
	 */
	ResponseEditor();
	
	/** greebo: Operator cast to widget to pack this page into
	 * 			a notebook tab or other parent widget.
	 */
	virtual operator GtkWidget*();

private:
	/** greebo: Gets called when the response selection gets changed 
	 */
	virtual void selectionChanged();
	
	void openContextMenu(GtkTreeView* view);
	void removeItem(GtkTreeView* view);

	/** greebo: Creates all the widgets
	 */
	void populatePage();
};

} // namespace ui

#endif /*RESPONSEEDITOR_H_*/
