#ifndef IMODELPREVIEW_H_
#define IMODELPREVIEW_H_

#include <string>
#include <boost/shared_ptr.hpp>

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
 * Use the GlobalRadiant() module to retrieve a new IModelPreview instance.
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
	virtual void setSize(int size) = 0;

	/** 
	 * Initialise the GL preview. This clears the window and sets up the 
	 * initial matrices and lights.
	 * Call this before showing the parent container.
	 */
	virtual void initialisePreview() = 0;

	/** 
	 * Set the widget to display the given model. If the model name is the 
	 * empty string, the widget will release the currently displayed model.
	 * 
	 * @param
	 * String name of the model to display.
	 */
	virtual void setModel(const std::string& model) = 0;

	/**
	 * Set the skin to apply on the currently-displayed model.
	 * 
	 * @param
	 * Name of the skin to apply.
	 */
	virtual void setSkin(const std::string& skin) = 0;

	// Retrieve the widget to pack this element into a parent container
	virtual GtkWidget* getWidget() = 0;

	/** 
	 * Get the model from the widget, in order to display properties about it.
	 */
	virtual model::IModelPtr getModel() = 0;

	// To be called on dialog shutdown - releases local model cache
	virtual void clear() = 0;
};
typedef boost::shared_ptr<IModelPreview> IModelPreviewPtr;

} // namespace ui

#endif /* IMODELPREVIEW_H_ */
