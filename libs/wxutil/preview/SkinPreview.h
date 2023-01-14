#pragma once

#include "ui/ideclpreview.h"
#include "ModelPreview.h"

namespace wxutil
{

class SkinPreview :
    public ModelPreview,
    public ui::IDeclarationPreview
{
private:
    std::string _model;

public:
    SkinPreview(wxWindow* parent, const std::string& model) :
        ModelPreview(parent),
        _model(model)
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
        setModel(_model);
        setSkin(declName);
    }
};

}
