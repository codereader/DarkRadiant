#pragma once

#include "icommandsystem.h"

#include "wxutil/WindowPosition.h"
#include "wxutil/PanedPosition.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/dataview/DeclarationTreeView.h"
#include "wxutil/dialog/DialogBase.h"

namespace ui
{

class ModelTreeView;

class SkinEditor final :
    public wxutil::DialogBase,
    private wxutil::XmlResourceBasedWidget
{
private:
    ModelTreeView* _modelTreeView;

    wxutil::DeclarationTreeView::Columns _columns;
    wxutil::DeclarationTreeView* _skinTreeView;

    wxutil::WindowPosition _windowPosition;
    wxutil::PanedPosition _panedPosition;

private:
    SkinEditor();
    ~SkinEditor() override;

public:
    int ShowModal() override;

    static void ShowDialog(const cmd::ArgumentList& args);

private:
    void setupModelTreeView();
    void setupSkinTreeView();

    template<typename ObjectClass>
    ObjectClass* getControl(const std::string& name)
    {
        return findNamedObject<ObjectClass>(this, name);
    }
};

}
