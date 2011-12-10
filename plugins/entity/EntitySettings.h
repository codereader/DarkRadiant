#pragma once

#include "iregistry.h"
#include <boost/shared_ptr.hpp>
#include "math/Vector3.h"

namespace entity {

namespace
{
	const char* const RKEY_SHOW_ENTITY_NAMES("user/ui/xyview/showEntityNames");
	const char* const RKEY_SHOW_ALL_SPEAKER_RADII = "user/ui/showAllSpeakerRadii";
	const char* const RKEY_SHOW_ALL_LIGHT_RADII = "user/ui/showAllLightRadii";
	const char* const RKEY_DRAG_RESIZE_SYMMETRICALLY = "user/ui/dragResizeEntitiesSymmetrically";
	const char* const RKEY_ALWAYS_SHOW_LIGHT_VERTICES = "user/ui/alwaysShowLightVertices";
	const char* const RKEY_FREE_MODEL_ROTATION = "user/ui/freeModelRotation";
	const char* const RKEY_SHOW_ENTITY_ANGLES = "user/ui/xyview/showEntityAngles";
}

class EntitySettings;
typedef boost::shared_ptr<EntitySettings> EntitySettingsPtr;

/**
 * greebo: A class managing the various settings for entities. It observes
 * the corresponding keyvalues in the registry and updates the internal
 * variables accordingly. This can be used as some sort of "cache"
 * to avoid slow registry queries during rendering, for instance.
 */
class EntitySettings: public sigc::trackable
{
public:
	enum LightEditVertexType
	{
		VERTEX_START_END_DESELECTED,
		VERTEX_START_END_SELECTED,
		VERTEX_INACTIVE,
		VERTEX_DESELECTED,
		VERTEX_SELECTED,
		NUM_LIGHT_VERTEX_COLOURS,
	};

private:
	// TRUE if entity names should be drawn
    bool _renderEntityNames;

	// TRUE if speaker radii should be drawn
	bool _showAllSpeakerRadii;

	// TRUE if light radii should be drawn even when not selected
	bool _showAllLightRadii;

	// TRUE if entities should resize symmetrically on drag-resize operations
	bool _dragResizeEntitiesSymmetrically;

	// TRUE if lights should always render their components
	bool _alwaysShowLightVertices;

	bool _freeModelRotation;

	// true if GenericEntities should render their direction arrows
	bool _showEntityAngles;

	// Cached colour values
	Vector3 _lightVertexColours[NUM_LIGHT_VERTEX_COLOURS];

	bool _lightVertexColoursLoaded;

	// Private constructor
	EntitySettings();
public:
	const Vector3& getLightVertexColour(LightEditVertexType type)
	{
		if (!_lightVertexColoursLoaded)
		{
			loadLightVertexColours();
		}

		return _lightVertexColours[type];
	}

	bool renderEntityNames() {
		return _renderEntityNames;
	}

	bool showAllSpeakerRadii() {
		return _showAllSpeakerRadii;
	}

	bool showAllLightRadii() {
		return _showAllLightRadii;
	}

	bool dragResizeEntitiesSymmetrically() {
		return _dragResizeEntitiesSymmetrically;
	}

	bool alwaysShowLightVertices() {
		return _alwaysShowLightVertices;
	}

	bool freeModelRotation()
	{
		return _freeModelRotation;
	}

	bool showEntityAngles()
	{
		return _showEntityAngles;
	}

	// Container for the singleton (ptr)
	static EntitySettingsPtr& InstancePtr();

	// Releases the singleton pointer
	static void destroy();

private:
	void keyChanged();
    void refreshFromRegistry();
    void observeKey(const std::string&);
	void loadLightVertexColours();
};

} // namespace entity
