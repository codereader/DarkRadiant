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
	std::ostream& _outStream;
  
	// Number of entities written (map global)
	int _entityCount;
	
	// Number of brushes written for the current entity (entity local)
	int _brushCount;

	// Are we writing dummy brushes to brushless entities?
	bool _writeDummyBrushes;
	
public:
	// Constructor
	NodeExporter(std::ostream& os);
	
	// Pre-descent callback
	virtual bool pre(const scene::INodePtr& node);
  
	// Post-descent callback
	virtual void post(const scene::INodePtr& node);

	// Instantiates a NodeExporter class and calls the traverse func
	static void write(scene::INodePtr root, GraphTraversalFunc traverse, std::ostream& os);

private:
	void exportEntity(const Entity& entity);
};

} // namespace map

#endif /* NODEEXPORTER_H_ */
