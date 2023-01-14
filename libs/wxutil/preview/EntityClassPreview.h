#pragma once

#include "ieclass.h"
#include "ui/ideclpreview.h"
#include "ModelPreview.h"
#include "../dialog/MessageBox.h"

namespace wxutil
{

class EntityClassPreview :
    public EntityPreview,
    public ui::IDeclarationPreview
{
public:
    EntityClassPreview(wxWindow* parent) :
        EntityPreview(parent)
    {}

    // Returns the widget that can be packed into the selector container
    wxWindow* GetPreviewWidget() override
    {
        return _mainPanel;
    }

    void ClearPreview() override
    {
        setEntity({});
    }

    void SetPreviewDeclName(const std::string& declName) override
    {
        auto eclass = GlobalEntityClassManager().findClass(declName);

        if (declName.empty() || !eclass)
        {
            ClearPreview();
            return;
        }

        try
        {
            // Create an entity of the selected type
            auto entity = GlobalEntityModule().createEntity(eclass);
            setEntity(entity);
        }
        catch (const std::runtime_error&)
        {
            Messagebox::ShowError(fmt::format(
                _("Unable to setup the preview,\ncould not find the entity class '{0}'"),
                declName));
        }
    }
};

}
