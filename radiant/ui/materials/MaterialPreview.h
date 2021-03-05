#pragma once

#include "ishaders.h"
#include "wxutil/preview/RenderPreview.h"

namespace ui
{

class MaterialPreview :
    public wxutil::RenderPreview
{
private:
    MaterialPtr _material;

public:
    MaterialPreview(wxWindow* parent);

    const MaterialPtr& getMaterial();
    void setMaterial(const MaterialPtr& material);
};

}
