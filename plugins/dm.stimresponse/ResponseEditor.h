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
	ResponseEditor(StimTypes& stimTypes);

	/** greebo: Sets the new entity (updates the treeviews)
	 */
	virtual void setEntity(SREntityPtr entity);

	/** greebo: Updates the widgets (e.g. after a selection change) 
	 */
	void update();

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
