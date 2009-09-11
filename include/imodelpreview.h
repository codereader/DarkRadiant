#ifndef IMODELPREVIEW_H_
#define IMODELPREVIEW_H_

#include <string>

typedef struct _GtkWidget GtkWidget;

namespace model {

class IModel;
typedef boost::shared_ptr<IModel> IModelPtr;

}

namespace ui
{

/**
 * The model preview can be packed into a GTK parent container
 * and provides methods to get/set the displayed IModel.
 *
 * Use the GlobalRadiant() module to retrieve a new IModelPreview class.
 */
class IModelPreview
{
public:
	/** 
	 * Set the pixel size of the IModelPreview widget. The widget is always 
	 * square. 
	 * 
	 * @param size
	 * The pixel size of the square widget.
	 */
	void setSize(int size);

	/** 
	 * Initialise the GL preview. This clears the window and sets up the 
	 * initial matrices and lights.
	 * Call this before showing the parent container.
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

	// Retrieve the widget to pack this element into a parent container
	GtkWidget* getWidget();

	/** 
	 * Get the model from the widget, in order to display properties about it.
	 */
	model::IModelPtr getModel();

	// To be called on dialog shutdown - releases local model cache
	void clear();
};
typedef boost::shared_ptr<IModelPreview> IModelPreviewPtr;

} // namespace ui

#endif /* IMODELPREVIEW_H_ */
