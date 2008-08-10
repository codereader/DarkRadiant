#ifndef LOGIC_EDITOR_H_
#define LOGIC_EDITOR_H_

#include <map>
#include <boost/shared_ptr.hpp>

typedef struct _GtkWidget GtkWidget;

namespace objectives {

/**
 * greebo: This class represents the UI elements needed to edit
 * a set of success- and failure logic strings.
 *
 * Use the getWidget() method to retrieve the GtkWidget for packing
 * the class into a parent container.
 */
class LogicEditor
{
	// The widget for packing the editor
	GtkWidget* _widget;

	// The indexed widget container
	std::map<int, GtkWidget*> _widgets;

public:
	/**
	 * The constructor will create the widgets.
	 */
	LogicEditor();

	// Returns the widget for packing into a parent container
	GtkWidget* getWidget();

	// Read accessors for the logic strings
	std::string getSuccessLogicStr();
	std::string getFailureLogicStr();

	// Write accessors for the logic strings
	void setSuccessLogicStr(const std::string& logicStr);
	void setFailureLogicStr(const std::string& logicStr);
};
typedef boost::shared_ptr<LogicEditor> LogicEditorPtr;

} // namespace objectives

#endif /* LOGIC_EDITOR_H_ */
