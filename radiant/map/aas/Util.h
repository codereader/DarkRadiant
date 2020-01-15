#pragma once

#include "math/Vector3.h"
#include "parser/DefTokeniser.h"
#include "string/convert.h"

namespace map
{
    inline Vector3 parseVector3(parser::DefTokeniser& tok)
    {
        Vector3 vec;

        tok.assertNextToken("(");
        vec[0] = string::convert<Vector3::ElementType>(tok.nextToken());
        vec[1] = string::convert<Vector3::ElementType>(tok.nextToken());
        vec[2] = string::convert<Vector3::ElementType>(tok.nextToken());
        tok.assertNextToken(")");

        return vec;
    }
}
