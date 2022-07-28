#include "DeclarationManagerInterface.h"

namespace script
{

class DeclarationVisitorWrapper :
    public DeclarationVisitor
{
public:
    void visit(const decl::IDeclaration::Ptr& declaration) override
    {
        // Wrap this method to python
        PYBIND11_OVERLOAD_PURE(
            void,               // Return type
            DeclarationVisitor, // Parent class
            visit,              // Name of function in C++ (must match Python name)
            ScriptDeclaration(declaration)  // Argument(s)
        );
    }
};

ScriptDeclaration DeclarationManagerInterface::findDeclaration(decl::Type type, const std::string& name)
{
    return ScriptDeclaration({});
}

void DeclarationManagerInterface::foreachDeclaration(decl::Type type, DeclarationVisitor& visitor)
{
    GlobalDeclarationManager().foreachDeclaration(type, [&](const decl::IDeclaration::Ptr& decl)
    {
        visitor.visit(decl);
    });
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

    py::class_<DeclarationVisitor, DeclarationVisitorWrapper> visitor(scope, "DeclarationVisitor");
    visitor.def(py::init<>());
    visitor.def("visit", &DeclarationVisitor::visit);

    // IDeclarationManager interface
    py::class_<DeclarationManagerInterface> materialManager(scope, "DeclarationManager");

    materialManager.def("foreachDeclaration", &DeclarationManagerInterface::foreachDeclaration);

    // Now point the Python variable "GlobalDeclarationManager" to this instance
    globals["GlobalDeclarationManager"] = this;
}

}
