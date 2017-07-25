#pragma once

#include "iaasfile.h"
#include "parser/DefTokeniser.h"
#include "Doom3AasFileSettings.h"
#include <vector>
#include "math/Plane3.h"
#include "math/AABB.h"

namespace map
{
    
class Doom3AasFile :
    public IAasFile
{
private:
    Doom3AasFileSettings _settings;

    std::vector<Plane3> _planes;
    std::vector<Vector3> _vertices;
    std::vector<Edge> _edges;

    typedef std::vector<int> Index;
    Index _edgeIndex;

    std::vector<Face> _faces;
    Index _faceIndex;

    std::vector<Area> _areas;

public:
    virtual std::size_t     getNumPlanes() const override;
    virtual const Plane3&   getPlane(std::size_t planeNum) const override;

    virtual std::size_t     getNumVertices() const override;
    virtual const Vector3&	getVertex(std::size_t vertexNum) const override;

    virtual std::size_t     getNumEdges() const override;
    virtual const Edge&     getEdge(std::size_t index) const override;

    virtual std::size_t     getNumEdgeIndexes() const override;
    virtual int 			getEdgeByIndex(int edgeIdx) const override;

    virtual std::size_t     getNumFaces() const override;
    virtual const Face&		getFace(int faceIndex) const override;
    virtual std::size_t     getNumFaceIndexes() const override;
    virtual int 			getFaceByIndex(int faceIdx) const override;

    virtual std::size_t     getNumAreas() const override;
    virtual const Area&     getArea(int areaNum) const override;

    void parseFromTokens(parser::DefTokeniser& tok);

private:
    void parseIndex(parser::DefTokeniser& tok, Index& index);
    void finishAreas();
    Vector3 calcReachableGoalForArea(const IAasFile::Area& area) const;
    Vector3 calcFaceCenter(int faceNum) const;
    Vector3 calcAreaCenter(const IAasFile::Area& area) const;
    AABB calcAreaBounds(const IAasFile::Area& area) const;
    AABB calcFaceBounds(int faceNum) const;
};
typedef std::shared_ptr<Doom3AasFile> Doom3AasFilePtr;

}
