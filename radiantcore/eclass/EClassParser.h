#pragma once

#include <map>
#include "ieclass.h"
#include "parser/ThreadedDeclParser.h"

#include "EntityClass.h"
#include "Doom3ModelDef.h"

namespace eclass
{

/**
 * The EClassParser is the loader for all entity classes and modelDefs. 
 * It loads all entityDef declarations from the .def files in the VFS.
 * Loading can take place synchronously or in a worker thread.
 */
class EClassParser final :
    public parser::ThreadedDeclParser<void>
{
private:
    IEntityClassManager& _owner;
    std::map<std::string, EntityClass::Ptr>& _entityClasses;
    std::map<std::string, Doom3ModelDef::Ptr>& _modelDefs;

    // A unique parse pass identifier, used to check when existing
    // definitions have been parsed
    std::size_t _curParseStamp;

public:
    EClassParser(IEntityClassManager& owner,
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
