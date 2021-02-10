#include "MaterialEditor.h"

#include "i18n.h"

#include <wx/panel.h>
#include <wx/splitter.h>

namespace ui
{

namespace
{
    const char* const DIALOG_TITLE = N_("Material Editor");
    const std::string RKEY_ROOT = "user/ui/materialEditor/";
    const std::string RKEY_SPLIT_POS = RKEY_ROOT + "splitPos";
    const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
}

MaterialEditor::MaterialEditor() :
    DialogBase(DIALOG_TITLE)
{
    loadNamedPanel(this, "MaterialEditorMainPanel");

    // Setup the splitter and preview
    auto* splitter = findNamedObject<wxSplitterWindow>(this, "MaterialEditorSplitter");
    splitter->SetSashPosition(GetSize().GetWidth() * 0.6f);
    splitter->SetMinimumPaneSize(10); // disallow unsplitting

    // Set the default size of the window
    FitToScreen(0.8f, 0.6f);

    Layout();
    Fit();

    // Connect the window position tracker
    _windowPosition.loadFromPath(RKEY_WINDOW_STATE);
    _windowPosition.connect(this);
    _windowPosition.applyPosition();

    _panedPosition.connect(splitter);
    _panedPosition.loadFromPath(RKEY_SPLIT_POS);

    CenterOnParent();
}

int MaterialEditor::ShowModal()
{
    // Restore the position
    _windowPosition.applyPosition();

    int returnCode = DialogBase::ShowModal();

    // Tell the position tracker to save the information
    _windowPosition.saveToPath(RKEY_WINDOW_STATE);

    return returnCode;
}

void MaterialEditor::ShowDialog(const cmd::ArgumentList& args)
{
    MaterialEditor* editor = new MaterialEditor;

    editor->ShowModal();
    editor->Destroy();
}

}
