#ifndef MEDIABROWSER_H_
#define MEDIABROWSER_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtktreestore.h>

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
	
	// Main tree store
	GtkTreeStore* _treeStore;
	
private:

	/* GTK CALLBACKS */
	
	static void _onExpose(GtkWidget*, GdkEventExpose*, MediaBrowser*);
	
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
