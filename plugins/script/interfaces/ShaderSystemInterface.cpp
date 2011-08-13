#include "ShaderSystemInterface.h"

#include <boost/bind.hpp>

namespace script
{

namespace
{
	class ShaderNameToShaderWrapper 
	{
	private:
		shaders::ShaderVisitor& _visitor;

	public:
		ShaderNameToShaderWrapper(shaders::ShaderVisitor& visitor) :
			_visitor(visitor)
		{}

		void visit(const std::string& name)
		{
			// Resolve the material name and pass it on
			MaterialPtr material = GlobalMaterialManager().getMaterialForName(name);
			_visitor.visit(material);
		}
	};
}

void ShaderSystemInterface::foreachShader(shaders::ShaderVisitor& visitor)
{
	// Note: foreachShader only traverses the loaded materials, use a small adaptor to traverse all known
	ShaderNameToShaderWrapper adaptor(visitor);

	GlobalMaterialManager().foreachShaderName(boost::bind(&ShaderNameToShaderWrapper::visit, &adaptor, _1));
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
