#pragma once

#include "imapformat.h"
#include "math/Matrix4.h"

namespace map
{

// A primitive parser for the "old" brushDef format
class BrushDefParser :
	public PrimitiveParser
{
public:
	const std::string& getKeyword() const;

    scene::INodePtr parse(parser::DefTokeniser& tok) const;
};
typedef std::shared_ptr<BrushDefParser> BrushDefParserPtr;

// For really old map formats, we don't even have the brushDef keyword
class LegacyBrushDefParser :
	public PrimitiveParser
{
public:
	const std::string& getKeyword() const;

    scene::INodePtr parse(parser::DefTokeniser& tok) const;

private:
    static Matrix4 getTexDef(float shiftS, float shiftT, float rotation, float scaleS, float scaleT);
};
typedef std::shared_ptr<LegacyBrushDefParser> LegacyBrushDefParserPtr;

}
