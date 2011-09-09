#pragma once

#include <boost/shared_ptr.hpp>
#include "imd5anim.h"

namespace md5
{

/**
 * Interface for a MD5 model object. This can be used
 * to control the animation applied to the MD5.
 */
class IMD5Model
{
public:
	/**
	 * Set the animation to play on this model.
	 * Pass a NULL animation to clear any anim.
	 */
	virtual void setAnim(const IMD5AnimPtr& anim) = 0;

	/**
	 * Returns the currently applied animation.
	 */
	virtual const IMD5AnimPtr& getAnim() const = 0;

	/**
	 * Update the mesh using the given animation playback time.
	 */
	virtual void updateAnim(std::size_t time) = 0;
};
typedef boost::shared_ptr<IMD5Model> IMD5ModelPtr;

} // namespace
