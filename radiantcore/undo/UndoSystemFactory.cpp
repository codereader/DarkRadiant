#include "iundo.h"

#include "i18n.h"
#include "ipreferencesystem.h"
#include "UndoSystem.h"
#include "module/StaticModule.h"

namespace undo
{

class UndoSystemFactory final :
    public IUndoSystemFactory
{
public:
    const std::string& getName() const override
    {
        static std::string _name(MODULE_UNDOSYSTEM_FACTORY);
        return _name;
    }

    const StringSet& getDependencies() const override
    {
        static StringSet _dependencies{ MODULE_PREFERENCESYSTEM };
        return _dependencies;
    }

    void initialiseModule(const IApplicationContext& ctx) override
    {
        // add the preference settings
        constructPreferences();
    }

    IUndoSystem::Ptr createUndoSystem() override
    {
        return std::make_shared<UndoSystem>();
    }

private:
    void constructPreferences()
    {
        IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Undo System"));
        page.appendSpinner(_("Undo Queue Size"), RKEY_UNDO_QUEUE_SIZE, 0, 1024, 1);
    }
};

// Static module instance
module::StaticModuleRegistration<UndoSystemFactory> _undoSystemFactoryModule;

}
