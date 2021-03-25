#include "MaterialDefinitionView.h"

#include "i18n.h"
#include "wxutil/SourceView.h"

namespace ui
{

MaterialDefinitionView::MaterialDefinitionView(const std::string& shaderName, wxWindow* parent) :
	DefinitionView(_("View Shader Definition"), parent)
{
	addSourceView(new wxutil::D3MaterialSourceViewCtrl(getMainPanel()));
	_material = GlobalMaterialManager().getMaterial(shaderName);
}

MaterialDefinitionView::~MaterialDefinitionView()
{
    _materialChanged.disconnect();
}

void MaterialDefinitionView::setShader(const std::string& shader)
{
    _materialChanged.disconnect();

	_material = GlobalMaterialManager().getMaterial(shader);

    if (_material)
    {
        _materialChanged = _material->sig_materialChanged().connect(
            sigc::mem_fun(this, &MaterialDefinitionView::update)
        );
    }

	update();
}

bool MaterialDefinitionView::isEmpty() const
{
	return !_material;
}

std::string MaterialDefinitionView::getDeclName()
{
	return _material ? _material->getName() : std::string();
}

std::string MaterialDefinitionView::getDeclFileName()
{
	return _material ? _material->getShaderFileName() : std::string();
}

std::string MaterialDefinitionView::getDefinition()
{
	return _material ? _material->getDefinition() : std::string();
}

}
