#ifndef DOOM3MAPFORMAT_H_
#define DOOM3MAPFORMAT_H_

#include "imapformat.h"

namespace map {

	namespace {
		// The supported Doom3 map version 
		const int MAPVERSION = 2;

		// The version of the map info file
		const int MAP_INFO_VERSION = 2;
	}

class Doom3MapFormat : 
	public MapFormat
{
public:
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	
    /**
     * Read tokens from a map stream and create entities accordingly.
     */
    virtual bool readGraph(const MapImportInfo& importInfo) const;

	// Write scene graph to an ostream
	virtual void writeGraph(const MapExportInfo& exportInfo) const;

protected:
	// Helper functions to handle child brushes of func_statics which have to
	// be saved relative to their parent's origin
	void addOriginToChildPrimitives(const scene::INodePtr& root) const;
	void removeOriginFromChildPrimitives(const scene::INodePtr& root) const;

	// Post-process the imported map, loading layers and moving child primitives
	virtual void onMapParsed(const MapImportInfo& importInfo) const;
};
typedef boost::shared_ptr<Doom3MapFormat> Doom3MapFormatPtr;

} // namespace map

#endif /* DOOM3MAPFORMAT_H_ */
