#include "MapPreview.h"

#include "ifilter.h"
#include "gtkutil/GLWidget.h"
#include "gtkutil/GLWidgetSentry.h"
#include <gtk/gtk.h>

#include "camera/RadiantCameraView.h"
#include "camera/CamRenderer.h"

#include "MapPreviewView.h"

namespace ui {

// Helper class, which notifies the MapPreview about a filter change
class MapPreviewFilterObserver :
	public FilterSystem::Observer
{
	MapPreview& _owner;
public:
	MapPreviewFilterObserver(MapPreview& owner) :
		_owner(owner)
	{}

	void onFiltersChanged() {
		_owner.onFiltersChanged();
	}
};
typedef boost::shared_ptr<MapPreviewFilterObserver> MapPreviewFilterObserverPtr;

namespace {
	const float PREVIEW_FOV = 75.0f;
}

MapPreview::MapPreview() :
	_widget(gtk_frame_new(NULL)),
	_glWidget(true)
{
	// Main vbox - above is the GL widget, below is the toolbar
	GtkWidget* vbx = gtk_vbox_new(FALSE, 0);
	
	// Cast the GLWidget object to GtkWidget for further use
	GtkWidget* glWidget = _glWidget;
	gtk_box_pack_start(GTK_BOX(vbx), glWidget, TRUE, TRUE, 0);
	
	// Connect up the signals
	gtk_widget_set_events(glWidget, GDK_DESTROY | GDK_EXPOSURE_MASK | 
									GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | 
									GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
	g_signal_connect(G_OBJECT(glWidget), "expose-event", G_CALLBACK(onExpose), this);
	g_signal_connect(G_OBJECT(glWidget), "motion-notify-event", G_CALLBACK(onMouseMotion), this);
	g_signal_connect(G_OBJECT(glWidget), "scroll-event", G_CALLBACK(onMouseScroll), this);
	
	// The HBox containing the toolbar and the menubar
	GtkWidget* toolHBox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbx), toolHBox, FALSE, FALSE, 0);

	// Create the menu
	gtk_box_pack_start(GTK_BOX(toolHBox), _filtersMenu, TRUE, TRUE, 0);

	// Pack into a frame and return
	gtk_container_add(GTK_CONTAINER(_widget), vbx);

	// Add an observer to the FilterSystem to get notified about changes
	_filterObserver = MapPreviewFilterObserverPtr(new MapPreviewFilterObserver(*this));

	GlobalFilterSystem().addObserver(_filterObserver);
}

MapPreview::~MapPreview() {
	GlobalFilterSystem().removeObserver(_filterObserver);
}

// Set the size request for the widget

void MapPreview::setSize(int size) {
	gtk_widget_set_size_request(_glWidget, size, size);	
}

void MapPreview::setRootNode(const scene::INodePtr& root) {
	_root = root;

	if (_root != NULL) {
		// Trigger an initial update of the subgraph
		GlobalFilterSystem().updateSubgraph(_root);

		// Calculate camera distance so map is appropriately zoomed
		_camDist = -(_root->worldAABB().getRadius() * 2.0); 
	}
}

scene::INodePtr MapPreview::getRootNode() {
	return _root;
}

void MapPreview::onFiltersChanged() {
	// Sanity check
	if (_root == NULL) return;

	GlobalFilterSystem().updateSubgraph(_root);
	draw();
}

void MapPreview::initialisePreview() {

	// Grab the GL widget with sentry object
	gtkutil::GLWidgetSentry sentry(_glWidget);

	// Clear the window
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClearColor(0.0, 0.0, 0.0, 0);
	glClearDepth(100.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Set up the camera
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(PREVIEW_FOV, 1, 0.1, 10000);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
			
	// Set up the lights
	glEnable(GL_LIGHTING);

	glEnable(GL_LIGHT0);
	GLfloat l0Amb[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	GLfloat l0Dif[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat l0Pos[] = { 1.0f, 1.0f, 1.0f, 0.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT, l0Amb);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, l0Dif);
	glLightfv(GL_LIGHT0, GL_POSITION, l0Pos);
	
	glEnable(GL_LIGHT1);
	GLfloat l1Dif[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat l1Pos[] = { 0.0, 0.0, 1.0, 0.0 };
	glLightfv(GL_LIGHT1, GL_DIFFUSE, l1Dif);
	glLightfv(GL_LIGHT1, GL_POSITION, l1Pos);

	// Reset the rotation
	_rotation = Matrix4::getIdentity();
	
	// Calculate camera distance so model is appropriately zoomed
	_camDist = -(20 * 2.0); 

	_stateSelect1 = GlobalRenderSystem().capture("$CAM_HIGHLIGHT");
	_stateSelect2 = GlobalRenderSystem().capture("$CAM_OVERLAY");
}

void MapPreview::draw() {
	if (_root == NULL) return;

	// Create scoped sentry object to swap the GLWidget's buffers
	gtkutil::GLWidgetSentry sentry(_glWidget);

	// Set up the render
	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	AABB aabb(Vector3(0,0,0), Vector3(10,10,10));

	// Premultiply with the translations
	glLoadIdentity();
	glTranslatef(0, 0, _camDist); // camera translation
	glMultMatrixd(_rotation); // post multiply with rotations
	glRotatef(90, -1, 0, 0); // axis rotation (y-up (GL) -> z-up (model))

	// Render the map
	glEnable(GL_LIGHTING);
	glTranslatef(-aabb.origin.x(), -aabb.origin.y(), -aabb.origin.z()); // model translation

	// Start rendering
	unsigned int globalstate = 
		RENDER_DEPTHTEST|RENDER_COLOURWRITE|RENDER_DEPTHWRITE|
		RENDER_ALPHATEST|RENDER_BLEND|RENDER_CULLFACE|RENDER_COLOURARRAY|
		RENDER_OFFSETLINE|RENDER_POLYGONSMOOTH|RENDER_LINESMOOTH|
		RENDER_COLOURCHANGE;

	globalstate |=	RENDER_FILL | RENDER_LIGHTING | RENDER_TEXTURE_2D | 
					RENDER_SMOOTH | RENDER_SCALED | RENDER_BUMP | RENDER_PROGRAM | RENDER_SCREEN;

	CamRenderer renderer(globalstate, _stateSelect1, _stateSelect2, Vector3(0,0,_camDist));
	MapPreviewView view;

	// Save the new GL matrix for GL draw
	glGetDoublev(GL_MODELVIEW_MATRIX, view.modelView);
	glGetDoublev(GL_PROJECTION_MATRIX, view.projection);

	// Instantiate a new walker class
	RenderHighlighted renderHighlightWalker(renderer, view);

	// Small helper class to use scene::Graph::Walkers for direct traversal
	class RenderAdaptor :
		public scene::NodeVisitor
	{
		scene::Graph::Walker& _walker;
	public:
		RenderAdaptor(scene::Graph::Walker& walker) :
			_walker(walker)
		{}

		bool pre(const scene::INodePtr& node)
		{
			_walker.visit(node);
			return true;
		}
	} adaptor(renderHighlightWalker);

	// Submit renderables from this map root
	Node_traverseSubgraph(_root, adaptor);
	
	// Submit renderables directly attached to the ShaderCache
	GlobalRenderSystem().forEachRenderable(
		RenderHighlighted::RenderCaller(RenderHighlighted(renderer, view)));

	renderer.render(view.modelView, view.projection);
}

void MapPreview::onExpose(GtkWidget* widget, GdkEventExpose* ev, MapPreview* self) { 
	self->draw();
}

void MapPreview::onMouseMotion(GtkWidget* widget, GdkEventMotion* ev, MapPreview* self) {
	if (ev->state & GDK_BUTTON1_MASK) { // dragging with mouse button
		static gdouble _lastX = ev->x;
		static gdouble _lastY = ev->y;

		// Calculate the mouse delta as a vector in the XY plane, and store the
		// current position for the next event.
		Vector3 deltaPos(ev->x - _lastX,
						 _lastY - ev->y,
						 0);
		_lastX = ev->x;
		_lastY = ev->y;
		
		// Calculate the axis of rotation. This is the mouse vector crossed with the Z axis,
		// to give a rotation axis in the XY plane at right-angles to the mouse delta.
		static Vector3 _zAxis(0, 0, 1);
		Vector3 axisRot = deltaPos.crossProduct(_zAxis);
		
		// Grab the GL widget, and update the modelview matrix with the 
		// additional rotation
		if (gtkutil::GLWidget::makeCurrent(widget)) {

			// Premultiply the current modelview matrix with the rotation,
			// in order to achieve rotations in eye space rather than object
			// space. At this stage we are only calculating and storing the
			// matrix for the GLDraw callback to use.
			glLoadIdentity();
			glRotated(-2, axisRot.x(), axisRot.y(), axisRot.z());
			glMultMatrixd(self->_rotation);

			// Save the new GL matrix for GL draw
			glGetDoublev(GL_MODELVIEW_MATRIX, self->_rotation);

			gtk_widget_queue_draw(widget); // trigger the GLDraw method to draw the actual model
		}
	}
}

void MapPreview::onMouseScroll(GtkWidget* widget, GdkEventScroll* ev, MapPreview* self) {
	// Sanity check
	if (self->_root == NULL) return;

	float inc = static_cast<float>(self->_root->worldAABB().getRadius() * 0.1);

	if (ev->direction == GDK_SCROLL_UP)
		self->_camDist += inc;
	else if (ev->direction == GDK_SCROLL_DOWN)
		self->_camDist -= inc;
	gtk_widget_queue_draw(widget);
}

} // namespace ui
