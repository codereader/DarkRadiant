#ifndef _MAP_PREVIEW_WIDGET_H_
#define _MAP_PREVIEW_WIDGET_H_

#include "gtkutil/GLWidget.h"
#include "math/matrix.h"
#include <gtk/gtkwidget.h>
#include "igl.h"
#include "irender.h"
#include "inode.h"

#include "ui/menu/FiltersMenu.h"

typedef struct _GdkEventExpose GdkEventExpose;

namespace ui {

// Forward decl.
class MapPreviewFilterObserver;
typedef boost::shared_ptr<MapPreviewFilterObserver> MapPreviewFilterObserverPtr;

/**
 * greebo: This is a preview widget similar to the ui::ModelPreview class,
 * providing a GL render preview of a given root node.
 *
 * It comes with a Filters Menu included. Use the GtkWidget* operator
 * to retrieve the widget for packing into a parent container.
 *
 * Use the setRootNode() method to specify the subgraph to preview.
 */
class MapPreview
{
	// Top-level widget
	GtkWidget* _widget;
	
	// GL widget
	gtkutil::GLWidget _glWidget;
	
	// Current distance between camera and preview
	GLfloat _camDist;
	
	// Current rotation matrix
	Matrix4 _rotation;

	// Constructs the filters menu (provides a GtkWidget* operator)
	ui::FiltersMenu _filtersMenu;

	// The root node of the scene to be rendered
	scene::INodePtr _root;

	ShaderPtr _stateSelect1;
	ShaderPtr _stateSelect2;

	// The filter observer
	MapPreviewFilterObserverPtr _filterObserver;
	
public:
	MapPreview();

	~MapPreview();
	
	/** 
	 * Set the pixel size of the MapPreviewCam widget. The widget is always 
	 * square.
	 * 
	 * @param size
	 * The pixel size of the square widget.
	 */
	void setSize(int size);
	
	/** 
	 * Initialise the GL preview. This clears the window and sets up the 
	 * initial matrices and lights.
	 */
	void initialisePreview();	 

	/** Operator cast to GtkWidget*, for packing into the parent window.
	 */
	operator GtkWidget* () {
		return _widget;
	}

	// Get/set the map root to render
	void setRootNode(const scene::INodePtr& root);
	scene::INodePtr getRootNode();

	// Updates the view
	void draw();

	// Gets called by a local helper object on each FilterSystem change
	void onFiltersChanged();

private:
	// GTK Callbacks
	static void onExpose(GtkWidget*, GdkEventExpose*, MapPreview*);
	static void onMouseMotion(GtkWidget*, GdkEventMotion*, MapPreview*);
	static void onMouseScroll(GtkWidget*, GdkEventScroll*, MapPreview*);
};

} // namespace ui

#endif /* _MAP_PREVIEW_WIDGET_H_ */
