#pragma once

#include "ifiltermenu.h"
#include "imodel.h"
#include "imodelpreview.h"

#include <string>
#include <map>
#include "gtkutil/preview/RenderPreview.h"

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
	public gtkutil::RenderPreview
{
private:
	// Toolbar buttons
	Gtk::ToggleToolButton* _drawBBox;

	// The parent entity
	scene::INodePtr _entity;

	// Current model to display
	scene::INodePtr _modelNode;

	// Name of last model, to detect changes in model which require camera
	// recalculation
	std::string _lastModel;

	// The filters menu
	IFilterMenuPtr _filtersMenu;

private:
	// gtkmm callbacks
	void callbackToggleBBox();

public:

	/** Construct a ModelPreview widget.
	 */
	ModelPreview();

	// IModelPreview implementation, wrapping to base
	void setSize(int width, int height)
	{
		RenderPreview::setSize(width, height);
	}

	void initialisePreview()
	{
		RenderPreview::initialisePreview();
	}

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
	scene::INodePtr getModelNode()
	{
		return _modelNode;
	}

protected:
	// Creates parent entity etc.
	void setupSceneGraph();

	AABB getSceneBounds();

	bool onPreRender();
	void onPostRender();
};
typedef boost::shared_ptr<ModelPreview> ModelPreviewPtr;

} // namespace
