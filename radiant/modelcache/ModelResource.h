#ifndef MODELRESOURCE_H_
#define MODELRESOURCE_H_

#include "ireference.h"
#include "imodel.h"
#include <set>
#include <boost/utility.hpp>

namespace model {

class ModelResource : 
	public Resource,
	public boost::noncopyable
{
	scene::INodePtr m_model;
  
	// Name given during construction
	const std::string m_originalName;
	
	std::string m_path;
	std::string m_name;
  
	// Type of resource (map, lwo etc)
	std::string _type;
	
	// ModelLoader for this resource type
	ModelLoader* m_loader;
	
	typedef std::set<Resource::Observer*> ResourceObserverList;
	ResourceObserverList _observers;
	
	std::time_t m_modified;
	bool _realised;

public:
	// Constructor
	ModelResource(const std::string& name);
	
	virtual ~ModelResource();

	void setModel(scene::INodePtr model);
	void clearModel();

	void loadCached();

	void loadModel();

	bool load();
	
	bool save() { return false; } // is empty for ModelResources
	
	void flush();
	
	scene::INodePtr getNode();
	void setNode(scene::INodePtr node);
	
	virtual void addObserver(Observer& observer);
	virtual void removeObserver(Observer& observer);
		
	bool realised();
  
	// Realise this ModelResource
	void realise();
	void unrealise();

  std::time_t modified() const;

  bool isModified() const;
  void refresh();
  
	/// \brief Returns the model loader for the model \p type or 0 if the model \p type has no loader module
	static ModelLoader* getModelLoaderForType(const std::string& type);
	
private:
	scene::INodePtr loadModelNode();
	
	scene::INodePtr loadModelResource();
};
// Resource pointer types
typedef boost::shared_ptr<ModelResource> ModelResourcePtr;
typedef boost::weak_ptr<ModelResource> ModelResourceWeakPtr;

} // namespace model

#endif /*MODELRESOURCE_H_*/
