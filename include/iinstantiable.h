#ifndef IINSTANTIABLE_H_
#define IINSTANTIABLE_H_

#include "ipath.h"
#include <boost/shared_ptr.hpp>

namespace scene {

class Instance;

class Instantiable
{
public:
	STRING_CONSTANT(Name, "scene::Instantiable");

	class Observer
	{
	public:
		/// \brief Called when an instance is added to the container.
		virtual void insert(scene::Instance* instance) = 0;
		/// \brief Called when an instance is removed from the container.
		virtual void erase(scene::Instance* instance) = 0;
	};

	class Visitor
	{
	public:
		virtual void visit(Instance& instance) const = 0;
	};

	/// \brief Returns a new instance uniquely identified by 'path'.
	virtual scene::Instance* create(const scene::Path& path, scene::Instance* parent) = 0;
	/// \brief Calls Visitor::visit(instance) for each instance in the container.
	virtual void forEachInstance(const Visitor& visitor) = 0;
	/// \brief Adds an instance to the container.
	virtual void insert(Observer* observer, const Path& path, scene::Instance* instance) = 0;
	/// \brief Returns an instance removed from the container.
	virtual scene::Instance* erase(Observer* observer, const Path& path) = 0;
};
typedef boost::shared_ptr<Instantiable> InstantiablePtr;

} // namespace scene

#endif /*IINSTANTIABLE_H_*/
