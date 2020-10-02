#include "MaterialDefinitionView.h"

#include "wxutil/SourceView.h"

namespace ui
{

MaterialDefinitionView::MaterialDefinitionView(const std::string& shaderName, wxWindow* parent) :
	DefinitionView(_("View Shader Definition"), parent)
{
	addSourceView(new wxutil::D3MaterialSourceViewCtrl(getMainPanel()));
	_material = GlobalMaterialManager().getMaterialForName(shaderName);
}

void MaterialDefinitionView::setShader(const std::string& shader)
{
	_material = GlobalMaterialManager().getMaterialForName(shader);
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
