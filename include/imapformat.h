#ifndef _imapformat_h__
#define _imapformat_h__

#include "imodule.h"

namespace scene
{
class NodeVisitor;
class INode;
typedef boost::shared_ptr<INode> INodePtr;
}

/** Callback function to control how the Walker traverses the scene graph. This function
 * will be provided to the map export module by the Radiant map code.
 */
typedef void (*GraphTraversalFunc) (scene::INodePtr root, scene::NodeVisitor& nodeExporter);

namespace map
{

// The MapExport/MapImport information structures, needed by the MapFormat::write() and
// MapFormat::read() methods. The actual definition of this structure is stored in the 
// files libs/MapExportInfo.h and libs/MapImportInfo.h.
class MapExportInfo;
class MapImportInfo;

/** 
 * Map Format interface. Each map format is able to traverse the scene graph and write
 * the contents into a mapfile, or to load a mapfile and populate a scene graph.
 */
class MapFormat :
	public RegisterableModule
{
public:
    virtual ~MapFormat() {}
	/**
	 * Read the contents of the given streams (which are contained in MapImportInfo)
	 * and add them as children to the given root node (also in MapImportInfo).
	 *
	 * @returns: TRUE on success, FALSE if parsing errors occurred.
	 */
	virtual bool readGraph(const map::MapImportInfo& importInfo) const = 0;

	/** Traverse the scene graph and write contents into the provided output stream.
	 * 
	 * @param exportInfo
	 * The MapExportInfo structure, which contains the ostream references and such.
	 */
	virtual void writeGraph(const map::MapExportInfo& exportInfo) const = 0;
};
typedef boost::shared_ptr<MapFormat> MapFormatPtr;


} // namespace map

#endif /* _imapformat_h__ */
