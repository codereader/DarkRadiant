#include "MapPosition.h"

#include "camera/GlobalCamera.h"
#include "ientity.h"
#include "itextstream.h"
#include "string/string.h"
#include "map/Map.h"

namespace map {

	namespace {
		const std::string RKEY_MAP_POSROOT = "game/mapFormat/mapPositionPosKey";
		const std::string RKEY_MAP_ANGLEROOT = "game/mapFormat/mapPositionAngleKey";
	}

MapPosition::MapPosition(unsigned int index) :
	_index(index),
	_position(0,0,0),
	_angle(0,0,0)
{
	// Construct the entity key names from the index
	_posKey = GlobalRegistry().get(RKEY_MAP_POSROOT) + intToStr(_index);
	_angleKey = GlobalRegistry().get(RKEY_MAP_ANGLEROOT) + intToStr(_index);
}

void MapPosition::load(Entity* entity) {
	// Sanity check
	if (entity != NULL) {
		const std::string savedPos = entity->getKeyValue(_posKey);
				
		if (savedPos != "") {
			// Construct the vectors out of the std::string
			_position = Vector3(savedPos);
			_angle = entity->getKeyValue(_angleKey);
		}
	}
}

void MapPosition::save(Entity* entity) {
	// Sanity check
	if (entity != NULL) {
		if (!empty()) {
			globalOutputStream() << "Saving to key: " << _posKey.c_str() << "\n";
			entity->setKeyValue(_posKey, _position);
			entity->setKeyValue(_angleKey, _angle);
		}
		else {
			// This is an empty position, clear the values
			remove(entity);
		}
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

void MapPosition::store() {
	globalOutputStream() << "Storing map position #" << _index << "\n";
	CamWndPtr camwnd = GlobalCamera().getCamWnd();
	
	if (camwnd != NULL) {
		_position = camwnd->getCameraOrigin();
		_angle = camwnd->getCameraAngles();
		
		// Tag the map as modified
		GlobalMap().setModified(true);
	}
	else {
		globalErrorStream() << "MapPosition: Warning: Couldn't find Camera.\n";
	}
}

void MapPosition::recall() {
	if (!empty()) {
		globalOutputStream() << "Restoring map position #" << _index << "\n";
		// Focus the view with the default angle
		Map::focusViews(_position, _angle);
	}
}

} // namespace map
