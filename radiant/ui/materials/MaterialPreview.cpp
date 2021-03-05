#include "MaterialPreview.h"

namespace ui
{

MaterialPreview::MaterialPreview(wxWindow* parent) :
    RenderPreview(parent, true)
{}

const MaterialPtr& MaterialPreview::getMaterial()
{
    return _material;
}

void MaterialPreview::setMaterial(const MaterialPtr& material)
{
    _material = material;
}

bool MaterialPreview::canDrawGrid()
{
    return false;
}

}
