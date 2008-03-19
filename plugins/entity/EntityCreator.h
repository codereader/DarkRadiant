#ifndef ENTITYCREATOR_H_
#define ENTITYCREATOR_H_

#include "ientity.h"
#include "ieclass.h"

namespace entity {

class Doom3EntityCreator : 
	public EntityCreator
{
public:
	/** greebo: Creates an entity for the given EntityClass
	 */
	scene::INodePtr createEntity(IEntityClassPtr eclass);
	
	/** greebo: Sets the function to call when any keyvalue gets changed.
	 * 	
	 * Note: this currently points to the EntityInspector in the radiant core.
	 */
	void setKeyValueChangedFunc(KeyValueChangedFunc func);
	
	/* Connect two entities using a "target" key.
	 */
	void connectEntities(const scene::INodePtr& source, const scene::INodePtr& target);

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	virtual void shutdownModule();
	
private:
	/** greebo: Creates the right entity for the entityclass.
	 */
	scene::INodePtr getEntityForEClass(IEntityClassPtr eclass);
};
typedef boost::shared_ptr<Doom3EntityCreator> Doom3EntityCreatorPtr;

} // namespace entity

#endif /*ENTITYCREATOR_H_*/
