#ifndef AI_VOCAL_SET_PROPERTYEDITOR_H_
#define AI_VOCAL_SET_PROPERTYEDITOR_H_

#include "ientityinspector.h"

namespace Gtk { class HBox; }

namespace ui
{

	namespace
	{
		const std::string DEF_VOCAL_SET_KEY = "def_vocal_set";
	}

class AIVocalSetPropertyEditor :
	public IPropertyEditor,
	public IPropertyEditorDialog
{
private:
	// The top-level widget
	Gtk::HBox* _widget;

	Entity* _entity;

public:
	// Default constructor
	AIVocalSetPropertyEditor();

	AIVocalSetPropertyEditor(Entity* entity,
		const std::string& key, const std::string& options);

	Gtk::Widget& getWidget();

	IPropertyEditorPtr createNew(Entity* entity,
								 const std::string& key,
								 const std::string& options);

	std::string runDialog(Entity* entity, const std::string& key);

private:
	void onChooseButton();
};

} // namespace ui

#endif /* AI_VOCAL_SET_PROPERTYEDITOR_H_ */
