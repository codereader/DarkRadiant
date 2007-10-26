#ifndef ECLASSTREE_H_
#define ECLASSTREE_H_

#include "iradiant.h"
#include "gtkutil/window/BlockingTransientWindow.h"
#include <boost/shared_ptr.hpp>

typedef struct _GtkListStore GtkListStore;
typedef struct _GtkTreeView GtkTreeView;
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkTreeStore GtkTreeStore;

namespace ui {

class EClassTree;
typedef boost::shared_ptr<EClassTree> EClassTreePtr;

class EClassTree :
	public gtkutil::BlockingTransientWindow
{
	// The EClass treeview widget and underlying liststore
	GtkTreeView* _eclassTreeView;
	GtkTreeStore* _eclassStore;
	
	// The treeview and liststore for the property pane
	GtkTreeView* _propertyTreeView;
	GtkListStore* _propertyStore;
	
	GtkWidget* _dialogVBox;
	
	// Private constructor, traverses the entity classes
	EClassTree();

public:
	// Shows the singleton class (static command target)
	static void showWindow();
	
private:
	// Constructs and adds all the dialog widgets
	void populateWindow();
	
	GtkWidget* createButtons(); 	// Dialog buttons
	GtkWidget* createEClassTreeView(); // EClass Tree
	
	// Static GTK callbacks
	static void onClose(GtkWidget* button, EClassTree* self);
};

} // namespace ui

#endif /*ECLASSTREE_H_*/

