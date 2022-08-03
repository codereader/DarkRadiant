#include "MD5AnimationCache.h"

#include "iarchive.h"
#include "ifilesystem.h"
#include "itextstream.h"
#include "parser/DefTokeniser.h"

namespace md5
{

IMD5AnimPtr MD5AnimationCache::getAnim(const std::string& vfsPath)
{
	// Check the cache first
	AnimationMap::iterator found = _animations.find(vfsPath);

	if (found != _animations.end())
	{
		return found->second;
	}

	// Not found, construct new animation with the given path
	ArchiveTextFilePtr file = GlobalFileSystem().openTextFile(vfsPath);

	if (file == NULL)
	{
		rWarning() << "Animation file " << vfsPath << " does not exist." << std::endl;
		return IMD5AnimPtr();
	}

	std::istream inputStream(&file->getInputStream());
	
	// Create the anim from scratch
	MD5AnimPtr anim(new MD5Anim);
	anim->parseFromStream(inputStream);

	// Store the anim in our cache
	_animations.insert(AnimationMap::value_type(vfsPath, anim));

	return anim;
}

const std::string& MD5AnimationCache::getName() const
{
	static std::string _name(MODULE_ANIMATIONCACHE);
	return _name;
}

const StringSet& MD5AnimationCache::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
	}

	return _dependencies;
}

void MD5AnimationCache::initialiseModule(const IApplicationContext& ctx)
{
}

void MD5AnimationCache::shutdownModule()
{
	_animations.clear();
}

} // namespace
