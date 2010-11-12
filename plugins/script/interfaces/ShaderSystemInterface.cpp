#include "ShaderSystemInterface.h"

namespace script {

void ShaderSystemInterface::foreachShader(shaders::ShaderVisitor& visitor) {
	GlobalMaterialManager().foreachShader(visitor);
}

ScriptShader ShaderSystemInterface::getMaterialForName(const std::string& name) {
	return ScriptShader(GlobalMaterialManager().getMaterialForName(name));
}

// IScriptInterface implementation
void ShaderSystemInterface::registerInterface(boost::python::object& nspace) {
	// Add the declaration for a Shader object
	nspace["Shader"] = boost::python::class_<ScriptShader>(
		"Shader", boost::python::init<const MaterialPtr&>())
		.def("getName", &ScriptShader::getName)
		.def("getShaderFileName", &ScriptShader::getShaderFileName)
		.def("getDescription", &ScriptShader::getDescription)
		.def("getDefinition", &ScriptShader::getDefinition)
		.def("isVisible", &ScriptShader::isVisible)
		.def("isAmbientLight", &ScriptShader::isAmbientLight)
		.def("isBlendLight", &ScriptShader::isBlendLight)
		.def("isFogLight", &ScriptShader::isFogLight)
		.def("isNull", &ScriptShader::isNull)
	;

	// Expose the ShaderVisitor interface
	nspace["ShaderVisitor"] =
		boost::python::class_<ShaderVisitorWrapper, boost::noncopyable>("ShaderVisitor")
		.def("visit", boost::python::pure_virtual(&ShaderVisitorWrapper::visit))
	;

	// Add the module declaration to the given python namespace
	nspace["GlobalMaterialManager"] = boost::python::class_<ShaderSystemInterface>("GlobalMaterialManager")
		.def("foreachShader", &ShaderSystemInterface::foreachShader)
		.def("getMaterialForName", &ShaderSystemInterface::getMaterialForName)
	;

	// Now point the Python variable "GlobalMaterialManager" to this instance
	nspace["GlobalMaterialManager"] = boost::python::ptr(this);
}

} // namespace script
