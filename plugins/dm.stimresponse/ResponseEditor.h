#ifndef RESPONSEEDITOR_H_
#define RESPONSEEDITOR_H_

#include "ClassEditor.h"

namespace ui {

class ResponseEditor :
	public ClassEditor
{
	struct ListButtons {
		GtkWidget* add;
		GtkWidget* remove;
	} _listButtons;
	
	struct ListContextMenu {
		GtkWidget* menu;
		GtkWidget* remove;
		GtkWidget* add;
	} _contextMenu;
	
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
	/** greebo: Adds a new default response to the entity
	 */
	void addResponse();

	// Widget creator helper
	GtkWidget* createListButtons();
	void createContextMenu();

	/** greebo: Gets called when the response selection gets changed 
	 */
	virtual void selectionChanged();
	
	void openContextMenu(GtkTreeView* view);
	void removeItem(GtkTreeView* view);

	/** greebo: Creates all the widgets
	 */
	void populatePage();
	
	// Context menu GTK callbacks
	static void onListContextMenuAdd(GtkWidget* w, ResponseEditor* self);
	static void onListContextMenuDelete(GtkWidget* w, ResponseEditor* self);
	
	static void onAddResponse(GtkWidget* button, ResponseEditor* self);
	static void onRemoveResponse(GtkWidget* button, ResponseEditor* self);
};

} // namespace ui

#endif /*RESPONSEEDITOR_H_*/
