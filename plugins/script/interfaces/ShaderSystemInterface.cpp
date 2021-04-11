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

ScriptMaterial ShaderSystemInterface::getMaterial(const std::string& name)
{
	return ScriptMaterial(GlobalMaterialManager().getMaterial(name));
}

bool ShaderSystemInterface::materialExists(const std::string& name)
{
    return GlobalMaterialManager().materialExists(name);
}

bool ShaderSystemInterface::materialCanBeModified(const std::string& name)
{
    return GlobalMaterialManager().materialCanBeModified(name);
}

ScriptMaterial ShaderSystemInterface::createEmptyMaterial(const std::string& name)
{
    return ScriptMaterial(GlobalMaterialManager().createEmptyMaterial(name));
}

ScriptMaterial ShaderSystemInterface::copyMaterial(const std::string& nameOfOriginal, const std::string& nameOfCopy)
{
    return ScriptMaterial(GlobalMaterialManager().copyMaterial(nameOfOriginal, nameOfCopy));
}

bool ShaderSystemInterface::renameMaterial(const std::string& oldName, const std::string& newName)
{
    return GlobalMaterialManager().renameMaterial(oldName, newName);
}

void ShaderSystemInterface::removeMaterial(const std::string& name)
{
    GlobalMaterialManager().removeMaterial(name);
}

void ShaderSystemInterface::saveMaterial(const std::string& name)
{
    GlobalMaterialManager().saveMaterial(name);
}

// IScriptInterface implementation
void ShaderSystemInterface::registerInterface(py::module& scope, py::dict& globals)
{
	// Add the declaration for a Material object
	py::class_<ScriptMaterial> material(scope, "Material");

    // Add the old name as alias
    scope.add_object("Shader", material);

    // Expose the enums in the material scope
    py::enum_<Material::SortRequest>(material, "SortRequest")
        .value("SUBVIEW", Material::SORT_SUBVIEW)
        .value("GUI", Material::SORT_GUI)
        .value("BAD", Material::SORT_BAD)
        .value("OPAQUE", Material::SORT_OPAQUE)
        .value("PORTAL_SKY", Material::SORT_PORTAL_SKY)
        .value("DECAL", Material::SORT_DECAL)
        .value("FAR", Material::SORT_FAR)
        .value("MEDIUM", Material::SORT_MEDIUM)
        .value("CLOSE", Material::SORT_CLOSE)
        .value("ALMOST_NEAREST", Material::SORT_ALMOST_NEAREST)
        .value("NEAREST", Material::SORT_NEAREST)
        .value("AFTER_FOG", Material::SORT_AFTER_FOG)
        .value("POST_PROCESS", Material::SORT_POST_PROCESS)
        .export_values();

    py::enum_<Material::CullType>(material, "CullType")
        .value("BACK", Material::CULL_BACK)
        .value("FRONT", Material::CULL_FRONT)
        .value("NONE", Material::CULL_NONE)
        .export_values();

    py::enum_<ClampType>(material, "ClampType")
        .value("REPEAT", CLAMP_REPEAT)
        .value("NOREPEAT", CLAMP_NOREPEAT)
        .value("ZEROCLAMP", CLAMP_ZEROCLAMP)
        .value("ALPHAZEROCLAMP", CLAMP_ALPHAZEROCLAMP)
        .export_values();

    py::enum_<Material::Flags>(material, "Flag")
        .value("NOSHADOWS", Material::FLAG_NOSHADOWS)
        .value("NOSELFSHADOW", Material::FLAG_NOSELFSHADOW)
        .value("FORCESHADOWS", Material::FLAG_FORCESHADOWS)
        .value("NOOVERLAYS", Material::FLAG_NOOVERLAYS)
        .value("FORCEOVERLAYS", Material::FLAG_FORCEOVERLAYS)
        .value("TRANSLUCENT", Material::FLAG_TRANSLUCENT)
        .value("FORCEOPAQUE", Material::FLAG_FORCEOPAQUE)
        .value("NOFOG", Material::FLAG_NOFOG)
        .value("NOPORTALFOG", Material::FLAG_NOPORTALFOG)
        .value("UNSMOOTHEDTANGENTS", Material::FLAG_UNSMOOTHEDTANGENTS)
        .value("MIRROR", Material::FLAG_MIRROR)
        .value("POLYGONOFFSET", Material::FLAG_POLYGONOFFSET)
        .value("ISLIGHTGEMSURF", Material::FLAG_ISLIGHTGEMSURF)
        .export_values();

	material.def(py::init<const MaterialPtr&>());
	material.def("getName", &ScriptMaterial::getName);
	material.def("getShaderFileName", &ScriptMaterial::getShaderFileName);
	material.def("setShaderFileName", &ScriptMaterial::setShaderFileName);
	material.def("getDescription", &ScriptMaterial::getDescription);
	material.def("getDefinition", &ScriptMaterial::getDefinition);
	material.def("isVisible", &ScriptMaterial::isVisible);
	material.def("isAmbientLight", &ScriptMaterial::isAmbientLight);
	material.def("isBlendLight", &ScriptMaterial::isBlendLight);
	material.def("isFogLight", &ScriptMaterial::isFogLight);
	material.def("isNull", &ScriptMaterial::isNull);
	material.def("getEditorImageExpressionString", &ScriptMaterial::getEditorImageExpressionString);
	material.def("setEditorImageExpressionFromString", &ScriptMaterial::setEditorImageExpressionFromString);
	material.def("getSortRequest", &ScriptMaterial::getSortRequest);
	material.def("setSortRequest", static_cast<void(ScriptMaterial::*)(float)>(&ScriptMaterial::setSortRequest));
	material.def("setSortRequest", static_cast<void(ScriptMaterial::*)(Material::SortRequest)>(&ScriptMaterial::setSortRequest));
	material.def("resetSortRequest", &ScriptMaterial::resetSortRequest);
	material.def("getPolygonOffset", &ScriptMaterial::getPolygonOffset);
	material.def("setPolygonOffset", &ScriptMaterial::setPolygonOffset);
	material.def("clearPolygonOffset", &ScriptMaterial::clearPolygonOffset);
	material.def("getClampType", &ScriptMaterial::getClampType);
	material.def("setClampType", &ScriptMaterial::setClampType);
	material.def("getCullType", &ScriptMaterial::getCullType);
	material.def("setCullType", &ScriptMaterial::setCullType);
	material.def("getMaterialFlags", &ScriptMaterial::getMaterialFlags);
	material.def("setMaterialFlag", &ScriptMaterial::setMaterialFlag);
	material.def("clearMaterialFlag", &ScriptMaterial::clearMaterialFlag);

	// Expose the MaterialVisitor interface

	py::class_<MaterialVisitor, MaterialVisitorWrapper> visitor(scope, "MaterialVisitor");
	visitor.def(py::init<>());
	visitor.def("visit", &MaterialVisitor::visit);

	// Add the module declaration to the given python namespace
	py::class_<ShaderSystemInterface> materialManager(scope, "MaterialManager");

	materialManager.def("foreachMaterial", &ShaderSystemInterface::foreachMaterial);
	materialManager.def("getMaterial", &ShaderSystemInterface::getMaterial);
	materialManager.def("materialExists", &ShaderSystemInterface::materialExists);
	materialManager.def("materialCanBeModified", &ShaderSystemInterface::materialCanBeModified);
	materialManager.def("createEmptyMaterial", &ShaderSystemInterface::createEmptyMaterial);
	materialManager.def("copyMaterial", &ShaderSystemInterface::copyMaterial);
	materialManager.def("renameMaterial", &ShaderSystemInterface::renameMaterial);
	materialManager.def("removeMaterial", &ShaderSystemInterface::removeMaterial);
	materialManager.def("saveMaterial", &ShaderSystemInterface::saveMaterial);

    scope.add_object("ShaderVisitor", visitor); // old compatibility name
	materialManager.def("foreachShader", &ShaderSystemInterface::foreachMaterial); // old compatibility name
	materialManager.def("getMaterialForName", &ShaderSystemInterface::getMaterial); // old compatibility name

	// Now point the Python variable "GlobalMaterialManager" to this instance
	globals["GlobalMaterialManager"] = this;
}

} // namespace script
