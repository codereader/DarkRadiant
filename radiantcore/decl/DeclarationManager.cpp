#include "ideclmanager.h"

#include <map>
#include "module/StaticModule.h"

namespace decl
{

class DeclarationManager :
    public IDeclarationManager
{
private:
    std::map<std::string, IDeclarationParser::Ptr> _parsersByTypename;

public:
    void registerDeclType(const std::string& typeName, const IDeclarationParser::Ptr& parser) override
    {
        if (_parsersByTypename.count(typeName) > 0)
        {
            throw std::logic_error("Type name " + typeName + " has already been registered");
        }

        _parsersByTypename.emplace(typeName, parser);
    }

    void unregisterDeclType(const std::string& typeName) override
    {
        auto existing = _parsersByTypename.find(typeName);

        if (existing == _parsersByTypename.end())
        {
            throw std::logic_error("Type name " + typeName + " has not been registered");
        }

        _parsersByTypename.erase(existing);
    }

    void registerDeclFolder(Type defaultType, const std::string& vfsFolder, const std::string& extension) override
    {

    }

    const std::string& getName() const override
    {
        static std::string _name(MODULE_DECLMANAGER);
        return _name;
    }

    const StringSet& getDependencies() const override
    {
        static StringSet _dependencies{};

        return _dependencies;
    }

    void initialiseModule(const IApplicationContext& ctx) override
    {
        rMessage() << getName() << "::initialiseModule called." << std::endl;
    }
};

module::StaticModuleRegistration<DeclarationManager> _declManagerModule;

}
