#include "SkinEditor.h"

#include "i18n.h"

namespace ui
{

namespace
{
    constexpr const char* const DIALOG_TITLE = N_("Skin Editor");

    const std::string RKEY_ROOT = "user/ui/skinEditor/";
    const std::string RKEY_SPLIT_POS = RKEY_ROOT + "splitPos";
    const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
}

SkinEditor::SkinEditor() :
    DialogBase(DIALOG_TITLE)
{
    loadNamedPanel(this, "SkinEditorMainPanel");

    // Set the default size of the window
    FitToScreen(0.8f, 0.9f);

    Layout();
    Fit();

    // Connect the window position tracker
    _windowPosition.loadFromPath(RKEY_WINDOW_STATE);
    _windowPosition.connect(this);
    _windowPosition.applyPosition();

    //_panedPosition.connect(splitter);
    //_panedPosition.loadFromPath(RKEY_SPLIT_POS);

    CenterOnParent();
}

SkinEditor::~SkinEditor()
{
    
}

int SkinEditor::ShowModal()
{
    // Restore the position
    _windowPosition.applyPosition();

    int returnCode = DialogBase::ShowModal();

    // Tell the position tracker to save the information
    _windowPosition.saveToPath(RKEY_WINDOW_STATE);

    return returnCode;
}

void SkinEditor::ShowDialog(const cmd::ArgumentList& args)
{
    auto* editor = new SkinEditor;

    editor->ShowModal();
    editor->Destroy();
}

}
