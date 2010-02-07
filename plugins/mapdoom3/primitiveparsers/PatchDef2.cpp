#include "PatchDef2.h"

#include "imap.h"
#include "ipatch.h"
#include "parser/DefTokeniser.h"
#include "string/string.h"

namespace map
{

const std::string& PatchDef2Parser::getKeyword() const
{
	static std::string _keyword("patchDef2");
	return _keyword;
}

/*
// Example Primitive
{
patchDef2
{
"textures/darkmod/stone/brick/rough_big_blocks03"
( 5 3 0 0 0 )
(
( ( 64 -88 108 0 0 ) ( 64 -88 184 0 -1.484375 ) ( 64 -88 184 0 -1.484375 ) )
( ( 64 -88 184 1.484375 0 ) ( 64 -88 184 1.484375 -1.484375 ) ( 64 -88 184 1.484375 -1.484375 ) )
( ( 112 -88 184 2.421875 0 ) ( 112 -88 184 2.421875 -1.484375 ) ( 112 -88 184 2.421875 -1.484375 ) )
( ( 160 -88 184 3.359375 0 ) ( 160 -88 184 3.359375 -1.484375 ) ( 160 -88 184 3.359375 -1.484375 ) )
( ( 160 -88 108 4.84375 0 ) ( 160 -88 184 4.84375 -1.484375 ) ( 160 -88 184 4.84375 -1.484375 ) )
)
}
}
*/
scene::INodePtr PatchDef2Parser::parse(parser::DefTokeniser& tok) const
{
	scene::INodePtr node = GlobalPatchCreator(DEF2).createPatch();

	IPatchNodePtr patchNode = boost::dynamic_pointer_cast<IPatchNode>(node);
	assert(patchNode != NULL);

	IPatch& patch = patchNode->getPatch();

	tok.assertNextToken("{");

	// Parse shader
	patch.setShader(tok.nextToken());

	// Parse parameters
	tok.assertNextToken("(");

	// parse matrix dimensions
	std::size_t cols = strToSizet(tok.nextToken());
	std::size_t rows = strToSizet(tok.nextToken());

	patch.setDims(cols, rows);
	
	// ignore contents/flags values
	tok.skipTokens(3);

	tok.assertNextToken(")");
	
	// Parse Patch Matrix
	parseMatrix(tok, patch);

	// Parse Footer
	tok.assertNextToken("}");
	tok.assertNextToken("}");

	patch.controlPointsChanged();

	return node;
}

} // namespace map
