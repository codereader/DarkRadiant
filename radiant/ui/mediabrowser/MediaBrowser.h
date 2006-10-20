#ifndef MEDIABROWSER_H_
#define MEDIABROWSER_H_

#include "ui/common/TexturePreviewCombo.h"

#include <gtk/gtkwidget.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtktreeselection.h>

namespace ui
{

/** Media Browser page of the group dialog, which allows browsing of
 * individual textures by name and loading them into the texture window
 * or applying directly to map geometry.
 */

class MediaBrowser
{
	// Main widget
	GtkWidget* _widget;
	
	// Main tree store, view and selection
	GtkTreeStore* _treeStore;
	GtkWidget* _treeView;
	GtkTreeSelection* _selection;
	
	// Context menu widget and items
	GtkWidget* _popupMenu;
	GtkWidget* _loadInTexturesView;
	GtkWidget* _applyToSelection;
	
	// Texture preview combo (GL widget and info table)
	TexturePreviewCombo _preview;
	
private:

	/* GTK CALLBACKS */
	
	static gboolean _onExpose(GtkWidget*, GdkEventExpose*, MediaBrowser*);
	static bool _onRightClick(GtkWidget*, GdkEventButton*, MediaBrowser*);
	static void _onActivateLoadContained(GtkMenuItem*, MediaBrowser*);
	static void _onActivateApplyTexture(GtkMenuItem*, MediaBrowser*);
	static void _onSelectionChanged(GtkWidget*, MediaBrowser*);
	
	/* Tree selection query functions */
	
	bool isDirectorySelected(); // is a directory selected
	std::string getSelectedName(); // return name of selection
	
	/* Function to update status of menu items based on selection
	 */
	void updateAvailableMenuItems();
	
public:
	
	/** Return the singleton instance.
	 */
	static MediaBrowser& getInstance() {
		static MediaBrowser _instance;
		return _instance;
	}

	/** Return the main widget for packing into
	 * the groupdialog or other parent container.
	 */
	GtkWidget* getWidget() {
		gtk_widget_show_all(_widget);
		return _widget;
	}
	
	/** Constructor creates GTK widgets.
	 */
	MediaBrowser();
};

}

#endif /*MEDIABROWSER_H_*/
