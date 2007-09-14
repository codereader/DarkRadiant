#include "GroupDialog.h"

#include <gtk/gtk.h>
#include "iregistry.h"
#include "iradiant.h"
#include "ieventmanager.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include <iostream>

namespace ui {
	
	namespace {
		const std::string RKEY_ROOT = "user/ui/groupDialog/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
		
		const std::string WINDOW_TITLE = "Entity";
	}

GroupDialog::GroupDialog() :
	_dialog(NULL),
	_currentPage(0)
{}

void GroupDialog::construct(GtkWindow* parent) {
	// Be sure to pass FALSE to the PersistentTransientWindow to prevent it from self-destruction
	_dialog = gtkutil::PersistentTransientWindow(WINDOW_TITLE, parent, false);
	
	// Set the default border width in accordance to the HIG
	//gtk_container_set_border_width(GTK_CONTAINER(_dialog), 12);
	//gtk_window_set_type_hint(GTK_WINDOW(_dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	g_signal_connect(G_OBJECT(_dialog), "delete-event", G_CALLBACK(onDelete), this);
	
	// Create all the widgets and pack them into the window
	populateWindow();
	
	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connectDialogWindow(GTK_WINDOW(_dialog));
	
	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(_dialog));
	_windowPosition.applyPosition();
}

void GroupDialog::show() {
	// Pass the call to toggleWindow() if there is anything to do
	if (!GTK_WIDGET_VISIBLE(_dialog)) {
		toggleWindow();
	}
}

GtkWindow* GroupDialog::getWindow() {
	return GTK_WINDOW(_dialog);
}

void GroupDialog::populateWindow() {
	_notebook = gtk_notebook_new();
	gtk_container_add(GTK_CONTAINER(_dialog), _notebook);
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(_notebook), GTK_POS_TOP);

	g_signal_connect(G_OBJECT(_notebook), "switch-page", G_CALLBACK(onPageSwitch), this);
}

GtkWidget* GroupDialog::getPage() {
	return gtk_notebook_get_nth_page(
		GTK_NOTEBOOK(_notebook), 
		gtk_notebook_get_current_page(GTK_NOTEBOOK(_notebook))
	);
}

bool GroupDialog::visible() {
	return (_dialog != NULL && GTK_WIDGET_VISIBLE(_dialog));
}

void GroupDialog::setPage(const std::string& name) {
	for (unsigned int i = 0; i < _pages.size(); i++) {
		if (_pages[i].name == name) {
			// Check, if the page is already visible
			if (getPage() == _pages[i].page && visible()) {
				// Yes, toggle the whole group dialog
				toggleWindow();
			}
			else {
				// Make sure the dialog is shown as well
				if (!visible()) {
					toggleWindow();
				}
				// Name found in the list, activate the page
				setPage(_pages[i].page);
			}
			// Don't continue the loop, we've found the page
			break;
		}
	}
}

void GroupDialog::setPage(GtkWidget* page) {
	_currentPage = gtk_notebook_page_num(GTK_NOTEBOOK(_notebook), page);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(_notebook), gint(_currentPage));
}

GroupDialog& GroupDialog::Instance() {
	static GroupDialog _instance;
	return _instance;
}

void GroupDialog::toggle() {
	Instance().toggleWindow();
}

void GroupDialog::toggleWindow() {
	// Do nothing, if the dialog is not yet constructed
	if (_dialog == NULL) {
		return;
	}
	
	// Pass the call to the utility methods that save/restore the window position
	if (GTK_WIDGET_VISIBLE(_dialog)) {
		// Save the window position, to make sure
		_windowPosition.readPosition();
		gtk_widget_hide(_dialog);
	}
	else {
		// Restore the position
		_windowPosition.applyPosition();
		// Now show the dialog window again
		gtk_widget_show(_dialog);
		// Unset the focus widget for this window to avoid the cursor 
		// from jumping into any entry fields
		gtk_window_set_focus(GTK_WINDOW(_dialog), NULL);
	}
}

void GroupDialog::shutdown() {
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	
	gtk_widget_hide(_dialog);
	
	GlobalEventManager().disconnectDialogWindow(GTK_WINDOW(_dialog));
}

GtkWidget* GroupDialog::addPage(const std::string& name,
								const std::string& tabLabel, 
								const std::string& tabIcon, 
								GtkWidget* page, const std::string& windowLabel) 
{
	// Make sure the notebook is visible before adding pages
	gtk_widget_show(_notebook);
	
	// Create the icon GtkImage and tab label
	GtkWidget* icon = gtk_image_new_from_pixbuf(GlobalRadiant().getLocalPixbuf(tabIcon));
	GtkWidget* label = gtk_label_new(tabLabel.c_str());

	// Pack into an hbox to create the title widget	
	GtkWidget* titleWidget = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(titleWidget), icon, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(titleWidget), label, FALSE, FALSE, 0);
	gtk_widget_show_all(titleWidget);
	
	// Show the child page before adding it to the notebook (GTK recommendation)
	gtk_widget_show(page);
	
	// Create the notebook page
	GtkWidget* notebookPage = gtk_notebook_get_nth_page(
		GTK_NOTEBOOK(_notebook), 
		gtk_notebook_insert_page(GTK_NOTEBOOK(_notebook), page, titleWidget, -1)
	);
	
	// Add this page to the local list
	Page newPage;
	newPage.name = name;
	newPage.page = notebookPage;
	newPage.title = windowLabel;
	
	_pages.push_back(newPage);

	return notebookPage;
}

void GroupDialog::updatePageTitle(unsigned int pageNumber) {
	if (pageNumber < _pages.size()) {
		gtk_window_set_title(GTK_WINDOW(_dialog), _pages[pageNumber].title.c_str());
	}
}

gboolean GroupDialog::onDelete(GtkWidget* widget, GdkEvent* event, GroupDialog* self) {
	// Toggle the visibility of the inspector window
	self->toggle();
	
	// Don't propagate the delete event
	return TRUE;
}

gboolean GroupDialog::onPageSwitch(GtkWidget* notebook, GtkWidget* page, guint pageNumber, GroupDialog* self) {
	self->updatePageTitle(pageNumber);
	return FALSE;
}

} // namespace ui
