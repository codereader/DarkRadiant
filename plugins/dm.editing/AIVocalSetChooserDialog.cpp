#include "AIVocalSetChooserDialog.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "ieclass.h"
#include "isound.h"

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
		const char* const WINDOW_TITLE = N_("Choose AI Vocal Set");

		// ListStore columns
		enum
		{
			NAME_COLUMN,
			NUM_COLUMNS,
		};

		// Widgets
		enum
		{
			WIDGET_VOCALSETVIEW,
			WIDGET_OKBUTTON,
			WIDGET_DESCRIPTION,
		};
	}

AIVocalSetChooserDialog::AIVocalSetChooserDialog() :
	gtkutil::BlockingTransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow()),
	_setStore(gtk_list_store_new(NUM_COLUMNS, G_TYPE_STRING)),
	_result(RESULT_CANCEL)
{
	if (module::GlobalModuleRegistry().moduleExists(MODULE_SOUNDMANAGER))
	{
		_preview = AIVocalSetPreviewPtr(new AIVocalSetPreview);
	}

	_widgets[WIDGET_VOCALSETVIEW] = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_setStore));

	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);

	GtkWindow* mainWindow = GlobalMainFrame().getTopLevelWindow();

	GdkRectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(mainWindow);
	gtk_window_set_default_size(
		GTK_WINDOW(getWindow()), gint(rect.width * 0.5f), gint(rect.height * 0.6f)
	);

	GtkTreeView* setsView = GTK_TREE_VIEW(_widgets[WIDGET_VOCALSETVIEW]);
	GtkWidget* hbx = gtk_hbox_new(FALSE, 6);

	gtk_tree_view_set_headers_visible(setsView, FALSE);

	_setSelection = gtk_tree_view_get_selection(setsView);
	g_signal_connect(G_OBJECT(_setSelection), "changed",
					 G_CALLBACK(onSetSelectionChanged), this);
	
	// Head Name column
	gtk_tree_view_append_column(setsView, gtkutil::TextColumn("", 0));

	// Left: the treeview
	gtk_box_pack_start(GTK_BOX(hbx), gtkutil::ScrolledFrame(GTK_WIDGET(setsView)), TRUE, TRUE, 0);

	// Right: the description
	GtkWidget* vbox2 = gtk_vbox_new(FALSE, 3);
	GtkWidget* descPanel = createDescriptionPanel();
	gtk_box_pack_start(GTK_BOX(vbox2), descPanel, TRUE, TRUE, 0);
	gtk_widget_set_size_request(descPanel, static_cast<gint>(rect.width*0.2f), -1);

	// Right: the preview control panel
	if (_preview != NULL)
	{
		gtk_box_pack_start(GTK_BOX(vbox2), _preview->getWidget(), FALSE, FALSE, 0);
	}

	gtk_box_pack_start(GTK_BOX(hbx), vbox2, FALSE, FALSE, 0);

	// Topmost: the tree plus description
	gtk_box_pack_start(GTK_BOX(vbox), hbx, TRUE, TRUE, 0);
	// Bottom: the button panel
	gtk_box_pack_start(GTK_BOX(vbox), createButtonPanel(), FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(getWindow()), vbox);

	// Check if the liststore is populated
	findAvailableSets();

	// Load the found sets into the GtkListStore
	populateSetStore();
}

AIVocalSetChooserDialog::Result AIVocalSetChooserDialog::getResult()
{
	return _result;
}

void AIVocalSetChooserDialog::setSelectedVocalSet(const std::string& setName)
{
	_selectedSet = setName;

	if (_selectedSet.empty())
	{
		gtk_tree_selection_unselect_all(_setSelection);
		return;
	}

	// Lookup the model path in the treemodel
	gtkutil::TreeModel::SelectionFinder finder(_selectedSet, NAME_COLUMN);
	
	GtkTreeView* setView = GTK_TREE_VIEW(_widgets[WIDGET_VOCALSETVIEW]);

	GtkTreeModel* model = gtk_tree_view_get_model(setView);
	gtk_tree_model_foreach(model, gtkutil::TreeModel::SelectionFinder::forEach, &finder);
	
	// Get the found TreePath (may be NULL)
	GtkTreePath* path = finder.getPath();

	if (path != NULL)
	{
		// Expand the treeview to display the target row
		gtk_tree_view_expand_to_path(setView, path);
		// Highlight the target row
		gtk_tree_view_set_cursor(setView, path, NULL, false);
		// Make the selected row visible 
		gtk_tree_view_scroll_to_cell(setView, path, NULL, true, 0.3f, 0.0f);
	}
	else
	{
		gtk_tree_selection_unselect_all(_setSelection);
	}
}

std::string AIVocalSetChooserDialog::getSelectedVocalSet()
{
	return _selectedSet;
}

GtkWidget* AIVocalSetChooserDialog::createButtonPanel()
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

GtkWidget* AIVocalSetChooserDialog::createDescriptionPanel()
{
	// Create a GtkTextView
	GtkWidget* textView = gtk_text_view_new();
	_widgets[WIDGET_DESCRIPTION] = textView;

	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textView), GTK_WRAP_WORD);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(textView), FALSE);

	return gtkutil::ScrolledFrame(textView);	
}

void AIVocalSetChooserDialog::onCancel(GtkWidget* widget, 
									AIVocalSetChooserDialog* self) 
{
	self->_selectedSet = "";
	self->_result = RESULT_CANCEL;

	self->destroy();
}

void AIVocalSetChooserDialog::onOK(GtkWidget* widget, AIVocalSetChooserDialog* self) 
{
	self->_result = RESULT_OK;

	// Done, just destroy the window
	self->destroy();
}

void AIVocalSetChooserDialog::onSetSelectionChanged(GtkTreeSelection* sel,
												   AIVocalSetChooserDialog* self)
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
		self->_selectedSet = gtkutil::TreeModel::getString(model, &iter, NAME_COLUMN); 

		// Lookup the IEntityClass instance
		IEntityClassPtr eclass = GlobalEntityClassManager().findClass(self->_selectedSet);	

		if (eclass != NULL)
		{
			// Update the preview pane
			if (self->_preview != NULL)
			{
				self->_preview->setVocalSetEclass(eclass);
			}

			// Update the usage panel
			GtkTextView* textView = GTK_TEXT_VIEW(self->_widgets[WIDGET_DESCRIPTION]);
			GtkTextBuffer* buf = gtk_text_view_get_buffer(textView);

			// Create the concatenated usage string
			// TODO: move this algorithm to IEntityClass?
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
		self->_selectedSet = "";

		if (self->_preview != NULL)
		{
			self->_preview->setVocalSetEclass(IEntityClassPtr());
		}

		gtk_widget_set_sensitive(self->_widgets[WIDGET_OKBUTTON], FALSE);
		gtk_widget_set_sensitive(self->_widgets[WIDGET_DESCRIPTION], FALSE);
	}
}

void AIVocalSetChooserDialog::populateSetStore()
{
	// Clear the head list to be safe
	gtk_list_store_clear(_setStore);

	for (SetList::const_iterator i = _availableSets.begin(); i != _availableSets.end(); ++i)
	{
		// Add the entity to the list
		GtkTreeIter iter;
		gtk_list_store_append(_setStore, &iter);
		gtk_list_store_set(_setStore, &iter, 
						   NAME_COLUMN, i->c_str(),
						   -1);
	}
}

namespace
{

class VocalSetEClassFinder :
	public EntityClassVisitor
{
	AIVocalSetChooserDialog::SetList& _list;

public:
	VocalSetEClassFinder(AIVocalSetChooserDialog::SetList& list) :
		_list(list)
	{}

	void visit(IEntityClassPtr eclass)
	{
		if (eclass->getAttribute("editor_vocal_set").value == "1")
		{
			_list.insert(eclass->getName());
		}
	}
};

} // namespace

void AIVocalSetChooserDialog::findAvailableSets()
{
	if (!_availableSets.empty())
	{
		return;
	}

	// Instantiate a finder class and traverse all eclasses
	VocalSetEClassFinder visitor(_availableSets);
	GlobalEntityClassManager().forEach(visitor);
}

// Init static class member
AIVocalSetChooserDialog::SetList AIVocalSetChooserDialog::_availableSets;

} // namespace ui
