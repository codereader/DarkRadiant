#ifndef CLASSEDITOR_H_
#define CLASSEDITOR_H_

#include "SREntity.h"

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkTreeSelection GtkTreeSelection;

namespace ui {

class ClassEditor
{
protected:
	GtkWidget* _pageVBox;
	
	GtkWidget* _list;
	GtkTreeSelection* _selection;
	
	// The entity object we're editing
	SREntityPtr _entity;

public:
	/** greebo: Operator cast to widget to pack this page into
	 * 			a notebook tab or other parent widget.
	 */
	virtual operator GtkWidget*() = 0;
	
	/** greebo: Sets the new entity (is called by the StimResponseEditor class)
	 */
	virtual void setEntity(SREntityPtr entity);

protected:
	// GTK Callbacks
	
};

} // namespace ui

#endif /*CLASSEDITOR_H_*/
