// greebo: Specialise the boost::lexical_cast<float>() function
#define SPECIALISE_STR_TO_FLOAT

#include "PatchDef3.h"

#include "imap.h"
#include "ipatch.h"
#include "parser/DefTokeniser.h"
#include "string/convert.h"

namespace map
{

const std::string& PatchDef3Parser::getKeyword() const
{
	static std::string _keyword("patchDef3");
	return _keyword;
}

/*
// Example Primitive
{
patchDef3
{
"textures/darkmod/nature/skybox/starry1/skyfade"
( 5 5 4 4 0 0 0 )
(
( ( 4288 1152 1824 0.5 0.5 ) ( 4288 1088 1952 0.25 0.5 ) ( 4288 1024 1952 0 0.5 ) ( 4288 960 1952 -0.25 0.5 ) ( 4288 896 1824 -0.5 0.5 ) )
( ( 4352 1152 1952 0.5 0.25 ) ( 4352 1088 2080 0.25 0.25 ) ( 4352 1024 2080 0 0.25 ) ( 4352 960 2080 -0.25 0.25 ) ( 4352 896 1952 -0.5 0.25 ) )
( ( 4416 1152 1952 0.5 0 ) ( 4416 1088 2080 0.25 0 ) ( 4416 1024 2080 0 0 ) ( 4416 960 2080 -0.25 0 ) ( 4416 896 1952 -0.5 0 ) )
( ( 4480 1152 1952 0.5 -0.25 ) ( 4480 1088 2080 0.25 -0.25 ) ( 4480 1024 2080 0 -0.25 ) ( 4480 960 2080 -0.25 -0.25 ) ( 4480 896 1952 -0.5 -0.25 ) )
( ( 4544 1152 1824 0.5 -0.5 ) ( 4544 1088 1952 0.25 -0.5 ) ( 4544 1024 1952 0 -0.5 ) ( 4544 960 1952 -0.25 -0.5 ) ( 4544 896 1824 -0.5 -0.5 ) )
)
}
}
*/
scene::INodePtr PatchDef3Parser::parse(parser::DefTokeniser& tok) const
{
	scene::INodePtr node = GlobalPatchCreator(DEF3).createPatch();

	IPatchNodePtr patchNode = boost::dynamic_pointer_cast<IPatchNode>(node);
	assert(patchNode != NULL);

	IPatch& patch = patchNode->getPatch();

	tok.assertNextToken("{");

	// Parse shader
	patch.setShader(tok.nextToken());

	// Parse parameters
	tok.assertNextToken("(");

	std::size_t cols = string::convert<std::size_t>(tok.nextToken());
	std::size_t rows = string::convert<std::size_t>(tok.nextToken());

	patch.setDims(cols, rows);

	// Parse fixed tesselation
	std::size_t subdivX = string::convert<std::size_t>(tok.nextToken());
	std::size_t subdivY = string::convert<std::size_t>(tok.nextToken());

	patch.setFixedSubdivisions(true, Subdivisions(subdivX, subdivY));

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
