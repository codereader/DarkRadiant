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
    return ScriptDeclaration(GlobalDeclarationManager().findDeclaration(type, name));
}

ScriptDeclaration DeclarationManagerInterface::findOrCreateDeclaration(decl::Type type, const std::string& name)
{
    return ScriptDeclaration(GlobalDeclarationManager().findOrCreateDeclaration(type, name));
}

void DeclarationManagerInterface::foreachDeclaration(decl::Type type, DeclarationVisitor& visitor)
{
    GlobalDeclarationManager().foreachDeclaration(type, [&](const decl::IDeclaration::Ptr& decl)
    {
        visitor.visit(decl);
    });
}

bool DeclarationManagerInterface::renameDeclaration(decl::Type type, const std::string& oldName, const std::string& newName)
{
    return GlobalDeclarationManager().renameDeclaration(type, oldName, newName);
}

void DeclarationManagerInterface::removeDeclaration(decl::Type type, const std::string& name)
{
    GlobalDeclarationManager().removeDeclaration(type, name);
}

void DeclarationManagerInterface::reloadDeclarations()
{
    GlobalDeclarationManager().reloadDeclarations();
}

void DeclarationManagerInterface::saveDeclaration(const ScriptDeclaration& decl)
{
    if (!decl.get()) return;

    GlobalDeclarationManager().saveDeclaration(decl.get());
}

void DeclarationManagerInterface::registerInterface(py::module& scope, py::dict& globals)
{
    py::class_<ScriptDeclaration> declaration(scope, "Declaration");

    py::enum_<decl::Type>(declaration, "Type")
        .value("NullType", decl::Type::None) // None is a reserved word in Python
        .value("Material", decl::Type::Material)
        .value("Table", decl::Type::Table)
        .value("EntityDef", decl::Type::EntityDef)
        .value("SoundShader", decl::Type::SoundShader)
        .value("ModelDef", decl::Type::ModelDef)
        .value("Particle", decl::Type::Particle)
        .value("Skin", decl::Type::Skin)
        .value("Fx", decl::Type::Fx)
        .export_values();

    py::class_<decl::DeclarationBlockSyntax>(scope, "DeclarationBlockSyntax")
        .def_readwrite("typeName", &decl::DeclarationBlockSyntax::typeName)
        .def_readwrite("name", &decl::DeclarationBlockSyntax::name)
        .def_readwrite("contents", &decl::DeclarationBlockSyntax::contents)
        .def_readwrite("modName", &decl::DeclarationBlockSyntax::modName);

    declaration.def(py::init<const decl::IDeclaration::Ptr&>());
    declaration.def("isNull", &ScriptDeclaration::isNull);
    declaration.def("getDeclName", &ScriptDeclaration::getDeclName);
    declaration.def("getDeclType", &ScriptDeclaration::getDeclType);
    declaration.def("getBlockSyntax", &ScriptDeclaration::getBlockSyntax);
    declaration.def("setBlockSyntax", &ScriptDeclaration::setBlockSyntax);
    declaration.def("getDeclFilePath", &ScriptDeclaration::getDeclFilePath);
    declaration.def("setDeclFilePath", &ScriptDeclaration::setDeclFilePath);

    py::class_<DeclarationVisitor, DeclarationVisitorWrapper>(scope, "DeclarationVisitor")
        .def(py::init<>())
        .def("visit", &DeclarationVisitor::visit);

    // IDeclarationManager interface
    py::class_<DeclarationManagerInterface>(scope, "DeclarationManager")
        .def("findDeclaration", &DeclarationManagerInterface::findDeclaration)
        .def("findOrCreateDeclaration", &DeclarationManagerInterface::findOrCreateDeclaration)
        .def("foreachDeclaration", &DeclarationManagerInterface::foreachDeclaration)
        .def("renameDeclaration", &DeclarationManagerInterface::renameDeclaration)
        .def("removeDeclaration", &DeclarationManagerInterface::removeDeclaration)
        .def("reloadDeclarations", &DeclarationManagerInterface::reloadDeclarations)
        .def("saveDeclaration", &DeclarationManagerInterface::saveDeclaration);

    // Now point the Python variable "GlobalDeclarationManager" to this instance
    globals["GlobalDeclarationManager"] = this;
}

}
