#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "objfile.h"
#include "../tgaLoader/texture.h"

void glmFirstPass(GLMmodel* model, FILE* file);
void glmSecondPass(GLMmodel* model, FILE* file);
GLMgroup* glmAddGroup(GLMmodel* model, const char* name);
GLMgroup* glmFindGroup(GLMmodel* model, const char* name);
void glmDelete(GLMmodel* model);
static void glmReadMTL(GLMmodel* model, char* name);
char* glmDirName(char* path);
unsigned int glmFindMaterial(GLMmodel* model, char* name);

#define T(x) (model->triangles[(x)])

static void
calculateTBN(GLMmodel *model, GLMgroup *group)
{
   unsigned int i;
   int vindex1, vindex2, vindex3;
   int tindex1, tindex2, tindex3;
   int *counters = NULL;
   C_TexCoord dUV1, dUV2;
   C_Vertex dV1, dV2;
   float rcp;

   assert(model->vertices);
   assert(model->texcoords);

   counters = new int[model->numvertices + 1];
   memset(counters, 0, (model->numvertices + 1) * sizeof(int));

   model->tangents = (float *)malloc(3 * (model->numvertices + 1) * sizeof(float));
   assert(model->tangents);
   memset(model->tangents, 0, 3 * (model->numvertices + 1) * sizeof(float));

   model->binormals = (float *)malloc(3 * (model->numvertices + 1)* sizeof(float));
   assert(model->binormals);
   memset(model->binormals, 0, 3 * (model->numvertices + 1) * sizeof(float));

   for(i = 0; i < group->numtriangles; i++) {
      vindex1 = model->triangles[group->triangles[i]].vindices[0];
      vindex2 = model->triangles[group->triangles[i]].vindices[1];
      vindex3 = model->triangles[group->triangles[i]].vindices[2];

      tindex1 = model->triangles[group->triangles[i]].tindices[0];
      tindex2 = model->triangles[group->triangles[i]].tindices[1];
      tindex3 = model->triangles[group->triangles[i]].tindices[2];

      dUV1.u = model->texcoords[2 * tindex2    ] - model->texcoords[2 * tindex1    ];
      dUV1.v = model->texcoords[2 * tindex2 + 1] - model->texcoords[2 * tindex1 + 1];

      dUV2.u = model->texcoords[2 * tindex3    ] - model->texcoords[2 * tindex1    ];
      dUV2.v = model->texcoords[2 * tindex3 + 1] - model->texcoords[2 * tindex1 + 1];

      dV1.x = model->vertices[3 * vindex2    ] - model->vertices[3 * vindex1    ];
      dV1.y = model->vertices[3 * vindex2 + 1] - model->vertices[3 * vindex1 + 1];
      dV1.z = model->vertices[3 * vindex2 + 2] - model->vertices[3 * vindex1 + 2];

      dV2.x = model->vertices[3 * vindex3    ] - model->vertices[3 * vindex1    ];
      dV2.y = model->vertices[3 * vindex3 + 1] - model->vertices[3 * vindex1 + 1];
      dV2.z = model->vertices[3 * vindex3 + 2] - model->vertices[3 * vindex1 + 2];

      rcp = 1.0f / (dUV1.u * dUV2.v - dUV1.v * dUV2.u);

      model->tangents[3 * vindex1    ] += (dV1.x * dUV2.v - dV2.x * dUV1.v) * rcp;
      model->tangents[3 * vindex1 + 1] += (dV1.y * dUV2.v - dV2.y * dUV1.v) * rcp;
      model->tangents[3 * vindex1 + 2] += (dV1.z * dUV2.v - dV2.z * dUV1.v) * rcp;

      model->tangents[3 * vindex2    ] = model->tangents[3 * vindex1    ];
      model->tangents[3 * vindex2 + 1] = model->tangents[3 * vindex1 + 1];
      model->tangents[3 * vindex2 + 2] = model->tangents[3 * vindex1 + 2];

      model->tangents[3 * vindex3    ] = model->tangents[3 * vindex1    ];
      model->tangents[3 * vindex3 + 1] = model->tangents[3 * vindex1 + 1];
      model->tangents[3 * vindex3 + 2] = model->tangents[3 * vindex1 + 2];


      model->binormals[3 * vindex1    ] -= (dV2.x * dUV1.u - dV1.x * dUV2.u) * rcp;
      model->binormals[3 * vindex1 + 1] -= (dV2.y * dUV1.u - dV1.y * dUV2.u) * rcp;
      model->binormals[3 * vindex1 + 2] -= (dV2.z * dUV1.u - dV1.z * dUV2.u) * rcp;

      model->binormals[3 * vindex2    ] = model->binormals[3 * vindex1    ];
      model->binormals[3 * vindex2 + 1] = model->binormals[3 * vindex1 + 1];
      model->binormals[3 * vindex2 + 2] = model->binormals[3 * vindex1 + 2];

      model->binormals[3 * vindex3    ] = model->binormals[3 * vindex1    ];
      model->binormals[3 * vindex3 + 1] = model->binormals[3 * vindex1 + 1];
      model->binormals[3 * vindex3 + 2] = model->binormals[3 * vindex1 + 2];

///      This code should give the same tangents as the code above
//      C_Vertex normal = {model->normals[3 * vindex1    ],
//                         model->normals[3 * vindex1 + 1],
//                         model->normals[3 * vindex1 + 2]};
//
//      C_Vertex binormal = {model->binormals[3 * vindex1    ],
//                           model->binormals[3 * vindex1 + 1],
//                           model->binormals[3 * vindex1 + 2]};
//
//      C_Vertex tangent = math::CrossProduct(&binormal, &normal);
//
//      model->tangents[3 * vindex1    ] += tangent.x;
//      model->tangents[3 * vindex1 + 1] += tangent.y;
//      model->tangents[3 * vindex1 + 2] += tangent.z;
//
//      model->tangents[3 * vindex2    ] += tangent.x;
//      model->tangents[3 * vindex2 + 1] += tangent.y;
//      model->tangents[3 * vindex2 + 2] += tangent.z;
//
//      model->tangents[3 * vindex3    ] += tangent.x;
//      model->tangents[3 * vindex3 + 1] += tangent.y;
//      model->tangents[3 * vindex3 + 2] += tangent.z;

      ++counters[vindex1];
      ++counters[vindex2];
      ++counters[vindex3];
   }

   for(i = 0; i < group->numtriangles; ++i) {
      vindex1 = model->triangles[group->triangles[i]].vindices[0];
      vindex2 = model->triangles[group->triangles[i]].vindices[1];
      vindex3 = model->triangles[group->triangles[i]].vindices[2];

      assert(counters[vindex1]);
      assert(counters[vindex2]);
      assert(counters[vindex3]);

      model->tangents[3 * vindex1    ] /= counters[vindex1];
      model->tangents[3 * vindex1 + 1] /= counters[vindex1];
      model->tangents[3 * vindex1 + 2] /= counters[vindex1];

      model->tangents[3 * vindex2    ] /= counters[vindex2];
      model->tangents[3 * vindex2 + 1] /= counters[vindex2];
      model->tangents[3 * vindex2 + 2] /= counters[vindex2];

      model->tangents[3 * vindex3    ] /= counters[vindex3];
      model->tangents[3 * vindex3 + 1] /= counters[vindex3];
      model->tangents[3 * vindex3 + 2] /= counters[vindex3];

      math::Normalize(&model->tangents[3 * vindex1    ],
                      &model->tangents[3 * vindex1 + 1],
                      &model->tangents[3 * vindex1 + 2]);

      math::Normalize(&model->tangents[3 * vindex2    ],
                      &model->tangents[3 * vindex2 + 1],
                      &model->tangents[3 * vindex2 + 2]);

      math::Normalize(&model->tangents[3 * vindex3    ],
                      &model->tangents[3 * vindex3 + 1],
                      &model->tangents[3 * vindex3 + 2]);

      model->binormals[3 * vindex1    ] /= counters[vindex1];
      model->binormals[3 * vindex1 + 1] /= counters[vindex1];
      model->binormals[3 * vindex1 + 2] /= counters[vindex1];

      model->binormals[3 * vindex2    ] /= counters[vindex2];
      model->binormals[3 * vindex2 + 1] /= counters[vindex2];
      model->binormals[3 * vindex2 + 2] /= counters[vindex2];

      model->binormals[3 * vindex3    ] /= counters[vindex3];
      model->binormals[3 * vindex3 + 1] /= counters[vindex3];
      model->binormals[3 * vindex3 + 2] /= counters[vindex3];

      math::Normalize(&model->binormals[3 * vindex1    ],
                      &model->binormals[3 * vindex1 + 1],
                      &model->binormals[3 * vindex1 + 2]);

      math::Normalize(&model->binormals[3 * vindex2    ],
                      &model->binormals[3 * vindex2 + 1],
                      &model->binormals[3 * vindex2 + 2]);

      math::Normalize(&model->binormals[3 * vindex3    ],
                      &model->binormals[3 * vindex3 + 1],
                      &model->binormals[3 * vindex3 + 2]);

//      printf("normal:\n");
//      printf("  %f %f %f\n", model->normals[3 * vindex1], model->normals[3 * vindex1 + 1], model->normals[3 * vindex1 + 2]);
//
//      printf("tangent:\n");
//      printf("  %f %f %f\n", model->tangents[3 * vindex1], model->tangents[3 * vindex1 + 1], model->tangents[3 * vindex1 + 2]);
//
//      printf("tangent:\n");
//      printf("  %f %f %f\n\n", model->binormals[3 * vindex1], model->binormals[3 * vindex1 + 1], model->binormals[3 * vindex1 + 2]);
   }

   delete[] counters;
}


static void
calculateNormals(GLMmodel *model, GLMgroup *group, bool flatShaded)
{
   int vindex1, vindex2, vindex3;
   int nindex1, nindex2, nindex3;
   C_Vertex v1, v2, v3, norm;
   unsigned int i;
   int *counters = NULL;

   assert(!model->normals);
   assert(!model->numnormals);
   assert(!(group->properties & HAS_NORMALS));

   model->numnormals = flatShaded ? 3 * model->numtriangles : model->numvertices;
   model->normals = (float *)malloc(3 * (model->numnormals + 1) * sizeof(float));
   memset(model->normals, 0, 3 * (model->numnormals + 1) * sizeof(float));

   group->properties |= HAS_NORMALS;

   if(!flatShaded) {
      counters = new int[model->numnormals + 1];
      assert(counters);
      memset(counters, 0, (model->numnormals + 1) * sizeof(int));
   }

   for(i = 0; i < group->numtriangles; i++) {
      vindex1 = model->triangles[group->triangles[i]].vindices[0];
      vindex2 = model->triangles[group->triangles[i]].vindices[1];
      vindex3 = model->triangles[group->triangles[i]].vindices[2];

      if(!flatShaded) {
         nindex1 = vindex1;
         nindex2 = vindex2;
         nindex3 = vindex3;
      } else {
         nindex1 = 3 * i;
         nindex2 = 3 * i + 1;
         nindex3 = 3 * i + 2;
      }

      model->triangles[group->triangles[i]].nindices[0] = nindex1;
      model->triangles[group->triangles[i]].nindices[1] = nindex2;
      model->triangles[group->triangles[i]].nindices[2] = nindex3;

//      printf("%d %d %d\n", vindex1, vindex2, vindex3);
//      printf("%d %d %d\n", nindex1, nindex2, nindex3);

      v1.x = model->vertices[3 * vindex1    ];
      v1.y = model->vertices[3 * vindex1 + 1];
      v1.z = model->vertices[3 * vindex1 + 2];

      v2.x = model->vertices[3 * vindex2    ];
      v2.y = model->vertices[3 * vindex2 + 1];
      v2.z = model->vertices[3 * vindex2 + 2];

      v3.x = model->vertices[3 * vindex3    ];
      v3.y = model->vertices[3 * vindex3 + 1];
      v3.z = model->vertices[3 * vindex3 + 2];

      norm = math::CrossProduct(&v1, &v2, &v3);

      model->normals[3 * nindex1    ] += norm.x;
      model->normals[3 * nindex1 + 1] += norm.y;
      model->normals[3 * nindex1 + 2] += norm.z;

      model->normals[3 * nindex2    ] += norm.x;
      model->normals[3 * nindex2 + 1] += norm.y;
      model->normals[3 * nindex2 + 2] += norm.z;

      model->normals[3 * nindex3    ] += norm.x;
      model->normals[3 * nindex3 + 1] += norm.y;
      model->normals[3 * nindex3 + 2] += norm.z;

//      printf("%f %f %f\n", v1.x, v1.y, v1.z);
//      printf("%f %f %f\n\n", model->normals[3 * nindex1    ], model->normals[3 * nindex1+1], model->normals[3 * nindex1+2]);
//
//      printf("%f %f %f\n", v2.x, v2.y, v2.z);
//      printf("%f %f %f\n\n", model->normals[3 * nindex2    ], model->normals[3 * nindex2+1], model->normals[3 * nindex2+2]);
//
//      printf("%f %f %f\n", v3.x, v3.y, v3.z);
//      printf("%f %f %f\n\n", model->normals[3 * nindex3    ], model->normals[3 * nindex3+1], model->normals[3 * nindex3+2]);

      if(!flatShaded) {
         ++counters[vindex1];
         ++counters[vindex2];
         ++counters[vindex3];
      }
   }

   for(i = 0; !flatShaded && i < group->numtriangles; ++i) {
      vindex1 = model->triangles[group->triangles[i]].vindices[0];
      vindex2 = model->triangles[group->triangles[i]].vindices[1];
      vindex3 = model->triangles[group->triangles[i]].vindices[2];

      assert(counters[vindex1]);
      assert(counters[vindex2]);
      assert(counters[vindex3]);

      model->normals[3 * vindex1    ] /= counters[vindex1];
      model->normals[3 * vindex1 + 1] /= counters[vindex1];
      model->normals[3 * vindex1 + 2] /= counters[vindex1];

      model->normals[3 * vindex2    ] /= counters[vindex2];
      model->normals[3 * vindex2 + 1] /= counters[vindex2];
      model->normals[3 * vindex2 + 2] /= counters[vindex2];

      model->normals[3 * vindex3    ] /= counters[vindex3];
      model->normals[3 * vindex3 + 1] /= counters[vindex3];
      model->normals[3 * vindex3 + 2] /= counters[vindex3];

      math::Normalize(&model->normals[3 * vindex1    ],
                      &model->normals[3 * vindex1 + 1],
                      &model->normals[3 * vindex1 + 2]);

      math::Normalize(&model->normals[3 * vindex2    ],
                      &model->normals[3 * vindex2 + 1],
                      &model->normals[3 * vindex2 + 2]);

      math::Normalize(&model->normals[3 * vindex3    ],
                      &model->normals[3 * vindex3 + 1],
                      &model->normals[3 * vindex3 + 2]);
   }

   if(!flatShaded) {
      delete[] counters;
   }
}


/* glmReadOBJ: Reads a model description from a Wavefront .OBJ file.
 * Returns a pointer to the created object which should be free'd with
 * glmDelete().
 *
 * filename - name of the file containing the Wavefront .OBJ format data.
 */
void glmReadOBJ(const char* filename, C_MeshGroup *meshgroup)
{
	GLMmodel* model;
	FILE*     file;
	int index, nindex;
   size_t size = 0;

   printf("Reading \"%s\" file... \n", filename);

	file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "glmReadOBJ() failed: can't open data file \"%s\".\n", filename);
		exit(1);
	}

	/* allocate a new model */
	model = (GLMmodel*)malloc(sizeof(GLMmodel));
	model->pathname      = strdup(filename);
	model->mtllibname    = NULL;
	model->numvertices   = 0;
	model->vertices      = NULL;
	model->numnormals    = 0;
	model->normals       = NULL;
	model->tangents      = NULL;
	model->binormals     = NULL;
	model->numtexcoords  = 0;
	model->texcoords     = NULL;
	model->numfacetnorms = 0;
	model->facetnorms    = NULL;
	model->numtriangles  = 0;
	model->triangles     = NULL;
	model->nummaterials  = 0;
	model->materials     = NULL;
	model->numgroups     = 0;
	model->groups        = NULL;
	model->position[0]   = 0.0;
	model->position[1]   = 0.0;
	model->position[2]   = 0.0;

	/* make a first pass through the file to get a count of the number
	   of vertices, normals, texcoords & triangles */
	glmFirstPass(model, file);

	/* allocate memory */
	model->vertices = (float*)malloc(sizeof(float) * 3 * (model->numvertices + 1));
	model->triangles = (GLMtriangle*)malloc(sizeof(GLMtriangle) * model->numtriangles);

	if (model->numnormals)
		model->normals = (float*)malloc(sizeof(float) * 3 * (model->numnormals + 1));

	if (model->numtexcoords)
		model->texcoords = (float*)malloc(sizeof(float) * 2 * (model->numtexcoords + 1));

	/* rewind to beginning of file and read in the data this pass */
	rewind(file);

	glmSecondPass(model, file);

	/* close the file */
	fclose(file);

/// ================================================================

	/// Init mesh struct
	meshgroup->nMeshes = model->numgroups;

   /// Copy vertices
   meshgroup->nVertices = model->numvertices;

   /// Copy mesh information
   GLMgroup *group = model->groups;
   C_Mesh *mesh;
   int totalVertices = 0, totalTriangles = 0;
   while(group) {
      assert(group->properties == model->properties);

      /// Allocate memory
      mesh = meshgroup->addMesh();
      mesh->nTriangles = group->numtriangles;
      mesh->nVertices = 3 * group->numtriangles;
      mesh->vertices = new C_Vertex[mesh->nVertices];
      mesh->normals = new C_Vertex[mesh->nVertices];
      mesh->tangents = new C_Vertex[mesh->nVertices];
      mesh->binormals = new C_Vertex[mesh->nVertices];

      if(group->properties & HAS_TEXCOORDS) {
         mesh->textCoords = new C_TexCoord[mesh->nVertices];
      }

      if(!(group->properties & HAS_NORMALS)) {
         calculateNormals(model, group, false);
      }

      calculateTBN(model, group);

      totalVertices += mesh->nVertices;
      totalTriangles += mesh->nTriangles;

      for(unsigned int i = 0; i < group->numtriangles; i++) {
         /// Copy vertices
         index = 3 * model->triangles[group->triangles[i]].vindices[0] /* - 1*/; /// -1 is not needed allthough obj file format considers starts indexing from 1 instead of 0.
         mesh->vertices[3 * i    ].x = model->vertices[index    ];
         mesh->vertices[3 * i    ].y = model->vertices[index + 1];
         mesh->vertices[3 * i    ].z = model->vertices[index + 2];

         index = 3 * model->triangles[group->triangles[i]].vindices[1];
         mesh->vertices[3 * i + 1].x = model->vertices[index    ];
         mesh->vertices[3 * i + 1].y = model->vertices[index + 1];
         mesh->vertices[3 * i + 1].z = model->vertices[index + 2];

         index = 3 * model->triangles[group->triangles[i]].vindices[2];
         mesh->vertices[3 * i + 2].x = model->vertices[index    ];
         mesh->vertices[3 * i + 2].y = model->vertices[index + 1];
         mesh->vertices[3 * i + 2].z = model->vertices[index + 2];

         /// Copy texture coordinates
         if(group->properties & HAS_TEXCOORDS) {
            index = 2 * model->triangles[group->triangles[i]].tindices[0];
//            printf("t:%d (%f %f) -- ", i, model->texcoords[index    ], model->texcoords[index + 1]);
            mesh->textCoords[3 * i    ].u = fabs(model->texcoords[index    ]);
            mesh->textCoords[3 * i    ].v = fabs(model->texcoords[index + 1]);

            index = 2 * model->triangles[group->triangles[i]].tindices[1];
//            printf("(%f %f) -- ", model->texcoords[index    ], model->texcoords[index + 1]);
            mesh->textCoords[3 * i + 1].u = fabs(model->texcoords[index    ]);
            mesh->textCoords[3 * i + 1].v = fabs(model->texcoords[index + 1]);

            index = 2 * model->triangles[group->triangles[i]].tindices[2];
//            printf("(%f %f)\n", model->texcoords[index    ], model->texcoords[index + 1]);
            mesh->textCoords[3 * i + 2].u = fabs(model->texcoords[index    ]);
            mesh->textCoords[3 * i + 2].v = fabs(model->texcoords[index + 1]);
         }

         /// Copy TBN
         if(group->properties & HAS_NORMALS) {
            index = 3 * model->triangles[group->triangles[i]].vindices[0] /* - 1*/; /// -1 is not needed allthough obj file format considers starts indexing from 1 instead of 0.
            nindex = 3 * model->triangles[group->triangles[i]].nindices[0] /* - 1*/; /// -1 is not needed allthough obj file format considers starts indexing from 1 instead of 0.
            mesh->normals[3 * i    ].x = model->normals[nindex    ];
            mesh->normals[3 * i    ].y = model->normals[nindex + 1];
            mesh->normals[3 * i    ].z = model->normals[nindex + 2];

            mesh->tangents[3 * i    ].x = model->tangents[index    ];
            mesh->tangents[3 * i    ].y = model->tangents[index + 1];
            mesh->tangents[3 * i    ].z = model->tangents[index + 2];

            mesh->binormals[3 * i    ].x = model->binormals[index    ];
            mesh->binormals[3 * i    ].y = model->binormals[index + 1];
            mesh->binormals[3 * i    ].z = model->binormals[index + 2];

            index = 3 * model->triangles[group->triangles[i]].vindices[1];
            nindex = 3 * model->triangles[group->triangles[i]].nindices[1];
            mesh->normals[3 * i + 1].x = model->normals[nindex    ];
            mesh->normals[3 * i + 1].y = model->normals[nindex + 1];
            mesh->normals[3 * i + 1].z = model->normals[nindex + 2];

            mesh->tangents[3 * i + 1].x = model->tangents[index    ];
            mesh->tangents[3 * i + 1].y = model->tangents[index + 1];
            mesh->tangents[3 * i + 1].z = model->tangents[index + 2];

            mesh->binormals[3 * i + 1].x = model->binormals[index    ];
            mesh->binormals[3 * i + 1].y = model->binormals[index + 1];
            mesh->binormals[3 * i + 1].z = model->binormals[index + 2];

            index = 3 * model->triangles[group->triangles[i]].vindices[2];
            nindex = 3 * model->triangles[group->triangles[i]].nindices[2];
            mesh->normals[3 * i + 2].x = model->normals[nindex    ];
            mesh->normals[3 * i + 2].y = model->normals[nindex + 1];
            mesh->normals[3 * i + 2].z = model->normals[nindex + 2];

            mesh->tangents[3 * i + 2].x = model->tangents[index    ];
            mesh->tangents[3 * i + 2].y = model->tangents[index + 1];
            mesh->tangents[3 * i + 2].z = model->tangents[index + 2];

            mesh->binormals[3 * i + 2].x = model->binormals[index    ];
            mesh->binormals[3 * i + 2].y = model->binormals[index + 1];
            mesh->binormals[3 * i + 2].z = model->binormals[index + 2];
         }
      }

      /// Copy material
//      printf("material: %d", group->material);
      if(model->materials[group->material].texture_diffuse && strlen(model->materials[group->material].texture_diffuse))
         mesh->texture_diffuse = textureManager->loadTexture(model->materials[group->material].texture_diffuse, TEXTURE_TRILINEAR);
      if(model->materials[group->material].texture_specular && strlen(model->materials[group->material].texture_specular))
         mesh->texture_specular = textureManager->loadTexture(model->materials[group->material].texture_specular, TEXTURE_TRILINEAR);
      if(model->materials[group->material].texture_normal && strlen(model->materials[group->material].texture_normal))
         mesh->texture_normal = textureManager->loadTexture(model->materials[group->material].texture_normal, TEXTURE_TRILINEAR);

      size += mesh->nVertices * sizeof(C_Vertex);
      if(group->properties & HAS_TEXCOORDS)
         size += mesh->nVertices * sizeof(C_TexCoord);
      if(group->properties & HAS_NORMALS)
         size += mesh->nVertices * sizeof(C_Vertex);

      group = group->next;
   }

   meshgroup->nTriangles = totalTriangles;
//   meshgroup->nVertices = totalVertices;

   printf("\t%d vertices - %d triangles\n", totalVertices, totalTriangles);
   printf("\tTotal size of model: %lu bytes\n", size);

   printf("Done!\n");

   glmDelete(model);
}

/* glmFirstPass: first pass at a Wavefront OBJ file that gets all the
 * statistics of the model (such as #vertices, #normals, etc)
 *
 * model - properly initialized GLMmodel structure
 * file  - (fopen'd) file descriptor
 */
void glmFirstPass(GLMmodel* model, FILE* file)
{
	unsigned int   numvertices;	/* number of vertices in model */
	unsigned int   numnormals;		/* number of normals in model */
	unsigned int   numtexcoords;	/* number of texcoords in model */
	unsigned int   numtriangles;	/* number of triangles in model */
	GLMgroup       *group;			/* current group */
	int            v, n, t;
	char           buf[128];

	/* make a default group */
	group = glmAddGroup(model, "default");

	numvertices = numnormals = numtexcoords = numtriangles = 0;
	while(fscanf(file, "%s", buf) != EOF) {
		switch(buf[0]) {
		case '#':				/* comment */
			/* eat up rest of line */
			fgets(buf, sizeof(buf), file);
			break;
		case 'v':				/* v, vn, vt */
			switch(buf[1]) {
			case '\0':			/* vertex */
				/* eat up rest of line */
				fgets(buf, sizeof(buf), file);
				numvertices++;
				break;
			case 'n':				/* normal */
				/* eat up rest of line */
				fgets(buf, sizeof(buf), file);
				numnormals++;
				break;
			case 't':				/* texcoord */
				/* eat up rest of line */
				fgets(buf, sizeof(buf), file);
				numtexcoords++;
				break;
			default:
				printf("glmFirstPass(): Unknown token \"%s\".\n", buf);
				exit(1);
				break;
			}
			break;
		case 'm':
         fgets(buf, sizeof(buf), file);
			sscanf(buf, "%s %s", buf, buf);
			model->mtllibname = strdup(buf);
         glmReadMTL(model, buf);
			break;
		case 'u':
			/* eat up rest of line */
			fgets(buf, sizeof(buf), file);
			break;
		case 'g':				/* group */
			/* eat up rest of line */
			fgets(buf, sizeof(buf), file);
#if SINGLE_STRING_GROUP_NAMES
			sscanf(buf, "%s", buf);
#else
			buf[strlen(buf)-1] = '\0';	/* nuke '\n' */
#endif
			group = glmAddGroup(model, buf);
			break;
		case 'f':				/* face */
			v = n = t = 0;
			fscanf(file, "%s", buf);
			/* can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d */
			if (strstr(buf, "//")) {
				/* v//n */
				sscanf(buf, "%d//%d", &v, &n);
				fscanf(file, "%d//%d", &v, &n);
				fscanf(file, "%d//%d", &v, &n);
				numtriangles++;
				group->numtriangles++;
				group->properties = HAS_VERTICES | HAS_NORMALS;
				model->properties = HAS_VERTICES | HAS_NORMALS;
				while(fscanf(file, "%d//%d", &v, &n) > 0) {
					numtriangles++;
					group->numtriangles++;
				}
			} else if (sscanf(buf, "%d/%d/%d", &v, &t, &n) == 3) {
				/* v/t/n */
				fscanf(file, "%d/%d/%d", &v, &t, &n);
				fscanf(file, "%d/%d/%d", &v, &t, &n);
				numtriangles++;
				group->numtriangles++;
				group->properties = HAS_VERTICES | HAS_NORMALS | HAS_TEXCOORDS;
				model->properties = HAS_VERTICES | HAS_NORMALS | HAS_TEXCOORDS;
				while(fscanf(file, "%d/%d/%d", &v, &t, &n) > 0) {
					numtriangles++;
					group->numtriangles++;
				}
			} else if (sscanf(buf, "%d/%d", &v, &t) == 2) {
				/* v/t */
				fscanf(file, "%d/%d", &v, &t);
				fscanf(file, "%d/%d", &v, &t);
				numtriangles++;
				group->numtriangles++;
				group->properties = HAS_VERTICES | HAS_TEXCOORDS;
				model->properties = HAS_VERTICES | HAS_TEXCOORDS;
				while(fscanf(file, "%d/%d", &v, &t) > 0) {
					numtriangles++;
					group->numtriangles++;
				}
			} else {
				/* v */
				fscanf(file, "%d", &v);
				fscanf(file, "%d", &v);
				numtriangles++;
				group->numtriangles++;
				group->properties = HAS_VERTICES;
				model->properties = HAS_VERTICES;
				while(fscanf(file, "%d", &v) > 0) {
					numtriangles++;
					group->numtriangles++;
				}
			}
			break;

		default:
			/* eat up rest of line */
			fgets(buf, sizeof(buf), file);
			break;
		}
	}

	/* set the stats in the model structure */
	model->numvertices  = numvertices;
	model->numnormals   = numnormals;
	model->numtexcoords = numtexcoords;
	model->numtriangles = numtriangles;

	/* allocate memory for the triangles in each group */
	group = model->groups;
	while(group) {
		group->triangles = (unsigned int*)malloc(sizeof(unsigned int) * group->numtriangles);
		group->numtriangles = 0;
		group = group->next;
	}
}

/* glmSecondPass: second pass at a Wavefront OBJ file that gets all
 * the data.
 *
 * model - properly initialized GLMmodel structure
 * file  - (fopen'd) file descriptor
 */
void glmSecondPass(GLMmodel* model, FILE* file)
{
	unsigned int   numvertices;	/* number of vertices in model */
	unsigned int   numnormals;		/* number of normals in model */
	unsigned int   numtexcoords;	/* number of texcoords in model */
	unsigned int   numtriangles;	/* number of triangles in model */
	float          *vertices;		/* array of vertices  */
	float          *normals;		/* array of normals */
	float          *texcoords;		/* array of texture coordinates */
	GLMgroup       *group;			/* current group pointer */
	unsigned int   material;		/* current material */
	int            v, n, t;
	char           buf[128];

	/* set the pointer shortcuts */
	vertices  = model->vertices;
	normals   = model->normals;
	texcoords = model->texcoords;
	group     = model->groups;

	/* on the second pass through the file, read all the data into the
	   allocated arrays */
	numvertices = numnormals = numtexcoords = 1;
	numtriangles = 0;
	material = 0;
	while(fscanf(file, "%s", buf) != EOF) {
		switch(buf[0]) {
		case '#':				/* comment */
			/* eat up rest of line */
			fgets(buf, sizeof(buf), file);
			break;
		case 'v':				/* v, vn, vt */
			switch(buf[1]) {
			case '\0':			/* vertex */
				fscanf(file, "%f %f %f",
				       &vertices[3 * numvertices + 0],
				       &vertices[3 * numvertices + 1],
				       &vertices[3 * numvertices + 2]);
				numvertices++;
				break;
			case 'n':				/* normal */
				fscanf(file, "%f %f %f",
				       &normals[3 * numnormals + 0],
				       &normals[3 * numnormals + 1],
				       &normals[3 * numnormals + 2]);
				numnormals++;
				break;
			case 't':				/* texcoord */
				fscanf(file, "%f %f",
				       &texcoords[2 * numtexcoords + 0],
				       &texcoords[2 * numtexcoords + 1]);
				numtexcoords++;
				break;
			}
			break;
		case 'u':
			fgets(buf, sizeof(buf), file);
			sscanf(buf, "%s %s", buf, buf);
         group->material = material = glmFindMaterial(model, buf);
			break;
		case 'g':				/* group */
			/* eat up rest of line */
			fgets(buf, sizeof(buf), file);
#if SINGLE_STRING_GROUP_NAMES
			sscanf(buf, "%s", buf);
#else
			buf[strlen(buf)-1] = '\0';	/* nuke '\n' */
#endif
			group = glmFindGroup(model, buf);
			group->material = material;
			break;
		case 'f':				/* face */
			v = n = t = 0;
			fscanf(file, "%s", buf);
			/* can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d */
			if (strstr(buf, "//")) {
				/* v//n */
				sscanf(buf, "%d//%d", &v, &n);
				T(numtriangles).vindices[0] = v;
				T(numtriangles).nindices[0] = n;
				fscanf(file, "%d//%d", &v, &n);
				T(numtriangles).vindices[1] = v;
				T(numtriangles).nindices[1] = n;
				fscanf(file, "%d//%d", &v, &n);
				T(numtriangles).vindices[2] = v;
				T(numtriangles).nindices[2] = n;
				group->triangles[group->numtriangles++] = numtriangles;
				numtriangles++;
				while(fscanf(file, "%d//%d", &v, &n) > 0) {
					T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
					T(numtriangles).nindices[0] = T(numtriangles-1).nindices[0];
					T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
					T(numtriangles).nindices[1] = T(numtriangles-1).nindices[2];
					T(numtriangles).vindices[2] = v;
					T(numtriangles).nindices[2] = n;
					group->triangles[group->numtriangles++] = numtriangles;
					numtriangles++;
				}
			} else if (sscanf(buf, "%d/%d/%d", &v, &t, &n) == 3) {
				/* v/t/n */
				T(numtriangles).vindices[0] = v;
				T(numtriangles).tindices[0] = t;
				T(numtriangles).nindices[0] = n;
				fscanf(file, "%d/%d/%d", &v, &t, &n);
				T(numtriangles).vindices[1] = v;
				T(numtriangles).tindices[1] = t;
				T(numtriangles).nindices[1] = n;
				fscanf(file, "%d/%d/%d", &v, &t, &n);
				T(numtriangles).vindices[2] = v;
				T(numtriangles).tindices[2] = t;
				T(numtriangles).nindices[2] = n;
				group->triangles[group->numtriangles++] = numtriangles;
				numtriangles++;
				while(fscanf(file, "%d/%d/%d", &v, &t, &n) > 0) {
					T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
					T(numtriangles).tindices[0] = T(numtriangles-1).tindices[0];
					T(numtriangles).nindices[0] = T(numtriangles-1).nindices[0];
					T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
					T(numtriangles).tindices[1] = T(numtriangles-1).tindices[2];
					T(numtriangles).nindices[1] = T(numtriangles-1).nindices[2];
					T(numtriangles).vindices[2] = v;
					T(numtriangles).tindices[2] = t;
					T(numtriangles).nindices[2] = n;
					group->triangles[group->numtriangles++] = numtriangles;
					numtriangles++;
				}
			} else if (sscanf(buf, "%d/%d", &v, &t) == 2) {
				/* v/t */
				T(numtriangles).vindices[0] = v;
				T(numtriangles).tindices[0] = t;
				fscanf(file, "%d/%d", &v, &t);
				T(numtriangles).vindices[1] = v;
				T(numtriangles).tindices[1] = t;
				fscanf(file, "%d/%d", &v, &t);
				T(numtriangles).vindices[2] = v;
				T(numtriangles).tindices[2] = t;
				group->triangles[group->numtriangles++] = numtriangles;
				numtriangles++;
				while(fscanf(file, "%d/%d", &v, &t) > 0) {
					T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
					T(numtriangles).tindices[0] = T(numtriangles-1).tindices[0];
					T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
					T(numtriangles).tindices[1] = T(numtriangles-1).tindices[2];
					T(numtriangles).vindices[2] = v;
					T(numtriangles).tindices[2] = t;
					group->triangles[group->numtriangles++] = numtriangles;
					numtriangles++;
				}
			} else {
				/* v */
				sscanf(buf, "%d", &v);
				T(numtriangles).vindices[0] = v;
				fscanf(file, "%d", &v);
				T(numtriangles).vindices[1] = v;
				fscanf(file, "%d", &v);
				T(numtriangles).vindices[2] = v;
				group->triangles[group->numtriangles++] = numtriangles;
				numtriangles++;
				while(fscanf(file, "%d", &v) > 0) {
					T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
					T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
					T(numtriangles).vindices[2] = v;
					group->triangles[group->numtriangles++] = numtriangles;
					numtriangles++;
				}
			}
			break;

		default:
			/* eat up rest of line */
			fgets(buf, sizeof(buf), file);
			break;
		}
	}

#if 0
	/* announce the memory requirements */
	printf(" Memory: %d bytes\n",
	       numvertices  * 3*sizeof(float) +
	       numnormals   * 3*sizeof(float) * (numnormals ? 1 : 0) +
	       numtexcoords * 3*sizeof(float) * (numtexcoords ? 1 : 0) +
	       numtriangles * sizeof(GLMtriangle));
#endif
}


/* glmReadMTL: read a wavefront material library file
 *
 * model - properly initialized GLMmodel structure
 * name  - name of the material library
 */
static void glmReadMTL(GLMmodel* model, char* name)
{
	FILE* file;
	char* dir;
	char* filename;
	char  buf[128];
	unsigned int nummaterials, i;

	dir = glmDirName(model->pathname);
	filename = (char*)malloc(sizeof(char) * (strlen(dir) + strlen(name) + 1));
	strcpy(filename, dir);
	strcat(filename, name);
	free(dir);

	file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "glmReadMTL() failed: can't open material file \"%s\".\n", filename);
		exit(1);
	}
	free(filename);

	/* count the number of materials in the file */
	nummaterials = 1;
	while(fscanf(file, "%s", buf) != EOF) {
		switch(buf[0]) {
		case '#':				/* comment */
			/* eat up rest of line */
			fgets(buf, sizeof(buf), file);
			break;
		case 'n':				/* newmtl */
			fgets(buf, sizeof(buf), file);
			nummaterials++;
			sscanf(buf, "%s %s", buf, buf);
			break;
		default:
			/* eat up rest of line */
			fgets(buf, sizeof(buf), file);
			break;
		}
	}

	rewind(file);

	model->materials = (GLMmaterial*)malloc(sizeof(GLMmaterial) * nummaterials);
	memset(model->materials, 0, sizeof(GLMmaterial) * nummaterials);
	model->nummaterials = nummaterials;

	/* set the default material */
	for (i = 0; i < nummaterials; i++) {
		model->materials[i].name = NULL;
		model->materials[i].shininess = 65.0;
		model->materials[i].diffuse[0] = 0.8;
		model->materials[i].diffuse[1] = 0.8;
		model->materials[i].diffuse[2] = 0.8;
		model->materials[i].diffuse[3] = 1.0;
		model->materials[i].ambient[0] = 0.2;
		model->materials[i].ambient[1] = 0.2;
		model->materials[i].ambient[2] = 0.2;
		model->materials[i].ambient[3] = 1.0;
		model->materials[i].specular[0] = 0.0;
		model->materials[i].specular[1] = 0.0;
		model->materials[i].specular[2] = 0.0;
		model->materials[i].specular[3] = 1.0;
	}
	model->materials[0].name = strdup("default");

	/* now, read in the data */
	nummaterials = 0;
	while(fscanf(file, "%s", buf) != EOF) {
		switch(buf[0]) {
		case '#':				/* comment */
			/* eat up rest of line */
			fgets(buf, sizeof(buf), file);
			break;
		case 'n':				/* newmtl */
			fgets(buf, sizeof(buf), file);
			sscanf(buf, "%s %s", buf, buf);
			nummaterials++;
			model->materials[nummaterials].name = strdup(buf);
			break;
		case 'N':
			fscanf(file, "%f", &model->materials[nummaterials].shininess);
			/* wavefront shininess is from [0, 1000], so scale for OpenGL */
			model->materials[nummaterials].shininess /= 1000.0;
			model->materials[nummaterials].shininess *= 128.0;
			break;
		case 'K':
			switch(buf[1]) {
			case 'd':
				fscanf(file, "%f %f %f",
				       &model->materials[nummaterials].diffuse[0],
				       &model->materials[nummaterials].diffuse[1],
				       &model->materials[nummaterials].diffuse[2]);
				break;
			case 's':
				fscanf(file, "%f %f %f",
				       &model->materials[nummaterials].specular[0],
				       &model->materials[nummaterials].specular[1],
				       &model->materials[nummaterials].specular[2]);
				break;
			case 'a':
				fscanf(file, "%f %f %f",
				       &model->materials[nummaterials].ambient[0],
				       &model->materials[nummaterials].ambient[1],
				       &model->materials[nummaterials].ambient[2]);
				break;
			default:
				/* eat up rest of line */
				fgets(buf, sizeof(buf), file);
				break;
			}
			break;
		case 'm':
		   switch(buf[5]) {
         case 'd':
         case 'a':
   			fscanf(file, "%s", buf);
	   		model->materials[nummaterials].texture_diffuse = strdup(buf);
			   break;
         case 's':
   			fscanf(file, "%s", buf);
	   		model->materials[nummaterials].texture_specular = strdup(buf);
			   break;
         case 'n':
   			fscanf(file, "%s", buf);
	   		model->materials[nummaterials].texture_normal = strdup(buf);
			   break;
			default:
            /* eat up rest of line */
            fgets(buf, sizeof(buf), file);
            break;
         }
         break;
		default:
			/* eat up rest of line */
			fgets(buf, sizeof(buf), file);
			break;
		}
	}

	fclose(file);
}


/// glmFindGroup: Find a group in the model
GLMgroup* glmFindGroup(GLMmodel* model, const char* name)
{
	GLMgroup* group;

	assert(model);

	group = model->groups;
	while(group) {
		if (!strcmp(name, group->name)) {
			break;
		}
		group = group->next;
	}

	return group;
}

/// glmAddGroup: Add a group to the model
GLMgroup* glmAddGroup(GLMmodel* model, const char* name)
{
	GLMgroup* group;

	group = glmFindGroup(model, name);
	model->properties = 0;
	if (!group) {
		group = (GLMgroup*)malloc(sizeof(GLMgroup));
		group->name = strdup(name);
		group->material = 0;
		group->numtriangles = 0;
		group->triangles = NULL;
		group->next = model->groups;
		group->properties = 0;
		model->groups = group;
		model->numgroups++;
	}

	return group;
}

/// glmDelete: Deletes a GLMmodel structure.
/// model - initialized GLMmodel structure
void glmDelete(GLMmodel* model)
{
  GLMgroup* group;
  unsigned int i;

  assert(model);

  if (model->pathname)   free(model->pathname);
  if (model->mtllibname) free(model->mtllibname);
  if (model->vertices)   free(model->vertices);
  if (model->normals)    free(model->normals);
  if (model->binormals)  free(model->binormals);
  if (model->tangents)   free(model->tangents);
  if (model->texcoords)  free(model->texcoords);
  if (model->facetnorms) free(model->facetnorms);
  if (model->triangles)  free(model->triangles);
  if (model->materials) {
    for (i = 0; i < model->nummaterials; i++) {
      free(model->materials[i].name);
//      if(model->materials[i].texture)
//         free(model->materials[i].texture);
    }
  }
  free(model->materials);
  while(model->groups) {
    group = model->groups;
    model->groups = model->groups->next;
    free(group->name);
    free(group->triangles);
    free(group);
  }

  free(model);
}

/* glmDirName: return the directory given a path
 *
 * path - filesystem path
 *
 * NOTE: the return value should be free'd.
 */
char* glmDirName(char* path)
{
  char* dir;
  char* s;

  dir = strdup(path);

  s = strrchr(dir, '/');
  if (s)
    s[1] = '\0';
  else
    dir[0] = '\0';

  return dir;
}

/* glmFindGroup: Find a material in the model
 */
unsigned int glmFindMaterial(GLMmodel* model, char* name)
{
  unsigned int i;

  /* XXX doing a linear search on a string key'd list is pretty lame,
     but it works and is fast enough for now. */
  for (i = 0; i < model->nummaterials; i++) {
    if (!strcmp(model->materials[i].name, name))
      goto found;
  }

  /* didn't find the name, so print a warning and return the default
     material (0). */
  printf("glmFindMaterial():  can't find material \"%s\".\n", name);
  i = 0;

found:
  return i;
}
