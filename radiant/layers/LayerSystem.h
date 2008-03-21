#ifndef LAYERSYSTEM_H_
#define LAYERSYSTEM_H_

#include <vector>
#include <map>
#include "ilayer.h"
#include "generic/callback.h"

namespace scene {

class LayerSystem :
	public ILayerSystem
{
	// greebo: An array of booleans reflects the visibility status
	// of all layers. Indexed by the layer id, it can be used to
	// quickly check whether a layer is visible or not.
	typedef std::vector<bool> LayerVisibilityList;
	LayerVisibilityList _layerVisibility;

	// The list of named layers, indexed by an integer ID
	typedef std::map<int, std::string> LayerMap;
	LayerMap _layers;

public:
	LayerSystem();

	/**
	 * greebo: Creates a new layer with the given name.
	 * 
	 * @returns: the ID of the layer of -1 if the layer could not
	 *           be created (e.g. due to a name conflict).
	 */
	int createLayer(const std::string& name);

	/**
	 * greebo: Deletes the named layer. All nodes are removed
	 *         from this layer before deletion.
	 */
	void deleteLayer(const std::string& name);

	virtual bool layerIsVisible(const std::string& layerName);
	virtual bool layerIsVisible(int layerID);

	virtual void setLayerVisibility(const std::string& layerName, bool visible);
	virtual void setLayerVisibility(int layerID, bool visible);

	void addSelectionToLayer(const std::string& layerName);

	virtual bool updateNodeVisibility(const scene::INodePtr& node);

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	virtual void shutdownModule();

	void toggleLayerVisibility();
	typedef MemberCaller<LayerSystem, &LayerSystem::toggleLayerVisibility> ToggleCaller;

	void addSelectionToLayer1();
	typedef MemberCaller<LayerSystem, &LayerSystem::addSelectionToLayer1> AddSelectionCaller;

private:
	// Internal event, updates the scenegraph
	void onLayerVisibilityChanged();

	// Returns the ID of the named layer, or -1 if name doesn't exist
	int getLayerID(const std::string& name) const;

	// Returns the highest used layer Id
	int getHighestLayerID() const;

	// Returns the lowest unused layer ID
	int getLowestUnusedLayerID();
};

// Internal accessor, only accessible within this binary
LayerSystem& getLayerSystem();

} // namespace scene

#endif /* LAYERSYSTEM_H_ */
