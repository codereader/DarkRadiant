#include "BrushDef3.h"

#include "imap.h"
#include "ibrush.h"
#include "parser/DefTokeniser.h"

namespace map
{

namespace
{
	inline MapImporterPtr Node_getMapImporter(const scene::INodePtr& node)
	{
		return boost::dynamic_pointer_cast<MapImporter>(node);
	}
}

const std::string& BrushDef3Parser::getKeyword() const
{
	static std::string _keyword("brushDef3");
	return _keyword;
}

scene::INodePtr BrushDef3Parser::parse(parser::DefTokeniser& tok) const
{
	scene::INodePtr brush = GlobalBrushCreator().createBrush();

	assert(brush != NULL);

	Node_getMapImporter(brush)->importTokens(tok);

	return brush;
}

} // namespace map
