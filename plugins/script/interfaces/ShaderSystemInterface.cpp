#include "ShaderSystemInterface.h"

#include <functional>

namespace script
{

namespace
{
	class ShaderNameToShaderWrapper
	{
	private:
        MaterialVisitor& _visitor;

	public:
		ShaderNameToShaderWrapper(MaterialVisitor& visitor) :
			_visitor(visitor)
		{}

		void visit(const std::string& name)
		{
			// Resolve the material name and pass it on
			auto material = GlobalMaterialManager().getMaterial(name);
			_visitor.visit(material);
		}
	};
}

void ShaderSystemInterface::foreachMaterial(MaterialVisitor& visitor)
{
	// Note: foreachShader only traverses the loaded materials, use a small adaptor to traverse all known
	ShaderNameToShaderWrapper adaptor(visitor);

	GlobalMaterialManager().foreachShaderName(
		std::bind(&ShaderNameToShaderWrapper::visit, &adaptor, std::placeholders::_1));
}

ScriptShader ShaderSystemInterface::getMaterial(const std::string& name)
{
	return ScriptShader(GlobalMaterialManager().getMaterial(name));
}

// IScriptInterface implementation
void ShaderSystemInterface::registerInterface(py::module& scope, py::dict& globals)
{
	// Add the declaration for a Shader object
	py::class_<ScriptShader> shader(scope, "Material");

    // Add the old name as alias
    scope.add_object("Shader", shader);

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

	// Expose the MaterialVisitor interface

	py::class_<MaterialVisitor, MaterialVisitorWrapper> visitor(scope, "MaterialVisitor");
	visitor.def(py::init<>());
	visitor.def("visit", &MaterialVisitor::visit);

	// Add the module declaration to the given python namespace
	py::class_<ShaderSystemInterface> materialManager(scope, "MaterialManager");

	materialManager.def("foreachMaterial", &ShaderSystemInterface::foreachMaterial);
	materialManager.def("getMaterial", &ShaderSystemInterface::getMaterial);

    scope.add_object("ShaderVisitor", visitor); // old compatibility name
	materialManager.def("foreachShader", &ShaderSystemInterface::foreachMaterial); // old compatibility name
	materialManager.def("getMaterialForName", &ShaderSystemInterface::getMaterial); // old compatibility name

	// Now point the Python variable "GlobalMaterialManager" to this instance
	globals["GlobalMaterialManager"] = this;
}

} // namespace script
