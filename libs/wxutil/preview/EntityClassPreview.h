#pragma once

#include "ieclass.h"
#include "ui/ideclpreview.h"
#include "ModelPreview.h"

namespace wxutil
{

class EntityClassPreview :
    public ModelPreview,
    public ui::IDeclarationPreview
{
public:
    EntityClassPreview(wxWindow* parent) :
        ModelPreview(parent)
    {}

    // Returns the widget that can be packed into the selector container
    wxWindow* GetPreviewWidget() override
    {
        return _mainPanel;
    }

    void ClearPreview() override
    {
        setModel({});
        setSkin({});
    }

    void SetPreviewDeclName(const std::string& declName) override
    {
        auto eclass = GlobalEntityClassManager().findClass(declName);

        if (declName.empty() || !eclass)
        {
            ClearPreview();
            return;
        }

        setModel(eclass->getAttributeValue("model"));
        setSkin(eclass->getAttributeValue("skin"));
    }
};

}
