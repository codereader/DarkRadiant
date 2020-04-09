#include "MapExporter.h"

#include <ostream>
#include "i18n.h"
#include "itextstream.h"
#include "ibrush.h"
#include "ipatch.h"
#include "ientity.h"
#include "imap.h"
#include "igroupnode.h"
#include "imainframe.h"
#include "../../brush/Brush.h"

#include "registry/registry.h"
#include "string/string.h"

#include "ChildPrimitives.h"

namespace map
{

	namespace
	{
		const char* const RKEY_FLOAT_PRECISION = "/mapFormat/floatPrecision";
		const char* const RKEY_MAP_SAVE_STATUS_INTERLEAVE = "user/ui/map/saveStatusInterleave";
	}

MapExporter::MapExporter(IMapWriter& writer, const scene::INodePtr& root, std::ostream& mapStream, std::size_t nodeCount) :
	_writer(writer),
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

MapExporter::MapExporter(IMapWriter& writer, const scene::INodePtr& root, 
				std::ostream& mapStream, std::ostream& auxStream, std::size_t nodeCount) :
	_writer(writer),
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
	if (_totalNodeCount > 0 && GlobalMainFrame().isActiveApp())
	{
		enableProgressDialog();
	}

	// Prepare the output stream
	game::IGamePtr curGame = GlobalGameManager().currentGame();
	assert(curGame != NULL);

	xml::NodeList nodes = curGame->getLocalXPath(RKEY_FLOAT_PRECISION);
	assert(!nodes.empty());

	int precision = string::convert<int>(nodes[0].getAttributeValue("value"));
	_mapStream.precision(precision);

	// Add origin to func_* children before writing
	prepareScene();
}

void MapExporter::exportMap(const scene::INodePtr& root, const GraphTraversalFunc& traverse)
{
	try
	{
		auto mapRoot = std::dynamic_pointer_cast<scene::IMapRootNode>(root);

		if (!mapRoot)
		{
			throw std::logic_error("Map node is not a scene::IMapRootNode");
		}

		_writer.beginWriteMap(mapRoot, _mapStream);

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

		_writer.endWriteMap(mapRoot, _mapStream);

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

void MapExporter::enableProgressDialog()
{
	_dialog.reset(new wxutil::ModalProgressDialog(_("Writing map")));
}

void MapExporter::disableProgressDialog()
{
	_dialog.reset();
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
			
			_writer.beginWriteEntity(entity, _mapStream);

			if (_infoFileExporter) _infoFileExporter->visitEntity(node, _entityNum);

			return true;
		}

		auto brush = std::dynamic_pointer_cast<IBrushNode>(node);

		if (brush && brush->getIBrush().hasContributingFaces())
		{
			// Progress dialog handling
			onNodeProgress();

			_writer.beginWriteBrush(brush, _mapStream);

			if (_infoFileExporter) _infoFileExporter->visitPrimitive(node, _entityNum, _primitiveNum);

			return true;
		}

		auto patch = std::dynamic_pointer_cast<IPatchNode>(node);

		if (patch)
		{
			// Progress dialog handling
			onNodeProgress();

			_writer.beginWritePatch(patch, _mapStream);

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
			_writer.endWriteEntity(entity, _mapStream);

			_entityNum++;
			return;
		}

		auto brush = std::dynamic_pointer_cast<IBrushNode>(node);

		if (brush && brush->getIBrush().hasContributingFaces())
		{
			_writer.endWriteBrush(brush, _mapStream);
			_primitiveNum++;
			return;
		}

		auto patch = std::dynamic_pointer_cast<IPatchNode>(node);

		if (patch)
		{
			_writer.endWritePatch(patch, _mapStream);
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
	if (_dialog && _dialogEventLimiter.readyForEvent())
	{
		std::string text = fmt::format(_("Writing node {0:d}"), _curNodeCount);
		_dialog->setTextAndFraction(
			text, 
			static_cast<double>(_curNodeCount) / static_cast<double>(_totalNodeCount)
		);
	}
}

void MapExporter::prepareScene()
{
	removeOriginFromChildPrimitives(_root);

	// Re-evaluate all brushes, to update the Winding calculations
	recalculateBrushWindings();
}

void MapExporter::finishScene()
{
	addOriginToChildPrimitives(_root);

	// Re-evaluate all brushes, to update the Winding calculations
	recalculateBrushWindings();
}

void MapExporter::recalculateBrushWindings()
{
	_root->foreachNode([] (const scene::INodePtr& child)->bool
	{
		Brush* brush = Node_getBrush(child);

		if (brush != NULL)
		{
			brush->evaluateBRep();
		}

		return true;
	});
}

} // namespace
