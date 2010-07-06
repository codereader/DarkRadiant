#ifndef AI_VOCAL_SET_PROPERTYEDITOR_H_ 
#define AI_VOCAL_SET_PROPERTYEDITOR_H_

#include "ientityinspector.h"
#include <set>

namespace ui
{

	namespace
	{
		const std::string DEF_VOCAL_SET_KEY = "def_vocal_set";
	}

class AIVocalSetPropertyEditor :
	public IPropertyEditor
{
private:
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
	AIVocalSetPropertyEditor();

	AIVocalSetPropertyEditor(Entity* entity, 
		const std::string& key, const std::string& options);

	IPropertyEditorPtr createNew(Entity* entity, 
								 const std::string& key,
								 const std::string& options);

private:
	static void onChooseButton(GtkWidget* button, AIVocalSetPropertyEditor* self);
};

} // namespace ui

#endif /* AI_VOCAL_SET_PROPERTYEDITOR_H_ */
