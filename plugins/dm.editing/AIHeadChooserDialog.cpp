#include "AIHeadChooserDialog.h"

#include "iradiant.h"
#include "ieclass.h"

#include <gtk/gtk.h>

#include "gtkutil/TextColumn.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/RightAlignment.h"

namespace ui
{

	namespace
	{
		const std::string WINDOW_TITLE("Choose AI Head");

		// ListStore columns
		enum
		{
			NAME_COLUMN,
			NUM_COLUMNS,
		};

		// Widgets
		enum
		{
			WIDGET_HEADVIEW,
			WIDGET_OKBUTTON,
			WIDGET_DESCRIPTION,
		};
	}

AIHeadChooserDialog::AIHeadChooserDialog() :
	gtkutil::BlockingTransientWindow(WINDOW_TITLE, GlobalRadiant().getMainWindow()),
	_headStore(gtk_list_store_new(NUM_COLUMNS, G_TYPE_STRING)),
	_result(RESULT_CANCEL)
{
	_widgets[WIDGET_HEADVIEW] = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_headStore));

	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);

	GtkWindow* mainWindow = GlobalRadiant().getMainWindow();

	GdkRectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(mainWindow);
	gtk_window_set_default_size(
		GTK_WINDOW(getWindow()), gint(rect.width * 0.7f), gint(rect.height * 0.6f)
	);

	// Allocate and setup the preview
	_preview = GlobalRadiant().createModelPreview();
	assert(_preview != NULL);

	_preview->setSize(gint(rect.width * 0.3f));

	GtkTreeView* headsView = GTK_TREE_VIEW(_widgets[WIDGET_HEADVIEW]);
	GtkWidget* hbx = gtk_hbox_new(FALSE, 6);

	gtk_tree_view_set_headers_visible(headsView, FALSE);

	_headSelection = gtk_tree_view_get_selection(headsView);
	g_signal_connect(G_OBJECT(_headSelection), "changed",
					 G_CALLBACK(onHeadSelectionChanged), this);
	
	// Head Name column
	gtk_tree_view_append_column(headsView, gtkutil::TextColumn("", 0));

	// Right: the treeview
	gtk_box_pack_start(GTK_BOX(hbx), gtkutil::ScrolledFrame(GTK_WIDGET(headsView)), TRUE, TRUE, 0);
	// Left: the preview and the description
	GtkWidget* vbox2 = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox2), _preview->getWidget(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), createDescriptionPanel(), FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(hbx), vbox2, FALSE, FALSE, 0);

	// Topmost: the tree plus preview
	gtk_box_pack_start(GTK_BOX(vbox), hbx, TRUE, TRUE, 0);
	// Bottom: the button panel
	gtk_box_pack_start(GTK_BOX(vbox), createButtonPanel(), FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(getWindow()), vbox);

	// Check if the liststore is populated
	findAvailableHeads();

	// Load the found heads into the GtkListStore
	populateHeadStore();
}

AIHeadChooserDialog::Result AIHeadChooserDialog::getResult()
{
	return _result;
}

void AIHeadChooserDialog::setSelectedHead(const std::string& headDef)
{
	_selectedHead = headDef;

	if (_selectedHead.empty())
	{
		gtk_tree_selection_unselect_all(_headSelection);
		return;
	}

	// Lookup the model path in the treemodel
	gtkutil::TreeModel::SelectionFinder finder(_selectedHead, NAME_COLUMN);
	
	GtkTreeView* headsView = GTK_TREE_VIEW(_widgets[WIDGET_HEADVIEW]);

	GtkTreeModel* model = gtk_tree_view_get_model(headsView);
	gtk_tree_model_foreach(model, gtkutil::TreeModel::SelectionFinder::forEach, &finder);
	
	// Get the found TreePath (may be NULL)
	GtkTreePath* path = finder.getPath();

	if (path != NULL)
	{
		// Expand the treeview to display the target row
		gtk_tree_view_expand_to_path(headsView, path);
		// Highlight the target row
		gtk_tree_view_set_cursor(headsView, path, NULL, false);
		// Make the selected row visible 
		gtk_tree_view_scroll_to_cell(headsView, path, NULL, true, 0.3f, 0.0f);
	}
	else
	{
		gtk_tree_selection_unselect_all(_headSelection);
	}
}

std::string AIHeadChooserDialog::getSelectedHead()
{
	return _selectedHead;
}

GtkWidget* AIHeadChooserDialog::createButtonPanel()
{
	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);

	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	_widgets[WIDGET_OKBUTTON] = gtk_button_new_from_stock(GTK_STOCK_OK);
	
	g_signal_connect(
		G_OBJECT(cancelButton), "clicked", G_CALLBACK(onCancel), this
	);
	g_signal_connect(
		G_OBJECT(_widgets[WIDGET_OKBUTTON]), "clicked", G_CALLBACK(onOK), this
	);

	gtk_box_pack_end(GTK_BOX(hbx), _widgets[WIDGET_OKBUTTON], TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, TRUE, TRUE, 0);

	return gtkutil::RightAlignment(hbx);
}

GtkWidget* AIHeadChooserDialog::createDescriptionPanel()
{
	// Create a GtkTextView
	GtkWidget* textView = gtk_text_view_new();
	_widgets[WIDGET_DESCRIPTION] = textView;

	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textView), GTK_WRAP_WORD);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(textView), FALSE);

	return gtkutil::ScrolledFrame(textView);	
}

void AIHeadChooserDialog::onCancel(GtkWidget* widget, 
									AIHeadChooserDialog* self) 
{
	self->_selectedHead = "";
	self->_result = RESULT_CANCEL;

	self->destroy();
}

void AIHeadChooserDialog::onOK(GtkWidget* widget, AIHeadChooserDialog* self) 
{
	self->_result = RESULT_OK;

	// Done, just destroy the window
	self->destroy();
}

void AIHeadChooserDialog::_preDestroy()
{
	// Clear the model preview cache
	_preview->clear();

	BlockingTransientWindow::_preDestroy();
}

void AIHeadChooserDialog::_postShow()
{
	// Initialise the GL widget after the widgets have been shown
	_preview->initialisePreview();

	BlockingTransientWindow::_postShow();
}

void AIHeadChooserDialog::onHeadSelectionChanged(GtkTreeSelection* sel,
												   AIHeadChooserDialog* self)
{
	// Prepare to check for a selection
	GtkTreeIter iter;
	GtkTreeModel* model;

	// Add button is enabled if there is a selection and it is not a folder.
	if (gtk_tree_selection_get_selected(sel, &model, &iter)) 
	{
		// Make the OK button active 
		gtk_widget_set_sensitive(self->_widgets[WIDGET_OKBUTTON], TRUE);
		gtk_widget_set_sensitive(self->_widgets[WIDGET_DESCRIPTION], TRUE);

		// Set the panel text with the usage information
		self->_selectedHead = gtkutil::TreeModel::getString(model, &iter, NAME_COLUMN); 

		// Lookup the IEntityClass instance
		IEntityClassPtr eclass = GlobalEntityClassManager().findClass(self->_selectedHead);	

		if (eclass != NULL)
		{
			self->_preview->setModel(eclass->getAttribute("model").value);
			self->_preview->setSkin(eclass->getAttribute("skin").value);

			// Update the usage panel
			GtkTextView* textView = GTK_TEXT_VIEW(self->_widgets[WIDGET_DESCRIPTION]);
			GtkTextBuffer* buf = gtk_text_view_get_buffer(textView);
			
			// Create the concatenated usage string
			std::string usage = "";
			EntityClassAttributeList usageAttrs = eclass->getAttributeList("editor_usage");
			for (EntityClassAttributeList::const_iterator i = usageAttrs.begin();
				 i != usageAttrs.end();
				 ++i)
			{
				// Add only explicit (non-inherited) usage strings
				if (!i->inherited) {
					if (!usage.empty())
						usage += std::string("\n") + i->value;
					else
						usage += i->value;
				}
			}
			
			gtk_text_buffer_set_text(buf, usage.c_str(), -1);
		}
	}
	else
	{
		self->_selectedHead = "";
		self->_preview->setModel("");

		gtk_widget_set_sensitive(self->_widgets[WIDGET_OKBUTTON], FALSE);
		gtk_widget_set_sensitive(self->_widgets[WIDGET_DESCRIPTION], FALSE);
	}
}

void AIHeadChooserDialog::populateHeadStore()
{
	// Clear the head list to be safe
	gtk_list_store_clear(_headStore);

	for (HeadList::const_iterator i = _availableHeads.begin(); i != _availableHeads.end(); ++i)
	{
		// Add the entity to the list
		GtkTreeIter iter;
		gtk_list_store_append(_headStore, &iter);
		gtk_list_store_set(_headStore, &iter, 
						   NAME_COLUMN, i->c_str(),
						   -1);
	}
}

namespace
{

class HeadEClassFinder :
	public EntityClassVisitor
{
	AIHeadChooserDialog::HeadList& _list;

public:
	HeadEClassFinder(AIHeadChooserDialog::HeadList& list) :
		_list(list)
	{}

	void visit(IEntityClassPtr eclass)
	{
		if (eclass->getAttribute("editor_head").value == "1")
		{
			_list.insert(eclass->getName());
		}
	}
};

} // namespace

void AIHeadChooserDialog::findAvailableHeads()
{
	if (!_availableHeads.empty())
	{
		return;
	}

	// Instantiate a finder class and traverse all eclasses
	HeadEClassFinder visitor(_availableHeads);
	GlobalEntityClassManager().forEach(visitor);
}

// Init static class member
AIHeadChooserDialog::HeadList AIHeadChooserDialog::_availableHeads;

} // namespace ui
