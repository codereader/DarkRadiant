#include "MapExporter.h"

#include <ostream>
#include "i18n.h"
#include "itextstream.h"
#include "ibrush.h"
#include "ipatch.h"
#include "ientity.h"
#include "imapresource.h"
#include "imap.h"
#include "igroupnode.h"

#include "registry/registry.h"
#include "string/string.h"

#include "ChildPrimitives.h"
#include "messages/MapFileOperation.h"

namespace map
{

	namespace
	{
		const char* const RKEY_FLOAT_PRECISION = "/mapFormat/floatPrecision";
		const char* const RKEY_MAP_SAVE_STATUS_INTERLEAVE = "user/ui/map/saveStatusInterleave";
	}

MapExporter::MapExporter(const MapFormat& format, const scene::IMapRootNodePtr& root, std::ostream& mapStream, std::size_t nodeCount) :
	_writer(format.getMapWriter()),
	_mapStream(mapStream),
	_root(root),
	_dialogEventLimiter(registry::getValue<int>(RKEY_MAP_SAVE_STATUS_INTERLEAVE)),
	_totalNodeCount(nodeCount),
	_curNodeCount(0),
	_entityNum(0),
	_primitiveNum(0)
{
	construct();
}

MapExporter::MapExporter(const MapFormat& format, const scene::IMapRootNodePtr& root,
				std::ostream& mapStream, std::ostream& auxStream, std::size_t nodeCount) :
	_writer(format.getMapWriter()),
	_mapStream(mapStream),
	_infoFileExporter(new InfoFileExporter(auxStream)),
	_root(root),
	_dialogEventLimiter(registry::getValue<int>(RKEY_MAP_SAVE_STATUS_INTERLEAVE)),
	_totalNodeCount(nodeCount),
	_curNodeCount(0),
	_entityNum(0),
	_primitiveNum(0)
{
	construct();
}

MapExporter::~MapExporter()
{
	// Close any info file stream
	_infoFileExporter.reset();

	// The finish() call is placed in the destructor to make sure that 
	// even on unhandled exceptions the map is left in a working state
	finishScene();
}

void MapExporter::construct()
{
	// Prepare the output stream
	game::IGamePtr curGame = GlobalGameManager().currentGame();
	assert(curGame);

	xml::NodeList nodes = curGame->getLocalXPath(RKEY_FLOAT_PRECISION);
	assert(!nodes.empty());

	int precision = string::convert<int>(nodes[0].getAttributeValue("value"));
	_mapStream.precision(precision);

	// Add origin to func_* children before writing
	prepareScene();
}

void MapExporter::exportMap(const scene::INodePtr& root, const GraphTraversalFunc& traverse)
{
	FileOperation startedMsg(FileOperation::Type::Export, FileOperation::Started, _totalNodeCount > 0);
	GlobalRadiantCore().getMessageBus().sendMessage(startedMsg);

	try
	{
		auto mapRoot = std::dynamic_pointer_cast<scene::IMapRootNode>(root);

		if (!mapRoot)
		{
			throw std::logic_error("Map node is not a scene::IMapRootNode");
		}

		_writer->beginWriteMap(mapRoot, _mapStream);

		if (_infoFileExporter)
		{
			_infoFileExporter->beginSaveMap(mapRoot);
		}
	}
	catch (IMapWriter::FailureException& ex)
	{
		rError() << "Failure exporting a node (pre): " << ex.what() << std::endl;
	}

	// Perform the actual map traversal
	traverse(root, *this);

	try
	{
		auto mapRoot = std::dynamic_pointer_cast<scene::IMapRootNode>(root);

		if (!mapRoot)
		{
			throw std::logic_error("Map node is not a scene::IMapRootNode");
		}

		_writer->endWriteMap(mapRoot, _mapStream);

		if (_infoFileExporter)
		{
			_infoFileExporter->finishSaveMap(mapRoot);
		}
	}
	catch (IMapWriter::FailureException& ex)
	{
		rError() << "Failure exporting a node (pre): " << ex.what() << std::endl;
	}

	// finishScene() is handled through the destructor
}

bool MapExporter::pre(const scene::INodePtr& node)
{
	try
	{
		auto entity = std::dynamic_pointer_cast<IEntityNode>(node);

		if (entity)
		{
			// Progress dialog handling
			onNodeProgress();
			
			_writer->beginWriteEntity(entity, _mapStream);

			if (_infoFileExporter) _infoFileExporter->visitEntity(node, _entityNum);

			return true;
		}

		auto brush = std::dynamic_pointer_cast<IBrushNode>(node);

		if (brush && brush->getIBrush().hasContributingFaces())
		{
			// Progress dialog handling
			onNodeProgress();

			_writer->beginWriteBrush(brush, _mapStream);

			if (_infoFileExporter) _infoFileExporter->visitPrimitive(node, _entityNum, _primitiveNum);

			return true;
		}

		auto patch = std::dynamic_pointer_cast<IPatchNode>(node);

		if (patch)
		{
			// Progress dialog handling
			onNodeProgress();

			_writer->beginWritePatch(patch, _mapStream);

			if (_infoFileExporter) _infoFileExporter->visitPrimitive(node, _entityNum, _primitiveNum);

			return true;
		}
	}
	catch (IMapWriter::FailureException& ex)
	{
		rError() << "Failure exporting a node (pre): " << ex.what() << std::endl;
	}

	return true; // full traversal
}

void MapExporter::post(const scene::INodePtr& node)
{
	try
	{
		auto entity = std::dynamic_pointer_cast<IEntityNode>(node);

		if (entity)
		{
			_writer->endWriteEntity(entity, _mapStream);

			_entityNum++;
			return;
		}

		auto brush = std::dynamic_pointer_cast<IBrushNode>(node);

		if (brush && brush->getIBrush().hasContributingFaces())
		{
			_writer->endWriteBrush(brush, _mapStream);
			_primitiveNum++;
			return;
		}

		auto patch = std::dynamic_pointer_cast<IPatchNode>(node);

		if (patch)
		{
			_writer->endWritePatch(patch, _mapStream);
			_primitiveNum++;
			return;
		}
	}
	catch (IMapWriter::FailureException& ex)
	{
		rError() << "Failure exporting a node (post): " << ex.what() << std::endl;
	}
}

void MapExporter::onNodeProgress()
{
	_curNodeCount++;

	// Update the dialog text. This will throw an exception if the cancel
	// button is clicked, which we must catch and handle.
	if (_dialogEventLimiter.readyForEvent())
	{
		float progressFraction = _totalNodeCount > 0 ? 
			static_cast<float>(_curNodeCount) / static_cast<float>(_totalNodeCount) : 0.0f;

		FileOperation msg(FileOperation::Type::Export, FileOperation::Progress, _totalNodeCount > 0, progressFraction);
		msg.setText(fmt::format(_("Writing node {0:d}"), _curNodeCount));

		GlobalRadiantCore().getMessageBus().sendMessage(msg);
	}
}

void MapExporter::prepareScene()
{
	removeOriginFromChildPrimitives(_root);

	// Re-evaluate all brushes, to update the Winding calculations
	recalculateBrushWindings();

	// Emit the pre-export event to give subscribers a chance to prepare the scene
	GlobalMapResourceManager().signal_onResourceExporting().emit(_root);
}

void MapExporter::finishScene()
{
	// Emit the post-export event to give subscribers a chance to cleanup the scene
	GlobalMapResourceManager().signal_onResourceExported().emit(_root);

	addOriginToChildPrimitives(_root);

	// Re-evaluate all brushes, to update the Winding calculations
	recalculateBrushWindings();

	FileOperation finishedMsg(FileOperation::Type::Export, FileOperation::Finished, _totalNodeCount > 0);
	GlobalRadiantCore().getMessageBus().sendMessage(finishedMsg);
}

void MapExporter::recalculateBrushWindings()
{
	_root->foreachNode([] (const scene::INodePtr& child)->bool
	{
		auto* brush = Node_getIBrush(child);

		if (brush != nullptr)
		{
			brush->evaluateBRep();
		}

		return true;
	});
}

} // namespace
