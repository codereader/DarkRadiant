#include "PatchDef3.h"

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

const std::string& PatchDef3Parser::getKeyword() const
{
	static std::string _keyword("patchDef3");
	return _keyword;
}

scene::INodePtr PatchDef3Parser::parse(parser::DefTokeniser& tok) const
{
	scene::INodePtr patch = GlobalPatchCreator(DEF3).createPatch();

	assert(patch != NULL);

	Node_getMapImporter(patch)->importTokens(tok);

	return patch;
}

} // namespace map
