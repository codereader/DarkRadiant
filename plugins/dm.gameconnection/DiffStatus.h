#pragma once

#include <map>

namespace map
{

/**
 * Represents the change of status of some object.
 * Used mainly to write map diff for hot reload feature.
 */
class DiffStatus
{
    //change in presence:
    // -1: removed
    //  1: added
    //  0: otherwise
    short _delta = 0;
    //true if object was modified in any way
    //added/removed objects count as modified too
    bool _modified = false;
    //true if object should be respawned afresh
    bool _respawn = false;

public:
    inline bool isAdded() const { return _delta > 0; }
    inline bool isRemoved() const { return _delta < 0; }
    inline bool isModified() const { return _modified; }
    inline bool needsRespawn() const { return _respawn; }

    inline static DiffStatus added() {
        DiffStatus res;
        res._delta = 1;
        res._modified = true;
        //res._respawn = true;    //TODO: add + remove forces respawn?
        return res;
    }
    inline static DiffStatus removed() {
        DiffStatus res;
        res._delta = -1;
        res._modified = true;
        return res;
    }
    inline static DiffStatus modified() {
        DiffStatus res;
        res._modified = true;
        return res;
    }
    inline static DiffStatus forceRespawn() {
        DiffStatus res;
        res._modified = true;
        res._respawn = true;
        return res;
    }

    DiffStatus combine(DiffStatus laterChange) const {
        DiffStatus res;
        res._delta = _delta + laterChange._delta;
        if (std::abs(res._delta) > 1) {
            assert(false);      //double addition without removal?...
            res._delta = (res._delta < 0 ? -1 : 1);
        }
        res._modified = _modified || laterChange._modified;
        res._respawn = _respawn || laterChange._respawn;
        return res;
    }
};

typedef std::map<std::string, DiffStatus> DiffEntityStatuses;

}
