#include "Import.h"

#include "imap.h"
#include "imapformat.h"
#include "inamespace.h"
#include "ientity.h"
#include "scene/BasicRootNode.h"
#include "map/algorithm/ChildPrimitives.h"
#include "map/algorithm/Merge.h"
#include "map/Map.h"
#include "scenelib.h"
#include "wxutil/dialog/MessageBox.h"

namespace map
{

namespace algorithm
{

void prepareNamesForImport(const scene::IMapRootNodePtr& targetRoot, const scene::INodePtr& foreignRoot)
{
    const auto& nspace = targetRoot->getNamespace();

    if (nspace)
    {
        // Prepare all names, but do not import them into the namespace. This
        // will happen when nodes are added to the target root later by the caller.
        nspace->ensureNoConflicts(foreignRoot);
    }
}

MapFormatPtr determineMapFormat(std::istream& stream, const std::string& type)
{
	// Get all registered map formats matching the extension
	auto availableFormats = type.empty() ?
		GlobalMapFormatManager().getAllMapFormats() :
		GlobalMapFormatManager().getMapFormatList(type);

	MapFormatPtr format;

	for (const auto& candidate : availableFormats)
	{
		// Rewind the stream before passing it to the format for testing
		// Map format valid, rewind the stream
		stream.seekg(0, std::ios_base::beg);

		if (candidate->canLoad(stream))
		{
			format = candidate;
			break;
		}
	}

	// Rewind the stream when we're done
	stream.seekg(0, std::ios_base::beg);

	return format;
}

MapFormatPtr determineMapFormat(std::istream& stream)
{
	return determineMapFormat(stream, std::string());
}

class SimpleMapImportFilter :
    public IMapImportFilter
{
private:
    scene::IMapRootNodePtr _root;

public:
    SimpleMapImportFilter() :
        _root(new scene::BasicRootNode)
    {}

    const scene::IMapRootNodePtr& getRootNode() const
    {
        return _root;
    }

    bool addEntity(const scene::INodePtr& entityNode)
    {
        _root->addChildNode(entityNode);
        return true;
    }

    bool addPrimitiveToEntity(const scene::INodePtr& primitive, const scene::INodePtr& entity)
    {
        if (Node_getEntity(entity)->isContainer())
        {
            entity->addChildNode(primitive);
            return true;
        }
        else
        {
            return false;
        }
    }
};

void importFromStream(std::istream& stream)
{
	GlobalSelectionSystem().setSelectedAll(false);

    // Instantiate the default import filter
    SimpleMapImportFilter importFilter;

    try
    {
        auto format = determineMapFormat(stream);

        if (!format)
        {
            throw IMapReader::FailureException(_("Unknown map format"));
        }

        auto reader = format->getMapReader(importFilter);

        // Start parsing
        reader->readFromStream(stream);

        // Prepare child primitives
        addOriginToChildPrimitives(importFilter.getRootNode());

        // Adjust all new names to fit into the existing map namespace
        prepareNamesForImport(GlobalMap().getRoot(), importFilter.getRootNode());

        MergeMap(importFilter.getRootNode());
    }
    catch (IMapReader::FailureException& ex)
    {
        wxutil::Messagebox::ShowError(fmt::format(_("Failure reading map from clipboard:\n{0}"), ex.what()));

        // Clear out the root node, otherwise we end up with half a map
        scene::NodeRemover remover;
        importFilter.getRootNode()->traverseChildren(remover);
    }
}

}

}
