#ifndef _MAP_PREVIEW_CAMERA_H_
#define _MAP_PREVIEW_CAMERA_H_

#include "gtkutil/GLWidget.h"
#include "math/matrix.h"
#include <gtk/gtkwidget.h>
#include "igl.h"
#include "irender.h"
#include "inode.h"

#include "ui/menu/FiltersMenu.h"

typedef struct _GdkEventExpose GdkEventExpose;

namespace map {

class MapPreviewCamera
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
	
public:
	MapPreviewCamera();
	
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

private:

	void draw();

	// GTK Callbacks
	static void onExpose(GtkWidget*, GdkEventExpose*, MapPreviewCamera*);
	static void onMouseMotion(GtkWidget*, GdkEventMotion*, MapPreviewCamera*);
	static void onMouseScroll(GtkWidget*, GdkEventScroll*, MapPreviewCamera*);
};

} // namespace map

#endif /* _MAP_PREVIEW_CAMERA_H_ */
