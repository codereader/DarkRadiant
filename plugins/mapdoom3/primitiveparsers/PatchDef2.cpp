#include "PatchDef2.h"

#include "imap.h"
#include "ipatch.h"
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

const std::string& PatchDef2Parser::getKeyword() const
{
	static std::string _keyword("patchDef2");
	return _keyword;
}

scene::INodePtr PatchDef2Parser::parse(parser::DefTokeniser& tok) const
{
	scene::INodePtr patch = GlobalPatchCreator(DEF2).createPatch();

	assert(patch != NULL);

	Node_getMapImporter(patch)->importTokens(tok);

	return patch;
}

} // namespace map
