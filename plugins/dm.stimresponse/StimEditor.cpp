#include "StimEditor.h"

#include <gtk/gtk.h>
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TreeModel.h"

#include "SREntity.h"

namespace ui {

	namespace {
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

StimEditor::StimEditor() {
	populatePage();
}

StimEditor::operator GtkWidget*() {
	return _pageVBox;
}

void StimEditor::populatePage() {
	GtkWidget* srHBox = gtk_hbox_new(FALSE, 0);
	
	// Pack it into an alignment so that it is indented
	gtk_box_pack_start(GTK_BOX(_pageVBox), GTK_WIDGET(srHBox), TRUE, TRUE, 0);
	
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
	
	gtk_box_pack_start(GTK_BOX(srHBox), 
		gtkutil::ScrolledFrame(_list), FALSE, FALSE, 0);
	
	//gtk_box_pack_start(GTK_BOX(srHBox), createSRWidgets(), TRUE, TRUE, 6);
	
	// Response effects section
    /*gtk_box_pack_start(GTK_BOX(_pageVBox),
    				   gtkutil::LeftAlignedLabel(LABEL_RESPONSE_EFFECTS),
    				   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_pageVBox), 
					   gtkutil::LeftAlignment(createEffectWidgets(), 18, 1.0),
					   TRUE, TRUE, 0);*/
}

void StimEditor::removeItem(GtkTreeView* view) {
	
}

void StimEditor::openContextMenu(GtkTreeView* view) {
	
}

void StimEditor::selectionChanged() {
	
}

} // namespace ui
