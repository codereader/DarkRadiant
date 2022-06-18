#include "ideclmanager.h"

#include "module/StaticModule.h"

namespace decl
{

class DeclarationManager :
    public IDeclarationManager
{
public:
    void registerDeclType(const std::string& typeName, const IDeclarationParser::Ptr& parser) override
    {
        
    }

    void unregisterDeclType(const std::string& typeName) override
    {

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
