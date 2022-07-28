#include "DeclarationManagerInterface.h"

namespace script
{

ScriptDeclaration DeclarationManagerInterface::findDeclaration(decl::Type type, const std::string& name)
{
    return ScriptDeclaration({});
}

void DeclarationManagerInterface::registerInterface(py::module& scope, py::dict& globals)
{
    py::class_<ScriptDeclaration> declaration(scope, "Declaration");

    py::enum_<decl::Type>(declaration, "Type")
        .value("None", decl::Type::None)
        .value("Material", decl::Type::Material)
        .value("Table", decl::Type::Table)
        .value("EntityDef", decl::Type::EntityDef)
        .value("SoundShader", decl::Type::SoundShader)
        .value("ModelDef", decl::Type::ModelDef)
        .value("Particle", decl::Type::Particle)
        .value("Skin", decl::Type::Skin)
        .export_values();


}

}
