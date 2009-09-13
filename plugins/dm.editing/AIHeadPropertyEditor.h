#ifndef AI_HEADPROPERTYEDITOR_H_ 
#define AI_HEADPROPERTYEDITOR_H_

#include "ientityinspector.h"
#include <set>

namespace ui
{

	namespace
	{
		const std::string DEF_HEAD_KEY = "def_head";
	}

class AIHeadPropertyEditor :
	public IPropertyEditor
{
	// The top-level widget
	GtkWidget* _widget;

	Entity* _entity;

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

private:
	static void onChooseButton(GtkWidget* button, AIHeadPropertyEditor* self);
};

} // namespace ui

#endif /* AI_HEADPROPERTYEDITOR_H_ */
