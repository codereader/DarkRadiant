#include "MaterialChooser.h"

#include "i18n.h"
#include "texturelib.h"

#include <wx/textctrl.h>

namespace ui
{

MaterialChooser::MaterialChooser(wxWindow* parent, MaterialSelector::TextureFilter filter, wxTextCtrl* targetEntry) :
	wxutil::DeclarationSelectorDialog(decl::Type::Material, _("Choose Material"), "MaterialChooser", parent),
	_targetEntry(targetEntry)
{
    SetSelector(new MaterialSelector(this, std::bind(&MaterialChooser::shaderSelectionChanged, this), filter));
}

sigc::signal<void>& MaterialChooser::signal_shaderChanged()
{
    return _shaderChangedSignal;
}

void MaterialChooser::shaderSelectionChanged()
{
	if (_targetEntry)
	{
		_targetEntry->SetValue(GetSelectedDeclName());
	}

	// Propagate the call up to the client (e.g. SurfaceInspector)
    _shaderChangedSignal.emit();
}

int MaterialChooser::ShowModal()
{
    std::string initialShader;

    if (_targetEntry != nullptr)
    {
        initialShader = _targetEntry->GetValue();

        // Set the cursor of the tree view to the currently selected shader
        SetSelectedDeclName(initialShader);
    }

    auto result = DeclarationSelectorDialog::ShowModal();

    if (_targetEntry)
    {
        _targetEntry->SetValue(result == wxID_OK ? GetSelectedDeclName() : initialShader);
        _shaderChangedSignal.emit();
    }

    return result;
}

} // namespace
