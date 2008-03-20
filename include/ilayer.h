#ifndef ILAYER_H_
#define ILAYER_H_

#include <set>
#include <string>

namespace scene {

// A list of named layers
typedef std::set<std::string> LayerList;

/** 
 * greebo: Interface of a Layered object.
 */
class Layered
{
public:
	/**
     * Add this object to the named layer.
     */
    virtual void addToLayer(const std::string& layer) = 0;

    /**
     * Remove this object from the named layer.
     */
    virtual void removeFromLayer(const std::string& layer) = 0;

    /**
     * Return the set of layers to which this object is assigned.
     */
    virtual LayerList getLayers() const = 0;
};

} // namespace scene

#endif /*ILAYER_H_*/
