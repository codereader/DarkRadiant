#include "DefinitionView.h"

#include "i18n.h"
#include "ishaders.h"

#include "SourceView.h"

#include <wx/sizer.h>
#include <wx/stattext.h>

namespace wxutil
{

DefinitionView::DefinitionView(const std::string& title, wxWindow* parent) :
	DialogBase(title, parent),
	_view(nullptr)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	_panel = new wxPanel(this, wxID_ANY);
	_panel->SetSizer(new wxBoxSizer(wxVERTICAL));

	auto* table = new wxFlexGridSizer(2, 2, 6, 6);

	auto* nameLabel = new wxStaticText(_panel, wxID_ANY, _("Name:"));
	auto* materialFileLabel = new wxStaticText(_panel, wxID_ANY, _("Defined in:"));
	
	_declName = new wxStaticText(_panel, wxID_ANY, "");
	_declName->SetFont(_declName->GetFont().Bold());

	_filename = new wxStaticText(_panel, wxID_ANY, "");
	_filename->SetFont(_filename->GetFont().Bold());
	
	nameLabel->SetMinSize(wxSize(90, -1));
	materialFileLabel->SetMinSize(wxSize(90, -1));

	table->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL);
	table->Add(_declName, 0, wxALIGN_CENTER_VERTICAL);
	
	table->Add(materialFileLabel, 0, wxALIGN_CENTER_VERTICAL);
	table->Add(_filename, 0, wxALIGN_CENTER_VERTICAL);

	auto* defLabel = new wxStaticText(_panel, wxID_ANY, _("Definition:"));

	_panel->GetSizer()->Add(table, 0);
	_panel->GetSizer()->Add(defLabel, 0, wxTOP, 6);
	
	GetSizer()->Add(_panel, 1, wxEXPAND | wxALL, 12);
	GetSizer()->Add(CreateStdDialogButtonSizer(wxOK), 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12);
}

void DefinitionView::setSourceView(SourceViewCtrl* view)
{
    // Remove the existing view first
    if (_view)
    {
        delete _view;
        _view = nullptr;
    }

	_view = view;
	_panel->GetSizer()->Add(_view, 1, wxEXPAND | wxTOP, 6);
}

int DefinitionView::ShowModal()
{
	// Let subclasses load the values into the controls
	update();

	FitToScreen(0.6f, 0.66f);

	return DialogBase::ShowModal();
}

void DefinitionView::update()
{
	if (isEmpty())
	{
		// Null-ify the contents
		_declName->SetLabelMarkup("");
		_filename->SetLabelMarkup("");

		_view->Enable(false);
		return;
	}

	// Add the shader and file name
	auto declName = getDeclName();

	_declName->SetLabel(declName);
	_filename->SetLabel(getDeclFileName());

	_view->Enable(true);

	// Surround the definition with curly braces, these are not included
	std::string definition = declName + "\n{\n\r";
	definition += getDefinition();
	definition += "\n\r}";

	// Value Updates are only possible when read-only is false
	_view->SetReadOnly(false);
	_view->SetValue(definition);
	_view->SetReadOnly(true);
}

wxWindow* DefinitionView::getMainPanel()
{
	return _panel;
}

} // namespace ui
