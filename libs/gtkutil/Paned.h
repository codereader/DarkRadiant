#ifndef PANED_H_
#define PANED_H_

#include "gtk/gtkhpaned.h"
#include "gtk/gtkvpaned.h"
#include "ifc/Widget.h"
#include <boost/shared_ptr.hpp>

namespace gtkutil
{

/** greebo: Encapsulation to create a paned (horizontal or vertical) container 
 * 
 * Pass the two contained widgets to the class constructor 
 * and use the getWidget() method to retrieve the completed framed widget.
 * 
 * Optionally, create an empty Paned view and add the children later usin
 * setFirstChild() and setSecondChild().
 */
class Paned :
	public Widget
{
	// The paned container
	GtkWidget* _paned;

public:

	enum PaneType
	{
		Horizontal = 0,
		Vertical = 1,
	};

	// Constructor (widgets to be filled later on)
	Paned(PaneType type) :
		_paned(type == Horizontal ? gtk_hpaned_new() : gtk_vpaned_new())
	{}

	// Construct a Paned with the specified children
	Paned(PaneType type, GtkWidget* firstChild, GtkWidget* secondChild) :
		_paned(type == Horizontal ? gtk_hpaned_new() : gtk_vpaned_new())
	{
		// Pack in the children
		setFirstChild(firstChild, false);
		setSecondChild(secondChild, false);
	}

	// Sets the first child to this paned view
	void setFirstChild(GtkWidget* child, bool allowShrinkBelowRequisition = false)
	{
		assert(gtk_paned_get_child1(GTK_PANED(_paned)) == NULL);

		gtk_paned_pack1(GTK_PANED(_paned), child, TRUE, allowShrinkBelowRequisition ? TRUE : FALSE);
	}

	// Sets the second child to this paned view
	void setSecondChild(GtkWidget* child, bool allowShrinkBelowRequisition = false)
	{
		assert(gtk_paned_get_child2(GTK_PANED(_paned)) == NULL);

		gtk_paned_pack2(GTK_PANED(_paned), child, TRUE, allowShrinkBelowRequisition ? TRUE : FALSE);
	}
	
protected:

	// Widget implementation
	virtual GtkWidget* _getWidget() const
	{
		return _paned;
	}
};
typedef boost::shared_ptr<Paned> PanedPtr;
	
} // namespace gtkutil

#endif /*PANED_H_*/
