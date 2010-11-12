#ifndef _DOOM3_PREFAB_FORMAT_H_
#define _DOOM3_PREFAB_FORMAT_H_

#include "Doom3MapFormat.h"

namespace map
{

/**
 * The prefab format is a specialised Doom3 Map Format, mainly to
 * ensure that no layer information is saved and loaded.
 */
class Doom3PrefabFormat :
	public Doom3MapFormat
{
public:
	// Override some RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual void initialiseModule(const ApplicationContext& ctx);

protected:
	virtual void onMapParsed(const MapImportInfo& importInfo) const;
};
typedef boost::shared_ptr<Doom3PrefabFormat> Doom3PrefabFormatPtr;

} // namespace

#endif /* _DOOM3_PREFAB_FORMAT_H_ */
