#ifndef AI_HEADPROPERTYEDITOR_H_ 
#define AI_HEADPROPERTYEDITOR_H_

#include "ientityinspector.h"

namespace Gtk { class HBox; }

namespace ui
{

	namespace
	{
		const std::string DEF_HEAD_KEY = "def_head";
	}

class AIHeadPropertyEditor :
	public IPropertyEditor
{
private:
	// The top-level widget
	Gtk::HBox* _widget;

	Entity* _entity;

public:
	// Default constructor
	AIHeadPropertyEditor();

	Gtk::Widget& getWidget();

	AIHeadPropertyEditor(Entity* entity, 
		const std::string& key, const std::string& options);

	IPropertyEditorPtr createNew(Entity* entity, 
								const std::string& key,
								const std::string& options);

private:
	void onChooseButton();
};

} // namespace ui

#endif /* AI_HEADPROPERTYEDITOR_H_ */
