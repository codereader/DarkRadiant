#ifndef MODELRESOURCE_H_
#define MODELRESOURCE_H_

#include "ireference.h"
#include "imodel.h"
#include <set>
#include <boost/utility.hpp>
#include <boost/weak_ptr.hpp>

namespace model {

class ModelResource : 
	public Resource,
	public boost::noncopyable
{
	scene::INodePtr _model;
  
	// Name given during construction
	const std::string _originalName;
	
	std::string _path;
	std::string _name;
  
	// Type of resource (ase, lwo etc)
	std::string _type;
	
	typedef std::set<Resource::Observer*> ResourceObserverList;
	ResourceObserverList _observers;
	
	bool _realised;

public:
	// Constructor
	ModelResource(const std::string& name);
	
	virtual ~ModelResource();

	bool load();
	bool save() { return false; } // is empty for ModelResources
	void flush();
	
	scene::INodePtr getNode();
	void setNode(scene::INodePtr node);
	
	virtual void addObserver(Observer& observer);
	virtual void removeObserver(Observer& observer);
	
	// Realise this ModelResource
	void realise();
	void unrealise();
	bool realised();

	void refresh();
  
	/// \brief Returns the model loader for the model \p type or 0 if the model \p type has no loader module
	static ModelLoaderPtr getModelLoaderForType(const std::string& type);
	
private:
	void setModel(scene::INodePtr model);
	void clearModel();
		
	scene::INodePtr loadModelNode();
};
// Resource pointer types
typedef boost::shared_ptr<ModelResource> ModelResourcePtr;
typedef boost::weak_ptr<ModelResource> ModelResourceWeakPtr;

} // namespace model

#endif /*MODELRESOURCE_H_*/
