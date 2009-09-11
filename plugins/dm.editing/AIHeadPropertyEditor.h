#ifndef AI_HEADPROPERTYEDITOR_H_ 
#define AI_HEADPROPERTYEDITOR_H_

#include "ientityinspector.h"

namespace ui
{

class AIHeadPropertyEditor :
	public IPropertyEditor
{
	// The top-level widget
	GtkWidget* _widget;

protected:
	// gtkutil::Widget impl.
	virtual GtkWidget* _getWidget() const
	{
		return _widget;
	}

public:
	// Default constructor
	AIHeadPropertyEditor();

	AIHeadPropertyEditor(Entity* entity, 
		const std::string& key, const std::string& options);

	IPropertyEditorPtr createNew(Entity* entity, 
								const std::string& key,
								const std::string& options);
};

} // namespace ui

#endif /* AI_HEADPROPERTYEDITOR_H_ */
