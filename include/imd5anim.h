#pragma once

#include "imodule.h"

namespace md5
{

/** 
 * Base class for an MD5 animation as used in Doom 3.
 */
class IMD5Anim 
{
public:
};
typedef boost::shared_ptr<IMD5Anim> IMD5AnimPtr;

class IAnimationCache :
	public RegisterableModule
{
public:
	/**
	 * Returns the MD5 animation for the given VFS path, or NULL if 
	 * the file does not exist or the anim was found to be invalid.
	 */
	virtual IMD5AnimPtr getAnim(const std::string& vfsPath) = 0;
};

const char* const MODULE_ANIMATIONCACHE("MD5AnimationCache");

} // namespace

inline md5::IAnimationCache& GlobalAnimationCache()
{
	// Cache the reference locally
	static md5::IAnimationCache& _animationCache(
		*boost::static_pointer_cast<md5::IAnimationCache>(
			module::GlobalModuleRegistry().getModule(md5::MODULE_ANIMATIONCACHE)
		)
	);
	return _animationCache;
}
