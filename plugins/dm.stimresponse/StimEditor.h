#ifndef STIMEDITOR_H_
#define STIMEDITOR_H_

#include "ClassEditor.h"

namespace ui {

class StimEditor :
	public ClassEditor
{

public:
	/** greebo: Constructor creates all the widgets
	 */
	StimEditor();
	
	/** greebo: Operator cast to widget to pack this page into
	 * 			a notebook tab or other parent widget.
	 */
	virtual operator GtkWidget*();

private:
	/** greebo: Gets called when the stim selection gets changed 
	 */
	virtual void selectionChanged();

	void openContextMenu(GtkTreeView* view);
	void removeItem(GtkTreeView* view);

	/** greebo: Creates all the widgets
	 */
	void populatePage();
};

} // namespace ui

#endif /*STIMEDITOR_H_*/
