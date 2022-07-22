#include "SoundInterface.h"

#include <pybind11/pybind11.h>

namespace script
{

ScriptSoundShader SoundManagerInterface::getSoundShader(const std::string& shaderName)
{
	return ScriptSoundShader(GlobalSoundManager().getSoundShader(shaderName));
}

bool SoundManagerInterface::playSound(const std::string& fileName)
{
	return GlobalSoundManager().playSound(fileName);
}

void SoundManagerInterface::stopSound()
{
	GlobalSoundManager().stopSound();
}

// IScriptInterface implementation
void SoundManagerInterface::registerInterface(py::module& scope, py::dict& globals)
{
	// Add the declaration for SoundRadii
	py::class_<ScriptSoundRadii> radii(scope, "SoundRadii");

	radii.def(py::init<const SoundRadii&>());
	radii.def("setMin", &ScriptSoundRadii::setMin);
	radii.def("setMax", &ScriptSoundRadii::setMax);
	radii.def("getMin", &ScriptSoundRadii::getMin);
	radii.def("getMax", &ScriptSoundRadii::getMax);

	// Add the declaration for a SoundShader
	py::class_<ScriptSoundShader> shader(scope, "SoundShader");

	shader.def(py::init<const ISoundShader::Ptr&>());
	shader.def("isNull", &ScriptSoundShader::isNull);
	shader.def("getName", &ScriptSoundShader::getName);
	shader.def("getRadii", &ScriptSoundShader::getRadii);
	shader.def("getSoundFileList", &ScriptSoundShader::getSoundFileList);
	shader.def("getShaderFilePath", &ScriptSoundShader::getShaderFilePath);
	shader.def("getDefinition", &ScriptSoundShader::getDefinition);

	// Add the module declaration to the given python namespace
	py::class_<SoundManagerInterface> soundManager(scope, "SoundManager");

	soundManager.def("getSoundShader", &SoundManagerInterface::getSoundShader);
	soundManager.def("playSound", &SoundManagerInterface::playSound);
	soundManager.def("stopSound", &SoundManagerInterface::stopSound);

	// Now point the Python variable "GlobalSoundManager" to this instance
	globals["GlobalSoundManager"] = this;
}

} // namespace script
