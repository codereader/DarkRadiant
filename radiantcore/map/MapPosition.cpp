#include "MapPosition.h"

#include "camera/GlobalCamera.h"
#include "ientity.h"
#include "itextstream.h"
#include "string/string.h"
#include "map/Map.h"
#include "gamelib.h"

namespace map
{

    namespace 
    {
        const char* const GKEY_MAP_POSROOT = "/mapFormat/mapPositionPosKey";
        const char* const GKEY_MAP_ANGLEROOT = "/mapFormat/mapPositionAngleKey";
        const char* const POSITION_KEY_FORMAT = "MapPosition{0:d}";
        const char* const ANGLE_KEY_FORMAT = "MapAngle{0:d}";
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

void MapPosition::loadFrom(Entity* entity)
{
    // Sanity check
    if (entity == nullptr) return;

    const std::string savedPos = entity->getKeyValue(_posKey);

    if (!savedPos.empty())
    {
        // Parse the vectors from std::string
        _position = string::convert<Vector3>(savedPos);
        _angle = string::convert<Vector3>(entity->getKeyValue(_angleKey));
    }
}

void MapPosition::removeFrom(Entity* entity)
{
    // Sanity check
    if (entity != nullptr)
    {
        entity->setKeyValue(_posKey, "");
        entity->setKeyValue(_angleKey, "");
    }
}

void MapPosition::loadFrom(const scene::IMapRootNodePtr& root)
{
    assert(root);

    auto pos = root->getProperty(fmt::format(POSITION_KEY_FORMAT, _index));

    if (!pos.empty())
    {
        // Parse the vectors from std::string
        _position = string::convert<Vector3>(pos);

        auto angle = root->getProperty(fmt::format(ANGLE_KEY_FORMAT, _index));
        _angle = string::convert<Vector3>(angle);
    }
}

void MapPosition::saveTo(const scene::IMapRootNodePtr& root)
{
    assert(root);

    if (!empty())
    {
        root->setProperty(fmt::format(POSITION_KEY_FORMAT, _index), string::to_string(_position));
        root->setProperty(fmt::format(ANGLE_KEY_FORMAT, _index), string::to_string(_angle));
    }
    else
    {
        removeFrom(root);
    }
}

void MapPosition::removeFrom(const scene::IMapRootNodePtr& root)
{
    root->removeProperty(fmt::format(POSITION_KEY_FORMAT, _index));
    root->removeProperty(fmt::format(ANGLE_KEY_FORMAT, _index));
}

void MapPosition::clear()
{
    _position = Vector3(0,0,0);
    _angle = Vector3(0,0,0);
}

bool MapPosition::empty() const 
{
    return _position == Vector3(0,0,0) && _angle == Vector3(0,0,0);
}

void MapPosition::store(const cmd::ArgumentList& args)
{
    auto mapRoot = GlobalMapModule().getRoot();

    if (!mapRoot)
    {
        rError() << "Cannot store map position, no map loaded." << std::endl;
        return;
    }

    rMessage() << "Storing map position #" << _index << std::endl;
    
    auto camwnd = GlobalCamera().getActiveCamWnd();

    if (!camwnd)
    {
        rWarning() << "MapPosition: Couldn't find Camera." << std::endl;
        return;
    }

    _position = camwnd->getCameraOrigin();
    _angle = camwnd->getCameraAngles();

    saveTo(mapRoot);

    // Tag the map as modified
    GlobalMap().setModified(true);
}

void MapPosition::recall(const cmd::ArgumentList& args) 
{
    auto mapRoot = GlobalMapModule().getRoot();

    if (!mapRoot)
    {
        rError() << "Cannot recall map position, no map loaded." << std::endl;
        return;
    }

    // Refresh our local data from the properties
    loadFrom(mapRoot);

    if (!empty())
    {
        rMessage() << "Restoring map position #" << _index << std::endl;

        // Focus the view with the default angle
        Map::focusViews(_position, _angle);
    }
    else
    {
        rMessage() << "Map position #" << _index << " has not been set" << std::endl;
    }
}

} // namespace map
