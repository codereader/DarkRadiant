#pragma once

#include <map>
#include "parser/ThreadedDeclParser.h"

#include "EntityClass.h"
#include "Doom3ModelDef.h"

namespace eclass
{

class EClassManager;

// Loads all entity classes in a worker thread
class EClassParser final :
    public parser::ThreadedDeclParser<void>
{
private:
    EClassManager& _owner;
    std::map<std::string, EntityClass::Ptr>& _entityClasses;
    std::map<std::string, Doom3ModelDef::Ptr>& _modelDefs;

    // A unique parse pass identifier, used to check when existing
    // definitions have been parsed
    std::size_t _curParseStamp;

public:
    EClassParser(EClassManager& owner, 
                 std::map<std::string, EntityClass::Ptr>& entityClasses, 
                 std::map<std::string, Doom3ModelDef::Ptr>& modelDefs) :
        ThreadedDeclParser<void>(decl::Type::EntityDef, "def/", "def", 1),
        _owner(owner),
        _entityClasses(entityClasses),
        _modelDefs(modelDefs),
        _curParseStamp(0)
    {}

protected:
    void onBeginParsing() override;

    // Parse the provided stream containing the contents of a single .def file.
    // Extract all entitydefs and create/update objects accordingly.
    void parse(std::istream& stream, const vfs::FileInfo& fileInfo, const std::string& modDir) override;

    void onFinishParsing() override;

private:
    void resolveInheritance();
    void resolveModelInheritance(const std::string& name, const Doom3ModelDef::Ptr& model);
    void applyColours();
};

}
