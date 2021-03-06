#ifndef _BSPCOMMON_H_
#define _BSPCOMMON_H_

#include <vector>

#include "glsl/glsl.h"
#include "globals.h"
#include "plane.h"
#include "bbox.h"
#include "mesh.h"

class C_BspNode;
class C_BspTree;

#define BACK		   0
#define FRONT		   1
#define INTERSECTS	2
#define COINCIDENT	3

extern int nConvexRooms;

struct poly_t {
	C_Vertex       *pVertices;
	C_Vertex       *pNorms;
	int            nVertices;
	bool           usedAsDivider;
};

struct brush_t {
	poly_t         *pPolys;
	int            nPolys;
};

typedef struct {
   C_MeshGroup    mesh;
   unsigned int   meshID;
   bool           drawn;
//   C_BBox         bbox;
} staticTreeObject_t;

#endif
