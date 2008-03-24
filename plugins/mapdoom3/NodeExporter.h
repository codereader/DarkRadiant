#ifndef NODEEXPORTER_H_
#define NODEEXPORTER_H_

#include "inode.h"
#include "imap.h"
#include <vector>
#include <ostream>

class Entity;

namespace map {

/* Walker class to traverse the scene graph and write each entity
 * out to the token stream, including its member brushes.
 */

class NodeExporter : 
	public scene::NodeVisitor
{
	// Stack to hold the parent entity when writing a brush. Either
	// the parent Entity is pushed, or a NULL pointer.
	std::vector<Entity*> _entityStack;

	// Output stream to write to
	std::ostream& _mapStream;
	std::ostream& _infoStream;
  
	// Number of entities written (map global)
	std::size_t _entityCount;
	
	// Number of brushes written for the current entity (entity local)
	std::size_t _brushCount;

	// Number of node-to-layer mappings written
	std::size_t _layerInfoCount;

	// Are we writing dummy brushes to brushless entities?
	bool _writeDummyBrushes;
	
public:
	// Constructor
	NodeExporter(std::ostream& mapStream, std::ostream& infoStream);

	// Destructor
	virtual ~NodeExporter();
	
	// Pre-descent callback
	virtual bool pre(const scene::INodePtr& node);
  
	// Post-descent callback
	virtual void post(const scene::INodePtr& node);

private:
	// Export all of the keyvalues from the given entity.
	void exportEntity(const Entity& entity);

	// Writes the layer numbers the node is a member of
	// The node pointer can be NULL, an empty Node {} block is exported in this case.
	void writeNodeLayerInfo(const scene::INodePtr& node);

	// Writes the names of the layers existing in this map
	void writeLayerNames();
};

} // namespace map

#endif /* NODEEXPORTER_H_ */
