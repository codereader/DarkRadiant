#ifndef MEDIABROWSER_H_
#define MEDIABROWSER_H_

#include "ui/common/TexturePreviewCombo.h"

#include "gtkutil/menu/PopupMenu.h"

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
	
	// Context menu
	gtkutil::PopupMenu _popupMenu;
	
	// Texture preview combo (GL widget and info table)
	TexturePreviewCombo _preview;
	
	// false, if the tree is not yet initialised.
	bool _isPopulated;
	
private:

	/* gtkutil::PopupMenu callbacks */
	bool _testSingleTexSel();
	bool _testLoadInTexView();
	void _onApplyToSel();
	void _onLoadInTexView();
	
	/* GTK CALLBACKS */
	
	static gboolean _onExpose(GtkWidget*, GdkEventExpose*, MediaBrowser*);
	static void _onSelectionChanged(GtkWidget*, MediaBrowser*);
	
	/* Tree selection query functions */
	
	bool isDirectorySelected(); // is a directory selected
	std::string getSelectedName(); // return name of selection
	
	// Populates the treeview
	void populate();
	
public:
	
	/** Return the singleton instance.
	 */
	static MediaBrowser& getInstance();

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
	
	/** Set the given path as the current selection, highlighting it
	 * in the tree view.
	 * 
	 * @param selection
	 * The fullname of the item to select, or the empty string if there 
	 * should be no selection.
	 */
	void setSelection(const std::string& selection);
	
	/** greebo: Rebuilds the media tree from scratch (call this after
	 * 			a "RefreshShaders" command.
	 */
	void reloadMedia();

	/** 
	 * greebo: Static command target for toggling the mediabrowser tab in the groupdialog.
	 */
	static void toggle();
};

}

#endif /*MEDIABROWSER_H_*/
