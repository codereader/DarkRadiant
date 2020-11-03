#include "EntityClassColourManager.h"

#include <sigc++/functors/mem_fun.h>
#include "ieclass.h"
#include "icolourscheme.h"
#include "UserInterfaceModule.h"

namespace ui
{

EntityClassColourManager::EntityClassColourManager()
{
    _loadedConn = GlobalEntityClassManager().defsLoadedSignal().connect(
        sigc::mem_fun(*this, &EntityClassColourManager::onEntityDefsLoaded)
    );
    _reloadedConn = GlobalEntityClassManager().defsReloadedSignal().connect(
        sigc::mem_fun(*this, &EntityClassColourManager::onEntityDefsReloaded)
    );
}

EntityClassColourManager::~EntityClassColourManager()
{
    _loadedConn.disconnect();
    _reloadedConn.disconnect();
}

void EntityClassColourManager::applyColourScheme()
{
    // greebo: Override the eclass colours of two special entityclasses
    Vector3 worlspawnColour = GlobalColourSchemeManager().getColour("default_brush");
    Vector3 lightColour = GlobalColourSchemeManager().getColour("light_volumes");

    auto light = GlobalEntityClassManager().findClass("light");

    if (light)
    {
        light->setColour(lightColour);
    }

    auto worldspawn = GlobalEntityClassManager().findClass("worldspawn");

    if (worldspawn)
    {
        worldspawn->setColour(worlspawnColour);
    }
}

void EntityClassColourManager::onEntityDefsLoaded()
{
    // Signal might receive us from a worker thread
    GetUserInterfaceModule().dispatch([this]()
    {
        applyColourScheme();
    });
}

void EntityClassColourManager::onEntityDefsReloaded()
{
    // Signal might receive us from a worker thread
    GetUserInterfaceModule().dispatch([this]()
    {
        applyColourScheme();
    });
}

}
