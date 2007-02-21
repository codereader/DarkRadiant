#include "TexTool.h"

#include "ieventmanager.h"
#include "iregistry.h"

#include <gtk/gtk.h>
#include <GL/glew.h>

#include "texturelib.h"
#include "selectionlib.h"
#include "gtkutil/TransientWindow.h"
#include "gtkutil/FramedWidget.h"
#include "gtkutil/glwidget.h"
#include "gtkutil/GLWidgetSentry.h"
#include "mainframe.h"
#include "brush/Face.h"
#include "patch/Patch.h"
#include "winding.h"

#include "textool/Selectable.h"
#include "textool/Transformable.h"
#include "textool/PatchItem.h"

#include "selection/algorithm/Primitives.h"
#include "selection/algorithm/Shader.h"


namespace ui {
	
	namespace {
		const std::string WINDOW_TITLE = "Texture Tool";
		
		const std::string RKEY_ROOT = "user/ui/textures/texTool/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
	}

TexTool::TexTool() :
	_winding(NULL),
	_patch(NULL),
	_selectionInfo(GlobalSelectionSystem().getSelectionInfo())
{
	// Be sure to pass FALSE to the TransientWindow to prevent it from self-destruction
	_window = gtkutil::TransientWindow(WINDOW_TITLE, MainFrame_getWindow(), false);
	
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(_window), 12);
	gtk_window_set_type_hint(GTK_WINDOW(_window), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	g_signal_connect(G_OBJECT(_window), "delete-event", G_CALLBACK(onDelete), this);
	g_signal_connect(G_OBJECT(_window), "focus-in-event", G_CALLBACK(triggerRedraw), this);
	
	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connect(GTK_OBJECT(_window));
	
	populateWindow();
	
	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(_window));
	
	// Register self to the SelSystem to get notified upon selection changes.
	GlobalSelectionSystem().addObserver(this);
}

void TexTool::populateWindow() {
	// Create the GL widget
	_glWidget = glwidget_new(TRUE);
	GtkWidget* frame = gtkutil::FramedWidget(_glWidget);
		
	// Connect the events
	gtk_widget_set_events(_glWidget, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
	g_signal_connect(G_OBJECT(_glWidget), "expose-event", G_CALLBACK(onExpose), this);
	g_signal_connect(G_OBJECT(_glWidget), "focus-in-event", G_CALLBACK(triggerRedraw), this);
	g_signal_connect(G_OBJECT(_glWidget), "button-press-event", G_CALLBACK(onMouseDown), this);
	g_signal_connect(G_OBJECT(_glWidget), "button-release-event", G_CALLBACK(onMouseUp), this);
	
	// Make the GL widget accept the global shortcuts
	GlobalEventManager().connect(GTK_OBJECT(_glWidget));
	
	GtkWidget* vbox = gtk_vbox_new(true, 0);
	gtk_box_pack_start(GTK_BOX(vbox), frame, true, true, 0);
	
	gtk_container_add(GTK_CONTAINER(_window), vbox);
}

void TexTool::toggle() {
	// Pass the call to the utility methods that save/restore the window position
	if (GTK_WIDGET_VISIBLE(_window)) {
		gtkutil::TransientWindow::minimise(_window);
		gtk_widget_hide_all(_window);
	}
	else {
		// Update the relevant member variables like shader, etc.
		update();
		// First restore the window
		gtkutil::TransientWindow::restore(_window);
		// then apply the saved position
		_windowPosition.applyPosition();
		// Now show it
		gtk_widget_show_all(_window);
	}
}

void TexTool::shutdown() {
	// De-register this as selectionsystem observer
	GlobalSelectionSystem().removeObserver(this);
	
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	
	GlobalEventManager().disconnect(GTK_OBJECT(_glWidget));
	GlobalEventManager().disconnect(GTK_OBJECT(_window));
}

TexTool& TexTool::Instance() {
	static TexTool _instance;
	
	return _instance;
}

void TexTool::update() {
	std::string selectedShader = selection::algorithm::getShaderFromSelection();
	_shader = GlobalShaderSystem().getShaderForName(selectedShader);
	
	// Try to retrieve the single selected face (throws on failure)
	try {
		Face& face = selection::algorithm::getLastSelectedFace();
		
		// Retrieve the winding from the face
		_winding = &face.getWinding();
	}
	catch (selection::InvalidSelectionException i) {
		_winding = NULL;
	}
	
	try {
		Patch& found = selection::algorithm::getLastSelectedPatch();
		_patch = &found;
	}
	catch (selection::InvalidSelectionException i) {
		_patch = NULL;
	}
}

void TexTool::selectionChanged() {
	update();
	
	// Clear the list to remove all the previously allocated items
	_items.clear();
	
	// Does the selection use one single shader?
	if (_shader->getName() != "") {
		if (_selectionInfo.patchCount > 0) {
			// One single named shader, get the selection list
			PatchPtrVector patchList = selection::algorithm::getSelectedPatches();
			globalOutputStream() << patchList.size() << " patches selected!\n";
			
			for (std::size_t i = 0; i < patchList.size(); i++) {
				// Allocate a new PatchItem on the heap (shared_ptr)
				selection::textool::TexToolItemPtr patchItem(
					new selection::textool::PatchItem(*patchList[0])
				);
				
				// Add it to the list
				_items.push_back(patchItem);
			}
		}
	}
	
	draw();
}

void TexTool::draw() {
	// Redraw
	gtk_widget_queue_draw(_glWidget);
}

AABB& TexTool::getExtents() {
	_selAABB = AABB();
	
	// Check for valid winding
	if (_winding != NULL) {
		for (Winding::iterator i = _winding->begin(); i != _winding->end(); i++) {
			_selAABB.includePoint(Vector3(i->texcoord[0], i->texcoord[1], 0));
		}
	}
	
	// Check for valid winding
	if (_patch != NULL) {
		for (PatchControlIter i = _patch->begin(); i != _patch->end(); i++) {
			_selAABB.includePoint(Vector3(i->m_texcoord[0], i->m_texcoord[1], 0));
		}
	}
	
	return _selAABB;
}

Vector2 TexTool::getTextureCoords(const double& x, const double& y) {
	Vector2 result;
	
	if (_selAABB.isValid()) {
		Vector3 aabbTL = _texSpaceAABB.origin - _texSpaceAABB.extents;
		Vector3 aabbBR = _texSpaceAABB.origin + _texSpaceAABB.extents;
		
		Vector2 topLeft(aabbTL[0], aabbTL[1]);
		Vector2 bottomRight(aabbBR[0], aabbBR[1]);
		
		// Determine the texcoords by the according proportionality factors
		result[0] = topLeft[0] + x * (bottomRight[0]-topLeft[0]) / _windowDims[0];
		result[1] = topLeft[1] + y * (bottomRight[1]-topLeft[1]) / _windowDims[1];
	}
	
	return result;
}

void TexTool::drawUVCoords() {
	// Check for valid winding
	if (_winding != NULL) {
		
		// Draw the Line Loop polygon representing the polygon
		glBegin(GL_LINE_LOOP);
		glColor3f(1, 1, 1);		
		
		for (Winding::iterator i = _winding->begin(); i != _winding->end(); i++) {
			glVertex2f(i->texcoord[0], i->texcoord[1]);
		}
		
		glEnd();
		
		// Draw again, this time draw only the winding points
		glPointSize(5);
		glBegin(GL_POINTS);
		glColor3f(1, 1, 1);
		
		for (Winding::iterator i = _winding->begin(); i != _winding->end(); i++) {
			glVertex2f(i->texcoord[0], i->texcoord[1]);
		}
		
		glEnd();
	}
	
	// Check for valid patch
	if (_patch != NULL) {
		glPointSize(5);
		glBegin(GL_POINTS);
		glColor3f(1, 1, 1);
		
		for (PatchControlIter i = _patch->begin(); i != _patch->end(); i++) {
			glVertex2f(i->m_texcoord[0], i->m_texcoord[1]);
		}
		
		glEnd();
	}
}

gboolean TexTool::onExpose(GtkWidget* widget, GdkEventExpose* event, TexTool* self) {
	// Update the information about the current selection 
	self->update();
	
	// Activate the GL widget
	gtkutil::GLWidgetSentry sentry(self->_glWidget);
	
	// Store the window dimensions for later calculations
	self->_windowDims = Vector2(event->area.width, event->area.height);
	
	// Initialise the viewport
	glViewport(0, 0, event->area.width, event->area.height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	// Clear the window with the specified background colour
	glClearColor(0.1f, 0.1f, 0.1f, 0); 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	
	// Do nothing, if the shader name is empty
	std::string shaderName = self->_shader->getName(); 
	if (shaderName == "") {
		globalOutputStream() << "TexTool: no unqiue shader selected.\n";
		return false;
	}
	
	AABB& selAABB = self->getExtents(); 
	
	// Is there a valid selection?
	if (!selAABB.isValid()) {
		globalOutputStream() << "TexTool: no valid AABB.\n";
		return false;
	}
	
	// Calculate the window extents
	self->_texSpaceAABB = AABB(selAABB.origin, selAABB.extents * 1.5);
	
	// Get the upper left and lower right corner coordinates
	Vector3 orthoTopLeft = self->_texSpaceAABB.origin - self->_texSpaceAABB.extents;
	Vector3 orthoBottomRight = self->_texSpaceAABB.origin + self->_texSpaceAABB.extents;
	
	// Initialise the 2D projection matrix with: left, right, bottom, top, znear, zfar 
	glOrtho(orthoTopLeft[0], 	// left 
			orthoBottomRight[0], // right
			orthoBottomRight[1], // bottom 
			orthoTopLeft[1], 	// top 
			-1, 1);
	
	glColor3f(1, 1, 1);
	// Tell openGL to draw front and back of the polygons in textured mode
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	// Acquire the texture number of the active texture
	TexturePtr tex = self->_shader->getTexture();
	glBindTexture(GL_TEXTURE_2D, tex->texture_number);
	
	// Draw the background texture
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	
	glTexCoord2f(orthoTopLeft[0], orthoTopLeft[1]);
	glVertex2f(orthoTopLeft[0], orthoTopLeft[1]);	// Upper left
	
	glTexCoord2f(orthoBottomRight[0], orthoTopLeft[1]);
	glVertex2f(orthoBottomRight[0], orthoTopLeft[1]);	// Upper right
	
	glTexCoord2f(orthoBottomRight[0], orthoBottomRight[1]);
	glVertex2f(orthoBottomRight[0], orthoBottomRight[1]);	// Lower right
	
	glTexCoord2f(orthoTopLeft[0], orthoBottomRight[1]);
	glVertex2f(orthoTopLeft[0], orthoBottomRight[1]);	// Lower left
	
	glEnd();
	glDisable(GL_TEXTURE_2D);
	
	// Draw the u/v coordinates
	self->drawUVCoords();
	
	/*glColor3f(1, 1, 1);
	std::string topLeftStr = floatToStr(orthoTopLeft[0]) + "," + floatToStr(orthoTopLeft[1]);
	std::string bottomRightStr = floatToStr(orthoBottomRight[0]) + "," + floatToStr(orthoBottomRight[1]);
	
	glRasterPos2f(orthoTopLeft[0] + 0.5, orthoTopLeft[1] + 0.5);
	GlobalOpenGL().drawString(topLeftStr.c_str());
	
	glRasterPos2f(orthoBottomRight[0] - 0.2, orthoBottomRight[1] + 0.2);
	GlobalOpenGL().drawString(bottomRightStr.c_str());*/
	
	return false;
}

gboolean TexTool::triggerRedraw(GtkWidget* widget, GdkEventFocus* event, TexTool* self) {
	// Trigger a redraw
	self->draw();
	return false;
}

gboolean TexTool::onDelete(GtkWidget* widget, GdkEvent* event, TexTool* self) {
	// Toggle the visibility of the textool window
	self->toggle();
	
	// Don't propagate the delete event
	return true;
}
	
gboolean TexTool::onMouseUp(GtkWidget* widget, GdkEventButton* event, TexTool* self) {
	return false;
}

gboolean TexTool::onMouseDown(GtkWidget* widget, GdkEventButton* event, TexTool* self) {
	globalOutputStream() << "Coords: " << event->x << "," << event->y << "\n";
	Vector2 texCoords = self->getTextureCoords(event->x, event->y);
	globalOutputStream() << "TexSpace: " << texCoords[0] << ", " << texCoords[1] << "\n";
	return false;
}
	
} // namespace ui
