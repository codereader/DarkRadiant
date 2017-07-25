#include "ShaderDefinitionView.h"

#include "i18n.h"
#include "ishaders.h"
#include "imainframe.h"

#include "wxutil/SourceView.h"
#include "wxutil/dialog/DialogBase.h"

#include <wx/sizer.h>
#include <wx/stattext.h>

namespace ui
{

ShaderDefinitionView::ShaderDefinitionView(wxWindow* parent) :
	wxPanel(parent, wxID_ANY)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxFlexGridSizer* table = new wxFlexGridSizer(2, 2, 6, 6);

	wxStaticText* nameLabel = new wxStaticText(this, wxID_ANY, _("Material:"));
	wxStaticText* materialFileLabel = new wxStaticText(this, wxID_ANY, _("Defined in:"));
	
	_materialName = new wxStaticText(this, wxID_ANY, "");
	_materialName->SetFont(_materialName->GetFont().Bold());

	_filename = new wxStaticText(this, wxID_ANY, "");
	_filename->SetFont(_filename->GetFont().Bold());
	
	nameLabel->SetMinSize(wxSize(90, -1));
	materialFileLabel->SetMinSize(wxSize(90, -1));

	table->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL);
	table->Add(_materialName, 0, wxALIGN_CENTER_VERTICAL);
	
	table->Add(materialFileLabel, 0, wxALIGN_CENTER_VERTICAL);
	table->Add(_filename, 0, wxALIGN_CENTER_VERTICAL);

	wxStaticText* defLabel = new wxStaticText(this, wxID_ANY, _("Definition:"));

	_view = new wxutil::D3MaterialSourceViewCtrl(this);

	GetSizer()->Add(table, 0);
	GetSizer()->Add(defLabel, 0, wxTOP, 6);
	GetSizer()->Add(_view, 1, wxEXPAND | wxTOP, 6);
}

void ShaderDefinitionView::setShader(const std::string& shader)
{
	_shader = shader;

	update();
}

void ShaderDefinitionView::update()
{
	// Find the shader
	MaterialPtr material = GlobalMaterialManager().getMaterialForName(_shader);

	if (material == NULL)
	{
		// Null-ify the contents
		_materialName->SetLabelMarkup("");
		_filename->SetLabelMarkup("");

		_view->Enable(false);
		return;
	}

	// Add the shader and file name
	_materialName->SetLabelMarkup("<b>" + material->getName() + "</b>");
	_filename->SetLabelMarkup(std::string("<b>") + material->getShaderFileName() + "</b>");

	_view->Enable(true);

	// Surround the definition with curly braces, these are not included
	std::string definition = _shader + "\n{\n\r";
	definition += material->getDefinition();
	definition += "\n\r}";

	// Value Updates are only possible when read-only is false
	_view->SetReadOnly(false);
	_view->SetValue(definition);
	_view->SetReadOnly(true);
}

void ShaderDefinitionView::ShowDialog(const std::string& shaderName)
{
	wxutil::DialogBase* dialog = new wxutil::DialogBase(_("View Shader Definition"));

	dialog->SetSizer(new wxBoxSizer(wxVERTICAL));

	// Construct a shader view and pass the shader name
	ShaderDefinitionView* view = new ShaderDefinitionView(dialog);
	view->setShader(shaderName);

	dialog->GetSizer()->Add(view, 1, wxEXPAND | wxALL, 12);
	dialog->GetSizer()->Add(dialog->CreateStdDialogButtonSizer(wxOK), 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12);

	dialog->FitToScreen(0.5f, 0.66f);

	dialog->ShowModal();

	dialog->Destroy();
}

} // namespace ui
