#pragma once

#include <sigc++/connection.h>
#include "wxutil/event/SingleIdleCallback.h"

namespace ui
{

namespace statusbar
{

// Class handling the (brushes, patches, entities) counter in the status bar
class MapStatistics final :
    private wxutil::SingleIdleCallback
{
private:
    sigc::connection _selectionChangedConn;
    sigc::connection _countersChangedConn;

public:
    MapStatistics();

    ~MapStatistics();

protected:
    void onIdle() override;

private:
    void updateStatusBar();
};

}

}
