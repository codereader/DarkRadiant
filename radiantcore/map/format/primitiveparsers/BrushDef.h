#pragma once

#include "ibrush.h"
#include "imapformat.h"
#include "math/Matrix3.h"

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

// For really old map formats, we don't even have the brushDef keyword
class LegacyBrushDefParser :
	public PrimitiveParser
{
public:
	const std::string& getKeyword() const;

    scene::INodePtr parse(parser::DefTokeniser& tok) const;

private:
    static Matrix3 calculateTextureMatrix(const std::string& shader, const Vector3& normal, const ShiftScaleRotation& ssr);
};

}
