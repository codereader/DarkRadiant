#include "SoundInterface.h"

namespace script {

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
void SoundManagerInterface::registerInterface(boost::python::object& nspace)
{
	// Add the declaration for SoundRadii
	nspace["SoundRadii"] = boost::python::class_<ScriptSoundRadii>("SoundRadii")
		.def(boost::python::init<const SoundRadii&>())
		.def("setMin", &ScriptSoundRadii::setMin)
		.def("setMax", &ScriptSoundRadii::setMax)
		.def("getMin", &ScriptSoundRadii::getMin)
		.def("getMax", &ScriptSoundRadii::getMax)
	;

	// Add the declaration for a SoundShader
	nspace["SoundShader"] = boost::python::class_<ScriptSoundShader>(
		"SoundShader", boost::python::init<const ISoundShaderPtr&>())
		.def("isNull", &ScriptSoundShader::isNull)
		.def("getName", &ScriptSoundShader::getName)
		.def("getRadii", &ScriptSoundShader::getRadii)
		.def("getSoundFileList", &ScriptSoundShader::getSoundFileList)
	;

	// Add the module declaration to the given python namespace
	nspace["GlobalSoundManager"] = boost::python::class_<SoundManagerInterface>("GlobalSoundManager")
		.def("getSoundShader", &SoundManagerInterface::getSoundShader)
		.def("playSound", &SoundManagerInterface::playSound)
		.def("stopSound", &SoundManagerInterface::stopSound)
	;

	// Now point the Python variable "GlobalSoundManager" to this instance
	nspace["GlobalSoundManager"] = boost::python::ptr(this);
}

} // namespace script
