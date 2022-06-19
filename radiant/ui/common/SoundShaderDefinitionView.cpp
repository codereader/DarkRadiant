#include "SoundShaderDefinitionView.h"

#include "i18n.h"
#include "wxutil/SourceView.h"

namespace ui
{

SoundShaderDefinitionView::SoundShaderDefinitionView(const std::string& shaderName, wxWindow* parent) :
	DefinitionView(_("View Sound Shader Definition"), parent)
{
	addSourceView(new wxutil::D3SoundShaderSourceViewCtrl(getMainPanel()));

	if (module::GlobalModuleRegistry().moduleExists(MODULE_SOUNDMANAGER))
	{
		_shader = GlobalSoundManager().getSoundShader(shaderName);
	}
}

void SoundShaderDefinitionView::setShader(const std::string& shader)
{
	if (module::GlobalModuleRegistry().moduleExists(MODULE_SOUNDMANAGER))
	{
		_shader = GlobalSoundManager().getSoundShader(shader);
	}

	update();
}

bool SoundShaderDefinitionView::isEmpty() const
{
	return !_shader;
}

std::string SoundShaderDefinitionView::getDeclName()
{
	return _shader ? _shader->getDeclName() : std::string();
}

std::string SoundShaderDefinitionView::getDeclFileName()
{
	return _shader ? _shader->getShaderFilePath() : std::string();
}

std::string SoundShaderDefinitionView::getDefinition()
{
	return _shader ? _shader->getDefinition() : std::string();
}

}
