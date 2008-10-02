#ifndef MODELBREAKDOWN_H_
#define MODELBREAKDOWN_H_

#include <map>
#include <string>
#include "scenelib.h"
#include "imodel.h"
#include "modelskin.h"

namespace map {

/** 
 * greebo: This object traverses the scenegraph on construction
 * counting all occurrences of each model (plus skins). 
 */
class ModelBreakdown :
	public scene::NodeVisitor
{
public:
	struct ModelCount
	{
		std::size_t count;
		std::size_t polyCount;

		typedef std::map<std::string, std::size_t> SkinCountMap;
		SkinCountMap skinCount;
		
		ModelCount() : 
			count(0)
		{}
	};

	// The map associating model names with occurrences
	typedef std::map<std::string, ModelCount> Map;

private:
	mutable Map _map;

public:
	ModelBreakdown() {
		_map.clear();
		GlobalSceneGraph().root()->traverse(*this);
	}
	
	bool pre(const scene::INodePtr& node) {
		// Check if this node is a model
		model::ModelNodePtr modelNode = Node_getModel(node);

		if (modelNode != NULL) {
			// Get the actual model from the node
			const model::IModel& model = modelNode->getIModel();

			Map::iterator found = _map.find(model.getModelPath());

			if (found == _map.end()) {
				std::pair<Map::iterator, bool> result = _map.insert(
					Map::value_type(model.getModelPath(), ModelCount())
				);

				found = result.first;

				// Store the polycount in the map
				found->second.polyCount = model.getPolyCount();
				found->second.skinCount.clear();
			}

			// The iterator "found" is valid at this point
			// Get a shortcut reference
			ModelCount& modelCount = found->second;
			
			modelCount.count++;
			
			// Increase the skin count, check if we have a skinnable model
			SkinnedModelPtr skinned = boost::dynamic_pointer_cast<SkinnedModel>(node);

			if (skinned != NULL) {
				std::string skinName = skinned->getSkin();

				ModelCount::SkinCountMap::iterator foundSkin = modelCount.skinCount.find(skinName);

				if (foundSkin == modelCount.skinCount.end()) {
					std::pair<ModelCount::SkinCountMap::iterator, bool> result = 
						modelCount.skinCount.insert(ModelCount::SkinCountMap::value_type(skinName, 0));

					foundSkin = result.first;
				}

				foundSkin->second++;
			}
		}
	
		return true;
	}

	// Accessor method to retrieve the entity breakdown map
	Map getMap() {
		return _map;
	}
	
	Map::const_iterator begin() const {
		return _map.begin();
	}
	
	Map::const_iterator end() const {
		return _map.end();
	}

}; // class ModelBreakdown

} // namespace map

#endif /* MODELBREAKDOWN_H_ */
