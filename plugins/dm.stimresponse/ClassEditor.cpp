#include "ClassEditor.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "gtkutil/TreeModel.h"

namespace ui {
	
	namespace {
		const unsigned int TREE_VIEW_WIDTH = 220;
		const unsigned int TREE_VIEW_HEIGHT = 200;
	}

ClassEditor::ClassEditor(StimTypes& stimTypes) :
	_stimTypes(stimTypes),
	_updatesDisabled(false)
{
	_pageVBox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(_pageVBox), 6);
	
	_list = gtk_tree_view_new();
	gtk_widget_set_size_request(_list, TREE_VIEW_WIDTH, TREE_VIEW_HEIGHT);
	
	_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_list));

	// Connect the signals to the callbacks
	g_signal_connect(G_OBJECT(_selection), "changed", 
					 G_CALLBACK(onSRSelectionChange), this);
	g_signal_connect(G_OBJECT(_list), "key-press-event", 
					 G_CALLBACK(onTreeViewKeyPress), this);
	g_signal_connect(G_OBJECT(_list), "button-release-event", 
					 G_CALLBACK(onTreeViewButtonRelease), this);
}

ClassEditor::operator GtkWidget*() {
	return _pageVBox;
}

void ClassEditor::setEntity(SREntityPtr entity) {
	_entity = entity; 
}

int ClassEditor::getIdFromSelection() {
	GtkTreeIter iter;
	GtkTreeModel* model;
	bool anythingSelected = gtk_tree_selection_get_selected(_selection, &model, &iter);
	
	if (anythingSelected && _entity != NULL) {
		return gtkutil::TreeModel::getInt(model, &iter, ID_COL);
	}
	else {
		return -1;
	}
}

// Static callbacks
void ClassEditor::onSRSelectionChange(GtkTreeSelection* treeView, ClassEditor* self) {
	self->selectionChanged();
}

gboolean ClassEditor::onTreeViewKeyPress(GtkTreeView* view, GdkEventKey* event, ClassEditor* self) {
	if (event->keyval == GDK_Delete) {
		self->removeItem(view);
		
		// Catch this keyevent, don't propagate
		return TRUE;
	}
	
	// Propagate further
	return FALSE;
}

gboolean ClassEditor::onTreeViewButtonRelease(GtkTreeView* view, GdkEventButton* ev, ClassEditor* self) {
	// Single click with RMB (==> open context menu)
	if (ev->button == 3) {
		self->openContextMenu(view);
	}
	
	return FALSE;
}

} // namespace ui
