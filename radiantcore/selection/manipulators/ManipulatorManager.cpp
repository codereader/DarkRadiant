#include "imanipulator.h"

#include <map>
#include "module/StaticModule.h"
#include "string/convert.h"
#include "TextureToolRotateManipulator.h"

namespace selection
{

class ManipulatorManager :
    public IManipulatorManager
{
private:
    using ManipulatorsByType = std::map<IManipulator::Type, std::function<IManipulator::Ptr()>>;
    std::map<IManipulator::Context, ManipulatorsByType> _manipulatorsByContext;

public:
    
    IManipulator::Ptr createManipulator(IManipulator::Context context, IManipulator::Type type) override
    {
        if (_manipulatorsByContext.count(context) == 0)
        {
            throw std::runtime_error("Manipulator context not known: " + string::to_string(static_cast<int>(context)));
        }

        auto existing = _manipulatorsByContext[context].find(type);

        if (existing == _manipulatorsByContext[context].end())
        {
            throw std::runtime_error("Manipulator type not known: " + string::to_string(static_cast<int>(type)));
        }

        return existing->second();
    }

    const std::string& getName() const
    {
        static std::string _name(MODULE_MANIPULATORMANAGER);
        return _name;
    }

    const StringSet& getDependencies() const
    {
        static StringSet _dependencies;
        return _dependencies;
    }

    void initialiseModule(const IApplicationContext& ctx)
    {
        rMessage() << getName() << "::initialiseModule called." << std::endl;

        _manipulatorsByContext.emplace(IManipulator::Context::Scene, ManipulatorsByType());
        _manipulatorsByContext.emplace(IManipulator::Context::TextureTool, ManipulatorsByType());

        _manipulatorsByContext[IManipulator::Context::TextureTool].emplace(
            IManipulator::Rotate, [] { return std::make_shared<TextureToolRotateManipulator>(); });
    }

    void shutdownModule()
    {
        _manipulatorsByContext.clear();
    }
};

module::StaticModule<ManipulatorManager> _manipulatorManagerInstance;

}
