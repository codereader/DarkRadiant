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

GroupDialog::GroupDialog(GtkWindow* parent) 
: gtkutil::PersistentTransientWindow(WINDOW_TITLE, parent, true),
  _currentPage(0)
{
	// Create all the widgets and pack them into the window
	populateWindow();
	
	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connectDialogWindow(GTK_WINDOW(getWindow()));
	
	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(getWindow()));
	_windowPosition.applyPosition();
}

// Public static method to construct the instance
void GroupDialog::construct(GtkWindow* parent) {
	InstancePtr() = GroupDialogPtr(new GroupDialog(parent));
	GlobalRadiant().addEventListener(InstancePtr());
}

void GroupDialog::populateWindow() {
	_notebook = gtk_notebook_new();
	gtk_container_add(GTK_CONTAINER(getWindow()), _notebook);
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(_notebook), GTK_POS_TOP);

	g_signal_connect(G_OBJECT(_notebook), "switch-page", G_CALLBACK(onPageSwitch), this);
}

GtkWidget* GroupDialog::getPage() {
	return gtk_notebook_get_nth_page(
		GTK_NOTEBOOK(_notebook), 
		gtk_notebook_get_current_page(GTK_NOTEBOOK(_notebook))
	);
}

// Display the named page
void GroupDialog::setPage(const std::string& name) {
	for (unsigned int i = 0; i < _pages.size(); i++) {
		if (_pages[i].name == name) {

			// Found page. Set it to active if it is not already active.
			if (getPage() != _pages[i].page) {
				setPage(_pages[i].page);
			}

			// Show the window
			show();
			
			// Don't continue the loop, we've found the page
			break;
		}
	}
}

void GroupDialog::setPage(GtkWidget* page) {
	_currentPage = gtk_notebook_page_num(GTK_NOTEBOOK(_notebook), page);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(_notebook), gint(_currentPage));
}

GroupDialogPtr& GroupDialog::InstancePtr() {
	static GroupDialogPtr _instancePtr;
	return _instancePtr;
}

// Public method to retrieve the instance
GroupDialog& GroupDialog::Instance() {
	assert(InstancePtr() != NULL);
	return *InstancePtr();
}

// Public static method to toggle the window visibility
void GroupDialog::toggle() {
	if (Instance().isVisible())
		Instance().hide();
	else
		Instance().show();
}

// Pre-hide callback from TransientWindow
void GroupDialog::_preHide() {
	// Save the window position, to make sure
	_windowPosition.readPosition();
}

// Pre-show callback from TransientWindow
void GroupDialog::_preShow() {
	// Restore the position
	_windowPosition.applyPosition();
}

// Post-show callback from TransientWindow
void GroupDialog::_postShow() {
	// Unset the focus widget for this window to avoid the cursor 
	// from jumping into any entry fields
	gtk_window_set_focus(GTK_WINDOW(getWindow()), NULL);
}

void GroupDialog::onRadiantShutdown() {
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	
	GlobalEventManager().disconnectDialogWindow(GTK_WINDOW(getWindow()));

	// De-register from the RadiantEventSystem
	GlobalRadiant().removeEventListener(InstancePtr());

	// Call the PersistentTransientWindow::destroy chain
	destroy();
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
		gtk_window_set_title(GTK_WINDOW(getWindow()), _pages[pageNumber].title.c_str());
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
