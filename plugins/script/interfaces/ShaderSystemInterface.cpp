#include "ShaderSystemInterface.h"

#include <functional>

namespace script
{

namespace
{
	class ShaderNameToShaderWrapper 
	{
	private:
		ShaderVisitor& _visitor;

	public:
		ShaderNameToShaderWrapper(ShaderVisitor& visitor) :
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

void ShaderSystemInterface::foreachShader(ShaderVisitor& visitor)
{
	// Note: foreachShader only traverses the loaded materials, use a small adaptor to traverse all known
	ShaderNameToShaderWrapper adaptor(visitor);

	GlobalMaterialManager().foreachShaderName(
		std::bind(&ShaderNameToShaderWrapper::visit, &adaptor, std::placeholders::_1));
}

ScriptShader ShaderSystemInterface::getMaterialForName(const std::string& name)
{
	return ScriptShader(GlobalMaterialManager().getMaterialForName(name));
}

// IScriptInterface implementation
void ShaderSystemInterface::registerInterface(py::module& scope, py::dict& globals) 
{
	// Add the declaration for a Shader object
	py::class_<ScriptShader> shader(scope, "Shader");

	shader.def(py::init<const MaterialPtr&>());
	shader.def("getName", &ScriptShader::getName);
	shader.def("getShaderFileName", &ScriptShader::getShaderFileName);
	shader.def("getDescription", &ScriptShader::getDescription);
	shader.def("getDefinition", &ScriptShader::getDefinition);
	shader.def("isVisible", &ScriptShader::isVisible);
	shader.def("isAmbientLight", &ScriptShader::isAmbientLight);
	shader.def("isBlendLight", &ScriptShader::isBlendLight);
	shader.def("isFogLight", &ScriptShader::isFogLight);
	shader.def("isNull", &ScriptShader::isNull);

	// Expose the ShaderVisitor interface

	py::class_<ShaderVisitor, ShaderVisitorWrapper> visitor(scope, "ShaderVisitor");
	visitor.def(py::init<>());
	visitor.def("visit", &ShaderVisitor::visit);

	// Add the module declaration to the given python namespace
	py::class_<ShaderSystemInterface> materialManager(scope, "MaterialManager");

	materialManager.def("foreachShader", &ShaderSystemInterface::foreachShader);
	materialManager.def("getMaterialForName", &ShaderSystemInterface::getMaterialForName);

	// Now point the Python variable "GlobalMaterialManager" to this instance
	globals["GlobalMaterialManager"] = this;
}

} // namespace script
