#include "Doom3AasFile.h"

#include "itextstream.h"
#include "string/convert.h"
#include "Util.h"

namespace map
{

void Doom3AasFile::parseFromTokens(parser::DefTokeniser& tok)
{
    while (tok.hasMoreTokens())
    {
        std::string token = tok.nextToken();

        if (token == "settings")
        {
            _settings.parseFromTokens(tok);
        }
        else if (token == "planes")
        {
            std::size_t planesCount = string::convert<std::size_t>(tok.nextToken());

            _planes.reserve(planesCount);

            tok.assertNextToken("{");

            // num ( a b c dist )
            for (std::size_t i = 0; i < planesCount; ++i)
            {
                string::convert<int>(tok.nextToken()); // plane index

                tok.assertNextToken("(");

                Plane3 plane;
                plane.normal().x() = string::convert<Vector3::ElementType>(tok.nextToken());
                plane.normal().y() = string::convert<Vector3::ElementType>(tok.nextToken());
                plane.normal().z() = string::convert<Vector3::ElementType>(tok.nextToken());
                plane.dist() = string::convert<Vector3::ElementType>(tok.nextToken());

                _planes.push_back(plane);

                tok.assertNextToken(")");
            }

            tok.assertNextToken("}");
        }
        else if (token == "vertices")
        {
            std::size_t vertCount = string::convert<std::size_t>(tok.nextToken());

            _vertices.reserve(vertCount);

            tok.assertNextToken("{");

            // num ( x y z )
            for (std::size_t i = 0; i < vertCount; ++i)
            {
                string::convert<int>(tok.nextToken()); // index
                _vertices.push_back(parseVector3(tok)); // components
            }

            tok.assertNextToken("}");
        }
        else if (token == "edges")
        {
            std::size_t edgeCount = string::convert<std::size_t>(tok.nextToken());

            _edges.reserve(edgeCount);

            tok.assertNextToken("{");

            // num ( vertIdx1 vertIdx2 )
            for (std::size_t i = 0; i < edgeCount; ++i)
            {
                string::convert<int>(tok.nextToken()); // index

                tok.assertNextToken("(");

                Edge edge;
                edge.vertexNumber[0] = string::convert<int>(tok.nextToken());
                edge.vertexNumber[1] = string::convert<int>(tok.nextToken());

                tok.assertNextToken(")");

                _edges.push_back(edge); // components
            }

            tok.assertNextToken("}");
        }
        else if (token == "edgeIndex")
        {
            parseIndex(tok, _edgeIndex);
        }
        else if (token == "faces")
        {
            std::size_t faceCount = string::convert<std::size_t>(tok.nextToken());

            _faces.reserve(faceCount);

            tok.assertNextToken("{");

            // num ( planeNum flags areas[0] areas[1] firstEdge numEdges )
            for (std::size_t i = 0; i < faceCount; ++i)
            {
                string::convert<int>(tok.nextToken()); // number

                tok.assertNextToken("(");

                Face face;

                face.planeNum = string::convert<int>(tok.nextToken());
                face.flags = string::convert<int>(tok.nextToken());
                face.areas[0] = string::convert<int>(tok.nextToken());
                face.areas[1] = string::convert<int>(tok.nextToken());
                face.firstEdge = string::convert<int>(tok.nextToken());
                face.numEdges = string::convert<int>(tok.nextToken());

                _faces.push_back(face);

                tok.assertNextToken(")");
            }

            tok.assertNextToken("}");
        }
        else if (token == "faceIndex")
        {
            parseIndex(tok, _faceIndex);
        }
        else if (token == "areas")
        {
            std::size_t areaCount = string::convert<std::size_t>(tok.nextToken());

            _areas.reserve(areaCount);

            tok.assertNextToken("{");

            // num ( flags contents firstFace numFaces cluster clusterAreaNum ) reachabilityCount { reachabilities }
            for (std::size_t i = 0; i < areaCount; ++i)
            {
                string::convert<int>(tok.nextToken()); // number

                tok.assertNextToken("(");

                Area area;

                area.flags = string::convert<int>(tok.nextToken());
                area.contents = string::convert<int>(tok.nextToken());
                area.firstFace = string::convert<int>(tok.nextToken());
                area.numFaces = string::convert<int>(tok.nextToken());
                area.cluster = string::convert<int>(tok.nextToken());
                area.clusterAreaNum = string::convert<int>(tok.nextToken());

                _areas.push_back(area);

                tok.assertNextToken(")");

                // Skip over reachabilities for the moment being
                std::size_t reachCount = string::convert<std::size_t>(tok.nextToken());
                tok.assertNextToken("{");

                while (tok.nextToken() != "}")
                {
                    // do nothing
                }
            }

            // Skip the step LinkReversedReachability();

            tok.assertNextToken("}");
        }
        // TODO: other sections
        else
        {
            throw parser::ParseException("Unknown token: " + token);
        }
    }
}

void Doom3AasFile::parseIndex(parser::DefTokeniser& tok, Doom3AasFile::Index& index)
{
    std::size_t idxCount = string::convert<std::size_t>(tok.nextToken());

    index.reserve(idxCount);

    tok.assertNextToken("{");

    // num ( idx )
    for (std::size_t i = 0; i < idxCount; ++i)
    {
        string::convert<int>(tok.nextToken()); // number

        tok.assertNextToken("(");
        index.push_back(string::convert<int>(tok.nextToken()));
        tok.assertNextToken(")");
    }

    tok.assertNextToken("}");
}

}
