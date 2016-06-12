#include "PreferenceItems.h"

#include <wx/stattext.h>
#include "registry/Widgets.h"

namespace ui
{

wxWindow* PreferenceLabel::createWidget(wxWindow* parent)
{
	_labelWidget = new wxStaticText(parent, wxID_ANY, "");
	_labelWidget->SetLabelMarkup(_label);

	return _labelWidget;
}

wxWindow* PreferenceEntry::createWidget(wxWindow* parent)
{
	_entryWidget = new wxTextCtrl(parent, wxID_ANY);

	return _entryWidget;
}

void PreferenceEntry::connectWidgetToKey(registry::Buffer& buffer, sigc::signal<void>& resetSignal)
{
	assert(_entryWidget && !_registryKey.empty());

	int minChars = static_cast<int>(std::max(buffer.get(_registryKey).size(), std::size_t(30)));
	_entryWidget->SetMinClientSize(wxSize(_entryWidget->GetCharWidth() * minChars, -1));

	// Connect the registry key to the newly created input field
	registry::bindWidgetToBufferedKey(_entryWidget, _registryKey, buffer, resetSignal);
}

}