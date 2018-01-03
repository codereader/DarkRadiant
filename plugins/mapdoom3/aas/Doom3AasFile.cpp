#include "Doom3AasFile.h"

#include "itextstream.h"
#include "string/convert.h"
#include "Util.h"

namespace map
{

// area flags
#define AREA_FLOOR					(1 << 0)		// AI can stand on the floor in this area
#define AREA_GAP					(1 << 1)		// area has a gap
#define AREA_LEDGE					(1 << 2)		// if entered the AI bbox partly floats above a ledge
#define AREA_LADDER					(1 << 3)		// area contains one or more ladder faces
#define AREA_LIQUID					(1 << 4)		// area contains a liquid
#define AREA_CROUCH					(1 << 5)		// AI cannot walk but can only crouch in this area
#define AREA_REACHABLE_WALK			(1 << 6)		// area is reachable by walking or swimming
#define AREA_REACHABLE_FLY			(1 << 7)		// area is reachable by flying
#define AREA_DOOR					(1 << 8)		// area contains one ore more doors

// face flags
#define FACE_SOLID					(1 << 0)		// solid at the other side
#define FACE_LADDER					(1 << 1)		// ladder surface
#define FACE_FLOOR					(1 << 2)		// standing on floor when on this face
#define FACE_LIQUID					(1 << 3)		// face seperating two areas with liquid
#define FACE_LIQUIDSURFACE			(1 << 4)		// face seperating liquid and air

std::size_t Doom3AasFile::getNumPlanes() const
{
    return _planes.size();
}

const Plane3& Doom3AasFile::getPlane(std::size_t planeNum) const
{
    return _planes[planeNum];
}

std::size_t Doom3AasFile::getNumVertices() const
{
    return _vertices.size();
}

const Vector3& Doom3AasFile::getVertex(std::size_t vertexNum) const
{
    return _vertices[vertexNum];
}

std::size_t Doom3AasFile::getNumEdges() const
{
    return _edges.size();
}

const IAasFile::Edge& Doom3AasFile::getEdge(std::size_t index) const
{
    return _edges[index];
}

std::size_t Doom3AasFile::getNumEdgeIndexes() const
{
    return _edgeIndex.size();
}

int Doom3AasFile::getEdgeByIndex(int edgeIdx) const
{
    return _edgeIndex[edgeIdx];
}

std::size_t Doom3AasFile::getNumFaces() const
{
    return _faces.size();
}

const IAasFile::Face& Doom3AasFile::getFace(int faceIndex) const
{
    return _faces[faceIndex];
}

std::size_t Doom3AasFile::getNumFaceIndexes() const
{
    return _faceIndex.size();
}

int Doom3AasFile::getFaceByIndex(int faceIdx) const
{
    return _faceIndex[faceIdx];
}

std::size_t Doom3AasFile::getNumAreas() const
{
    return _areas.size();
}

const IAasFile::Area& Doom3AasFile::getArea(int areaNum) const
{
    return _areas[areaNum];
}

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
                face.flags = string::convert<unsigned short>(tok.nextToken());
                face.areas[0] = string::convert<short>(tok.nextToken());
                face.areas[1] = string::convert<short>(tok.nextToken());
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

                area.flags = string::convert<unsigned short>(tok.nextToken());
                area.contents = string::convert<unsigned short>(tok.nextToken());
                area.firstFace = string::convert<int>(tok.nextToken());
                area.numFaces = string::convert<int>(tok.nextToken());
                area.cluster = string::convert<short>(tok.nextToken());
                area.clusterAreaNum = string::convert<short>(tok.nextToken());

                _areas.push_back(area);

                tok.assertNextToken(")");

                // Skip over reachabilities for the moment being
                /*std::size_t reachCount = */string::convert<std::size_t>(tok.nextToken());
                tok.assertNextToken("{");

                while (tok.nextToken() != "}")
                {
                    // do nothing
                }
            }

            // Skip the step LinkReversedReachability();

            tok.assertNextToken("}");
        }
        else if (token == "nodes" || token == "portals" || token == "portalIndex" || token == "clusters")
        {
            tok.nextToken(); // integer
            tok.assertNextToken("{");

            while (tok.nextToken() != "}")
            {
                // do nothing
            }
        }
        else
        {
            throw parser::ParseException("Unknown token: " + token);
        }
    }

    finishAreas();
}

void Doom3AasFile::finishAreas()
{
    for (Area& area : _areas)
    {
        area.center = calcReachableGoalForArea(area);
		area.bounds = calcAreaBounds(area);
    }
}

#define INTSIGNBITSET(i)		(((const unsigned int)(i)) >> 31)

AABB Doom3AasFile::calcFaceBounds(int faceNum) const
{
	AABB bounds;

	const Face& face = _faces[faceNum];

	for (int i = 0; i < face.numEdges; i++)
    {
		int edgeNum = _edgeIndex[face.firstEdge + i];
		const Edge& edge = _edges[abs(edgeNum)];

		bounds.includePoint(_vertices[edge.vertexNumber[INTSIGNBITSET(edgeNum)]]);
	}
	return bounds;
}

AABB Doom3AasFile::calcAreaBounds(const IAasFile::Area& area) const
{
	AABB bounds;

	for (int i = 0; i < area.numFaces; i++)
    {
		int faceNum = _faceIndex[area.firstFace + i];
		bounds.includeAABB(calcFaceBounds(abs(faceNum)));
	}

	return bounds;
}

Vector3 Doom3AasFile::calcFaceCenter(int faceNum) const
{
	Vector3 center(0,0,0);

	const Face& face = _faces[faceNum];

	if (face.numEdges > 0)
    {
		for (int i = 0; i < face.numEdges; i++)
        {
			int edgeNum = _edgeIndex[face.firstEdge + i];
			const Edge& edge = _edges[abs(edgeNum)];

			center += _vertices[edge.vertexNumber[INTSIGNBITSET(edgeNum)]];
		}
		center /= face.numEdges;
	}

	return center;
}

Vector3 Doom3AasFile::calcAreaCenter(const IAasFile::Area& area) const
{
	Vector3 center(0,0,0);

	if (area.numFaces > 0)
    {
		for (int i = 0; i < area.numFaces; i++)
        {
			int faceNum = _faceIndex[area.firstFace + i];
			center += calcFaceCenter(abs(faceNum));
		}

		center /= area.numFaces;
	}

	return center;
}

Vector3 Doom3AasFile::calcReachableGoalForArea(const IAasFile::Area& area) const
{
	if (!(area.flags & (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY)) || (area.flags & AREA_LIQUID))
    {
		return calcAreaCenter(area);
	}

    Vector3 center(0,0,0);

	int numFaces = 0;

	for (int i = 0; i < area.numFaces; i++)
    {
		int faceNum = _faceIndex[area.firstFace + i];

		if (!(_faces[abs(faceNum)].flags & FACE_FLOOR))
        {
			continue;
		}

		center += calcFaceCenter(abs(faceNum));
		numFaces++;
	}

	if (numFaces > 0)
    {
		center /= numFaces;
	}

    // No downward trace here

    return center;
}

void Doom3AasFile::parseIndex(parser::DefTokeniser& tok, Index& index)
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
