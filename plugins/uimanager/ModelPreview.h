#ifndef MODELPREVIEW_H_
#define MODELPREVIEW_H_

#include "ifiltermenu.h"
#include "imodel.h"
#include "imodelpreview.h"
#include "math/matrix.h"

#include <GL/glew.h>
#include <string>
#include <map>
#include "gtkutil/GLWidget.h"
#include <gtkmm/frame.h>

namespace Gtk { class ToggleToolButton; }

namespace ui
{
	
/** Preview widget for models and skins. This class encapsulates the GTK widgets 
 * to render a specified model and skin using OpenGL, and return a single GTK 
 * widget to be packed into the parent window. The model and skin is changed 
 * with setModel() and setSkin(). This class handles zooming and rotating the 
 * model itself.
 */

class ModelPreview :
	public IModelPreview,
	public Gtk::Frame
{
private:
	// GL widget
	gtkutil::GLWidget* _glWidget;
	
	// Toolbar buttons
	Gtk::ToggleToolButton* _drawBBox;
	
	// A small cache mapping model names to IModel objects to avoid
	// reloading the models on each selection or skin change.
	// This map is cleared on dialog closure so that the allocated
	// models get released when they're not needed anymore.
	typedef std::map<std::string, model::IModelPtr> ModelMap;
	ModelMap _modelCache;

	// Current model to display
	model::IModelPtr _model;

	// Name of last model, to detect changes in model which require camera
	// recalculation
	std::string _lastModel;

	// Current distance between camera and preview
	GLfloat _camDist;
	
	// Current rotation matrix
	Matrix4 _rotation;

	// The filters menu
	IFilterMenuPtr _filtersMenu;
	
private:

	// gtkmm callbacks
	bool callbackGLDraw(GdkEventExpose*);
	bool callbackGLMotion(GdkEventMotion*);
	bool callbackGLScroll(GdkEventScroll*);
	void callbackToggleBBox();
	
public:
	
	/** Construct a ModelPreview widget.
	 */
	ModelPreview();
	
	/** 
	 * Set the pixel size of the ModelPreview widget. The widget is always 
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

	/** 
	 * Set the widget to display the given model. If the model name is the 
	 * empty string, the widget will release the currently displayed model.
	 * 
	 * @param
	 * String name of the model to display.
	 */
	void setModel(const std::string& model);
	
	/**
	 * Set the skin to apply on the currently-displayed model.
	 * 
	 * @param
	 * Name of the skin to apply.
	 */
	void setSkin(const std::string& skin);

	Gtk::Widget* getWidget();
	
	/** 
	 * Get the model from the widget, in order to display properties about it.
	 */
	model::IModelPtr getModel() {
		return _model;	
	}

	// To be called on dialog shutdown - releases local model cache
	void clear();
};
typedef boost::shared_ptr<ModelPreview> ModelPreviewPtr;

} // namespace

#endif /*MODELPREVIEW_H_*/
