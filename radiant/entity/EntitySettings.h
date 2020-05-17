#pragma once

#include "iregistry.h"
#include <sigc++/connection.h>
#include <sigc++/signal.h>
#include <memory>
#include "math/Vector3.h"

namespace entity {

namespace
{
	const char* const RKEY_SHOW_ENTITY_NAMES("user/ui/xyview/showEntityNames");
	const char* const RKEY_SHOW_ALL_SPEAKER_RADII = "user/ui/showAllSpeakerRadii";
	const char* const RKEY_SHOW_ALL_LIGHT_RADII = "user/ui/showAllLightRadii";
	const char* const RKEY_DRAG_RESIZE_SYMMETRICALLY = "user/ui/dragResizeEntitiesSymmetrically";
	const char* const RKEY_ALWAYS_SHOW_LIGHT_VERTICES = "user/ui/alwaysShowLightVertices";
	const char* const RKEY_FREE_OBJECT_ROTATION = "user/ui/rotateObjectsIndependently";
	const char* const RKEY_SHOW_ENTITY_ANGLES = "user/ui/xyview/showEntityAngles";
}

class EntitySettings;
typedef std::shared_ptr<EntitySettings> EntitySettingsPtr;

/**
 * greebo: A class managing the various settings for entities. It observes
 * the corresponding keyvalues in the registry and updates the internal
 * variables accordingly. This can be used as some sort of "cache"
 * to avoid slow registry queries during rendering, for instance.
 */
class EntitySettings :
	public IEntitySettings
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

	bool _freeObjectRotation;

	// true if GenericEntities should render their direction arrows
	bool _showEntityAngles;

	// Cached colour values
	Vector3 _lightVertexColours[NUM_LIGHT_VERTEX_COLOURS];

	bool _lightVertexColoursLoaded;

	std::vector<sigc::connection> _registryConnections;

	sigc::signal<void> _signalSettingsChanged;

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

	bool getRenderEntityNames() const override
	{
		return _renderEntityNames;
	}

	void setRenderEntityNames(bool value) override
	{
		_renderEntityNames = value;
		onSettingsChanged();
	}

	bool getShowAllSpeakerRadii() const override
	{
		return _showAllSpeakerRadii;
	}

	void setShowAllSpeakerRadii(bool value) override
	{
		_showAllSpeakerRadii = value;
		onSettingsChanged();
	}

	bool getShowAllLightRadii() const override
	{
		return _showAllLightRadii;
	}

	void setShowAllLightRadii(bool value) override
	{
		_showAllLightRadii = value;
		onSettingsChanged();
	}

	bool getDragResizeEntitiesSymmetrically() const override
	{
		return _dragResizeEntitiesSymmetrically;
	}

	void setDragResizeEntitiesSymmetrically(bool value) override
	{
		_dragResizeEntitiesSymmetrically = value;
		onSettingsChanged();
	}

	bool getAlwaysShowLightVertices() const override
	{
		return _alwaysShowLightVertices;
	}

	void setAlwaysShowLightVertices(bool value) override
	{
		_alwaysShowLightVertices = value;
		onSettingsChanged();
	}

	bool getFreeObjectRotation() const override
	{
		return _freeObjectRotation;
	}

	void setFreeObjectRotation(bool value) override
	{
		_freeObjectRotation = value;
		onSettingsChanged();
	}

	bool getShowEntityAngles() const override
	{
		return _showEntityAngles;
	}

	void setShowEntityAngles(bool value) override
	{
		_showEntityAngles = value;
		onSettingsChanged();
	}

	sigc::signal<void>& signal_settingsChanged() override
	{
		return _signalSettingsChanged;
	}

	// Container for the singleton (ptr)
	static EntitySettingsPtr& InstancePtr();

	// Releases the singleton pointer
	static void destroy();

private:
	void onSettingsChanged();
    void initialiseAndObserveKey(const std::string& key, bool& targetBool);
	void loadLightVertexColours();
};

} // namespace entity
