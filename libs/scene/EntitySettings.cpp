#include "EntitySettings.h"

#include "registry/registry.h"
#include "registry/adaptors.h"

EntitySettings::EntitySettings() :
	_lightVertexColours(static_cast<std::size_t>(LightEditVertexType::NumberOfVertexTypes))
{
	// Wire up all booleans to update on registry value changes
	initialiseAndObserveKey(RKEY_SHOW_ENTITY_NAMES, _renderEntityNames);
	initialiseAndObserveKey(RKEY_SHOW_ALL_SPEAKER_RADII, _showAllSpeakerRadii);
	initialiseAndObserveKey(RKEY_SHOW_ALL_LIGHT_RADII, _showAllLightRadii);
	initialiseAndObserveKey(RKEY_DRAG_RESIZE_SYMMETRICALLY, _dragResizeEntitiesSymmetrically);
	initialiseAndObserveKey(RKEY_ALWAYS_SHOW_LIGHT_VERTICES, _alwaysShowLightVertices);
	initialiseAndObserveKey(RKEY_FREE_OBJECT_ROTATION, _freeObjectRotation);
	initialiseAndObserveKey(RKEY_SHOW_ENTITY_ANGLES, _showEntityAngles);

	// Initialise the default colours
	_lightVertexColours[static_cast<std::size_t>(LightEditVertexType::StartEndDeselected)] = Vector3(0,1,1);
	_lightVertexColours[static_cast<std::size_t>(LightEditVertexType::StartEndSelected)] = Vector3(0,0,1);
	_lightVertexColours[static_cast<std::size_t>(LightEditVertexType::Inactive)] = Vector3(1,0,0);
	_lightVertexColours[static_cast<std::size_t>(LightEditVertexType::Deselected)] = Vector3(0,1,0);
	_lightVertexColours[static_cast<std::size_t>(LightEditVertexType::Selected)] = Vector3(0,0,1);
}

void EntitySettings::initialiseAndObserveKey(const std::string& key, bool& targetBool)
{
	// Load the initial value from the registry
	targetBool = registry::getValue<bool>(key);

	_registryConnections.emplace_back(registry::observeBooleanKey(key,
		[&]() { targetBool = true; onSettingsChanged(); },
		[&]() { targetBool = false; onSettingsChanged(); })
	);
}

EntitySettingsPtr& EntitySettings::InstancePtr()
{
	static EntitySettingsPtr _entitySettingsInstancePtr;

	if (!_entitySettingsInstancePtr)
	{
		_entitySettingsInstancePtr.reset(new EntitySettings);
	}

	return _entitySettingsInstancePtr;
}

void EntitySettings::destroy()
{
	// free the instance
	InstancePtr().reset();
}

void EntitySettings::onSettingsChanged()
{
	_signalSettingsChanged.emit();
}
