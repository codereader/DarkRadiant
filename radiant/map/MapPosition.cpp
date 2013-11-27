#include "MapPosition.h"

#include "camera/GlobalCamera.h"
#include "ientity.h"
#include "itextstream.h"
#include "string/string.h"
#include "map/Map.h"
#include "gamelib.h"

namespace map {

    namespace {
        const std::string GKEY_MAP_POSROOT = "/mapFormat/mapPositionPosKey";
        const std::string GKEY_MAP_ANGLEROOT = "/mapFormat/mapPositionAngleKey";
    }

MapPosition::MapPosition(unsigned int index) :
    _index(index),
    _position(0,0,0),
    _angle(0,0,0)
{
    // Construct the entity key names from the index
    _posKey = game::current::getValue<std::string>(GKEY_MAP_POSROOT) + string::to_string(_index);
    _angleKey = game::current::getValue<std::string>(GKEY_MAP_ANGLEROOT) + string::to_string(_index);
}

void MapPosition::load(Entity* entity)
{
    // Sanity check
    if (entity != NULL)
    {
        const std::string savedPos = entity->getKeyValue(_posKey);

        if (savedPos != "")
        {
            // Construct the vectors out of the std::string
            _position = string::convert<Vector3>(savedPos);
            _angle = string::convert<Vector3>(entity->getKeyValue(_angleKey));
        }
    }
}

void MapPosition::save(Entity* entity)
{
    // Sanity check
    if (entity == NULL) return;

    if (!empty())
    {
        rMessage() << "Saving to key: " << _posKey << std::endl;
        entity->setKeyValue(_posKey, string::to_string(_position));
        entity->setKeyValue(_angleKey, string::to_string(_angle));
    }
    else
    {
        // This is an empty position, clear the values
        remove(entity);
    }
}

void MapPosition::remove(Entity* entity) {
    // Sanity check
    if (entity != NULL) {
        entity->setKeyValue(_posKey, "");
        entity->setKeyValue(_angleKey, "");
    }
}

void MapPosition::clear() {
    _position = Vector3(0,0,0);
    _angle = Vector3(0,0,0);
}

bool MapPosition::empty() const {
    return (_position == Vector3(0,0,0) && _angle == Vector3(0,0,0));
}

void MapPosition::store(const cmd::ArgumentList& args) {
    rMessage() << "Storing map position #" << _index << std::endl;
    CamWndPtr camwnd = GlobalCamera().getActiveCamWnd();

    if (camwnd != NULL) {
        _position = camwnd->getCameraOrigin();
        _angle = camwnd->getCameraAngles();

        // Tag the map as modified
        GlobalMap().setModified(true);
    }
    else {
        rError() << "MapPosition: Warning: Couldn't find Camera." << std::endl;
    }
}

void MapPosition::recall(const cmd::ArgumentList& args) {
    if (!empty()) {
        rMessage() << "Restoring map position #" << _index << std::endl;
        // Focus the view with the default angle
        Map::focusViews(_position, _angle);
    }
}

} // namespace map
