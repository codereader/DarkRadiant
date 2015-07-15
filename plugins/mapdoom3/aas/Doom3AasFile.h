#pragma once

#include "iaasfile.h"
#include "parser/DefTokeniser.h"
#include "Doom3AasFileSettings.h"
#include <vector>
#include "math/Plane3.h"

namespace map
{
    
class Doom3AasFile :
    public IAasFile
{
private:
    Doom3AasFileSettings _settings;

    std::vector<Plane3> _planes;
    std::vector<Vector3> _vertices;

    // An edge references two vertices by index
    struct Edge
    {
        int vertexNumber[2];
    };
    std::vector<Edge> _edges;

    typedef std::vector<int> Index;

    Index _edgeIndex;

    struct Face 
    {
	    int 				planeNum;   // number of the plane this face is on
	    unsigned short		flags;      // face flags
	    int					numEdges;   // number of edges in the boundary of the face
	    int					firstEdge;  // first edge in the edge index
	    short				areas[2];   // area at the front and back of this face
    };
    std::vector<Face> _faces;
    Index _faceIndex;

    struct Area
    {
        int             numFaces;			 // number of faces used for the boundary of the area
        int             firstFace;			 // first face in the face index used for the boundary of the area
        AABB            bounds;				 // bounds of the area
        Vector3         center;				 // center of the area an AI can move towards
        unsigned short  flags;				 // several area flags
        unsigned short  contents;			 // contents of the area
        short           cluster;			 // cluster the area belongs to, if negative it's a portal
        short           clusterAreaNum;		 // number of the area in the cluster
        int             travelFlags;		 // travel flags for traveling through this area
        //Reachability*   reach = nullptr;	 // reachabilities that start from this area
        //Reachability*   rev_reach = nullptr; // reachabilities that lead to this area
    };
    std::vector<Area> _areas;

public:
    void parseFromTokens(parser::DefTokeniser& tok);

private:
    void parseIndex(parser::DefTokeniser& tok, Index& index);
};
typedef std::shared_ptr<Doom3AasFile> Doom3AasFilePtr;

}
