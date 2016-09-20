#pragma once

#include <wx/event.h>
#include "ientityinspector.h"

namespace ui
{

namespace
{
	const std::string DEF_VOCAL_SET_KEY = "def_vocal_set";
}

class AIVocalSetPropertyEditor :
	public IPropertyEditor,
	public IPropertyEditorDialog,
	public wxEvtHandler
{
private:
	// The top-level widget
	wxPanel* _widget;

	Entity* _entity;

public:
	// Default constructor
	AIVocalSetPropertyEditor();

	~AIVocalSetPropertyEditor();

	AIVocalSetPropertyEditor(wxWindow* parent, Entity* entity,
		const std::string& key, const std::string& options);

	wxPanel* getWidget() override;
	void updateFromEntity() override;

	IPropertyEditorPtr createNew(wxWindow* parent, Entity* entity,
								 const std::string& key,
								 const std::string& options) override;

	std::string runDialog(Entity* entity, const std::string& key);

private:
	void onChooseButton(wxCommandEvent& ev);
};

} // namespace ui
