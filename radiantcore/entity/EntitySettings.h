#pragma once

#include "ientity.h"
#include "iregistry.h"
#include <sigc++/connection.h>
#include <sigc++/signal.h>
#include <memory>
#include "math/Vector3.h"

namespace entity {

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
	std::vector<Vector3> _lightVertexColours;

	std::vector<sigc::connection> _registryConnections;

	sigc::signal<void> _signalSettingsChanged;

	// Private constructor
	EntitySettings();
public:
	const Vector3& getLightVertexColour(LightEditVertexType type) const override
	{
		assert(type != LightEditVertexType::NumberOfVertexTypes);
		return _lightVertexColours[static_cast<std::size_t>(type)];
	}

	void setLightVertexColour(LightEditVertexType type, const Vector3& value) override
	{
		assert(type != LightEditVertexType::NumberOfVertexTypes);
		_lightVertexColours[static_cast<std::size_t>(type)] = value;

		onSettingsChanged();
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
};

} // namespace entity
