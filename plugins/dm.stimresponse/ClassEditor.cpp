#include "ClassEditor.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "gtkutil/TreeModel.h"

namespace ui {
	
	namespace {
		const unsigned int TREE_VIEW_WIDTH = 220;
		const unsigned int TREE_VIEW_HEIGHT = 200;
		
		static void textCellDataFunc(GtkTreeViewColumn* treeColumn,
							 GtkCellRenderer* cell,
							 GtkTreeModel* treeModel,
							 GtkTreeIter* iter, 
							 gpointer data)
		{
			bool inherited = gtkutil::TreeModel::getBoolean(treeModel, iter, INHERIT_COL);
			
			g_object_set(G_OBJECT(cell), "sensitive", !inherited, NULL);
		}
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
					 
	// Add the columns to the treeview
	// ID number
	GtkTreeViewColumn* numCol = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(numCol, "#");
	GtkCellRenderer* numRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(numCol, numRenderer, FALSE);
	gtk_tree_view_column_set_attributes(numCol, numRenderer, 
										"text", INDEX_COL,
										NULL);
	gtk_tree_view_column_set_cell_data_func(numCol, numRenderer,
                                            textCellDataFunc,
                                            NULL, NULL);
	
	gtk_tree_view_append_column(GTK_TREE_VIEW(_list), numCol);
	
	// The S/R icon
	GtkTreeViewColumn* classCol = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(classCol, "S/R");
	GtkCellRenderer* pixbufRenderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(classCol, pixbufRenderer, FALSE);
	gtk_tree_view_column_set_attributes(classCol, pixbufRenderer, 
										"pixbuf", CLASS_COL,
										NULL);
	gtk_tree_view_column_set_cell_data_func(classCol, pixbufRenderer,
                                            textCellDataFunc,
                                            NULL, NULL);
	
	gtk_tree_view_append_column(GTK_TREE_VIEW(_list), classCol);
	
	// The Type
	GtkTreeViewColumn* typeCol = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(typeCol, "Type");
	
	GtkCellRenderer* typeIconRenderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(typeCol, typeIconRenderer, FALSE);
	
	GtkCellRenderer* typeTextRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(typeCol, typeTextRenderer, FALSE);
	
	gtk_tree_view_column_set_attributes(typeCol, typeTextRenderer, 
										"text", CAPTION_COL,
										NULL);
	gtk_tree_view_column_set_cell_data_func(typeCol, typeTextRenderer,
                                            textCellDataFunc,
                                            NULL, NULL);
	
	gtk_tree_view_column_set_attributes(typeCol, typeIconRenderer, 
										"pixbuf", ICON_COL,
										NULL);
	gtk_tree_view_column_set_cell_data_func(typeCol, typeIconRenderer,
                                            textCellDataFunc,
                                            NULL, NULL);
	
	gtk_tree_view_append_column(GTK_TREE_VIEW(_list), typeCol);
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

void ClassEditor::setProperty(const std::string& key, const std::string& value) {
	int id = getIdFromSelection();
	
	if (id > 0) {
		// Don't edit inherited stims/responses
		if (!_entity->get(id).inherited()) {
			_entity->setProperty(id, key, value);
		}
	}

	// Call the method of the child class to update the widgets
	update();
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

void ClassEditor::onEntryChanged(GtkEditable* editable, ClassEditor* self) {
	if (self->_updatesDisabled) return; // Callback loop guard
	
	self->entryChanged(editable);
}

void ClassEditor::onCheckboxToggle(GtkToggleButton* toggleButton, ClassEditor* self) {
	if (self->_updatesDisabled) return; // Callback loop guard
	
	self->checkBoxToggled(toggleButton);
}

void ClassEditor::onStimTypeSelect(GtkComboBox* widget, ClassEditor* self) {
	if (self->_updatesDisabled) return; // Callback loop guard
	
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(widget, &iter)) {
		// Load the stim name (e.g. "STIM_FIRE") directly from the liststore
		GtkTreeModel* model = gtk_combo_box_get_model(widget);
		std::string name = gtkutil::TreeModel::getString(model, &iter, 3); // 3 = StimTypes::NAME_COL
		
		// Write it to the entity
		self->setProperty("type", name);
	}
}

} // namespace ui
