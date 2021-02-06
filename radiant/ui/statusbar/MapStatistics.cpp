#include "MapStatistics.h"

#include "i18n.h"
#include "istatusbarmanager.h"
#include "iselection.h"
#include "icounter.h"
#include "selectionlib.h"

namespace ui
{

namespace statusbar
{

constexpr const char* const STATUS_BAR_ELEMENT = "MapStatistics";

MapStatistics::MapStatistics()
{
    _selectionChangedConn = GlobalSelectionSystem().signal_selectionChanged().connect(
        [this](const ISelectable&) { requestIdleCallback(); }
    );

    _countersChangedConn = GlobalCounters().signal_countersChanged().connect(
        [this]() { requestIdleCallback(); }
    );

    // Add the counter element
    GlobalStatusBarManager().addTextElement(
        STATUS_BAR_ELEMENT,
        "",  // no icon
        StandardPosition::MapStatistics,
        _("Number of brushes/patches/entities in this map\n(Number of selected items shown in parentheses)")
    );

    requestIdleCallback();
}

MapStatistics::~MapStatistics()
{
    _selectionChangedConn.disconnect();
    _countersChangedConn.disconnect();
}

void MapStatistics::onIdle()
{
    updateStatusBar();
}

void MapStatistics::updateStatusBar()
{
    const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
    auto& counterMgr = GlobalCounters();

    std::string text =
        fmt::format(_("Brushes: {0:d} ({1:d}) Patches: {2:d} ({3:d}) Entities: {4:d} ({5:d})"),
            counterMgr.getCounter(counterBrushes).get(),
            info.brushCount,
            counterMgr.getCounter(counterPatches).get(),
            info.patchCount,
            counterMgr.getCounter(counterEntities).get(),
            info.entityCount);

    GlobalStatusBarManager().setText(STATUS_BAR_ELEMENT, text);
}

}

}