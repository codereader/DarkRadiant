#ifndef MEDIABROWSER_H_
#define MEDIABROWSER_H_

#include "ui/common/TexturePreviewCombo.h"

#include <gtk/gtkwidget.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtkmenuitem.h>

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
	
	// Main tree store and view
	GtkTreeStore* _treeStore;
	GtkWidget* _treeView;
	
	// Context menu widget
	GtkWidget* _popupMenu;
	
	// Texture preview combo (GL widget and info table)
	TexturePreviewCombo _preview;
	
private:

	/* GTK CALLBACKS */
	
	static gboolean _onExpose(GtkWidget*, GdkEventExpose*, MediaBrowser*);
	static bool _onRightClick(GtkWidget*, GdkEventButton*, MediaBrowser*);
	static void _onActivateLoadContained(GtkMenuItem*, MediaBrowser*);
	
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
