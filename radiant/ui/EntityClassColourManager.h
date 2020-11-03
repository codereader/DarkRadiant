#pragma once

#include <sigc++/connection.h>

namespace ui
{

class EntityClassColourManager
{
private:
	sigc::connection _loadedConn;
	sigc::connection _reloadedConn;

public:
    EntityClassColourManager();

    ~EntityClassColourManager();

private:
    void applyColourScheme();
    
    void onEntityDefsLoaded();
    void onEntityDefsReloaded();
};

}
