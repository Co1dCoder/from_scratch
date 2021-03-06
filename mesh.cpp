#include <stdlib.h>
#include <string.h>

#include "mesh.h"
#include "objreader/objfile.h"

C_BaseMesh::C_BaseMesh(void)
{
   PRINT_FUNC_ENTRY;

   nVertices = 0;
   nTriangles = 0;
}

C_Mesh::C_Mesh(void)
{
   PRINT_FUNC_ENTRY;

   vertices = NULL;
   colors = NULL;
   textCoords = NULL;
   normals = NULL;
   tangents = NULL;
   binormals = NULL;
   indices = NULL;
   next = NULL;
   group = NULL;
   refCounter = 1;
   texture_diffuse = NULL;
   texture_specular = NULL;
   texture_normal = NULL;

   memset(vbos, 0, MAX_VBOS_PER_OBJECT * sizeof(unsigned));
}

C_Mesh *
C_Mesh::refMesh(void)
{
   PRINT_FUNC_ENTRY;

   ++refCounter;
   return this;
}

int
C_Mesh::unRefMesh(void)
{
   PRINT_FUNC_ENTRY;

   assert(refCounter > 0);
   return --refCounter;
}

void
C_MeshGroup::softCopy(const C_MeshGroup *group)
{
   PRINT_FUNC_ENTRY;

   if(this != group) {
      /// Increase the reference counter of all meshes
      meshes = group->meshes->refMesh();
      C_Mesh *meshList = meshes->next;
      while(meshList) {
         (void)meshList->refMesh();
         meshList = meshList->next;
      }

      nMeshes = group->nMeshes;
      nTriangles = group->nTriangles;
      nVertices = group->nVertices;
      shader = group->shader;
      bbox = group->bbox;
      position = group->position;
      matrix = group->matrix;
      applyFrustumCulling = group->applyFrustumCulling;
   }
}

C_Mesh &C_Mesh::operator= (const C_Mesh &mesh)
{
   PRINT_FUNC_ENTRY;

   if(this != &mesh) {
      if(mesh.vertices) {
         vertices = new C_Vertex[mesh.nVertices];
         memcpy(vertices, mesh.vertices, mesh.nVertices * sizeof(C_Vertex));
      }

      if(mesh.normals) {
         normals = new C_Vertex[mesh.nVertices];
         memcpy(normals, mesh.normals, mesh.nVertices* sizeof(C_Vertex));
      }

      if(mesh.tangents) {
         tangents = new C_Vertex[mesh.nVertices];
         memcpy(tangents, mesh.tangents, mesh.nVertices* sizeof(C_Vertex));
      }

      if(mesh.binormals) {
         binormals = new C_Vertex[mesh.nVertices];
         memcpy(binormals, mesh.binormals, mesh.nVertices* sizeof(C_Vertex));
      }

      if(mesh.textCoords) {
         textCoords = new C_TexCoord[mesh.nVertices];
         memcpy(textCoords, mesh.textCoords, mesh.nVertices * sizeof(C_TexCoord));
      }

      if(mesh.colors) {
         colors = new C_Color[mesh.nVertices];
         memcpy(colors, mesh.colors, mesh.nVertices * sizeof(C_Color));
      }

      assert(!indices);

      bbox = mesh.bbox;
      nVertices = mesh.nVertices;
      nTriangles = mesh.nTriangles;
      refCounter = mesh.refCounter;

      if(mesh.texture_diffuse) {
         texture_diffuse = mesh.texture_diffuse->refTexture();
      }

      if(mesh.texture_normal) {
         texture_normal = mesh.texture_normal->refTexture();
      }

      if(mesh.texture_specular) {
         texture_specular = mesh.texture_specular->refTexture();
      }
   }

   return *this;
}

C_Mesh::~C_Mesh(void)
{
   PRINT_FUNC_ENTRY;

   assert(!refCounter);

   if(colors)     delete[] colors;
   if(vertices)   delete[] vertices;
   if(textCoords) delete[] textCoords;
   if(normals)    delete[] normals;
   if(tangents)   delete[] tangents;
   if(binormals)  delete[] binormals;
   if(indices)    delete[] indices;
   if(texture_diffuse && !texture_diffuse->unrefTexture())     delete texture_diffuse;
   if(texture_normal && !texture_normal->unrefTexture())       delete texture_normal;
   if(texture_specular && !texture_specular->unrefTexture())   delete texture_specular;
}

C_MeshGroup::C_MeshGroup(void)
{
   PRINT_FUNC_ENTRY;

   meshes = NULL;
   nMeshes = 0;
   matrix = Identity;
   position.x = position.y = position.z = 0.0f;

   rotationQuat.Identity();
   rotated = false;
}

C_MeshGroup::~C_MeshGroup(void)
{
   PRINT_FUNC_ENTRY;

   C_Mesh *oldMesh;
   C_Mesh *mesh = meshes;

   while(mesh) {
      oldMesh = mesh;
      mesh = mesh->next;
      if(!oldMesh->unRefMesh()) {
         delete oldMesh;
      }
   }

   nMeshes = 0;
   meshes = NULL;
}

C_MeshGroup &C_MeshGroup::operator= (const C_MeshGroup &group)
{
   PRINT_FUNC_ENTRY;

   if(this != &group) {
      C_Mesh *mesh = group.meshes;
      C_Mesh *newMesh;

      while(mesh) {
         newMesh = addMesh();
         *newMesh = *mesh;
         mesh = mesh->next;
      }

      nMeshes = group.nMeshes;
      nTriangles = group.nTriangles;
      nVertices = group.nVertices;
      shader = group.shader;
      bbox = group.bbox;
      position = group.position;
      matrix = group.matrix;
      applyFrustumCulling = group.applyFrustumCulling;
   }

   return *this;
}

C_Mesh *
C_MeshGroup::addMesh(void)
{
   PRINT_FUNC_ENTRY;

   C_Mesh *newMesh = new C_Mesh();
   newMesh->next = meshes;
   meshes = newMesh;
   newMesh->group = this;
   nMeshes++;

   return newMesh;
}

void
C_Mesh::applyTransformationOnVertices(const ESMatrix *mat)
{
   assert(normals);
   assert(vertices);

   for (int i = 0; i < nVertices; i++) {
      vertices[i] = math::transformPoint(mat, &vertices[i]);

      if(normals) {
         normals[i] = math::transformNormal(mat, &normals[i]);
         math::Normalize(&normals[i]);
      }

      if(binormals) {
         binormals[i] = math::transformNormal(mat, &binormals[i]);
         math::Normalize(&binormals[i]);
      }

      if(tangents) {
         tangents[i] = math::transformNormal(mat, &tangents[i]);
         math::Normalize(&tangents[i]);
      }
   }
}

void
C_MeshGroup::applyTransformationOnVertices(const ESMatrix *mat)
{
   C_Mesh *mesh = meshes;
   while(mesh) {
      mesh->applyTransformationOnVertices(mat);
      mesh = mesh->next;
   }

   /// Update bboxes
   calculateBbox();
}

void
C_Mesh::calculateBbox(void)
{
   float minX = GREATEST_FLOAT, minY = GREATEST_FLOAT, minZ = GREATEST_FLOAT;
   float maxX = SMALLEST_FLOAT, maxY = SMALLEST_FLOAT, maxZ = SMALLEST_FLOAT;

   for(int i = 0; i < nVertices; ++i) {
      if(vertices[i].x > maxX) maxX = vertices[i].x;
      if(vertices[i].y > maxY) maxY = vertices[i].y;
      if(vertices[i].z > maxZ) maxZ = vertices[i].z;

      if(vertices[i].x < minX) minX = vertices[i].x;
      if(vertices[i].y < minY) minY = vertices[i].y;
      if(vertices[i].z < minZ) minZ = vertices[i].z;
   }

   bbox.SetMax(maxX , maxY , maxZ);
   bbox.SetMin(minX , minY , minZ);
   bbox.SetVertices();
}

void
C_MeshGroup::calculateBbox(void)
{
   float minX = GREATEST_FLOAT, minY = GREATEST_FLOAT, minZ = GREATEST_FLOAT;
   float maxX = SMALLEST_FLOAT, maxY = SMALLEST_FLOAT, maxZ = SMALLEST_FLOAT;
   C_Vertex min, max;

   C_Mesh *mesh = meshes;
   while(mesh) {
      mesh->calculateBbox();

      mesh->bbox.GetMax(&max);
      mesh->bbox.GetMin(&min);

      if(min.x < minX) minX = min.x;
      if(min.y < minY) minY = min.y;
      if(min.z < minZ) minZ = min.z;

      if(max.x > maxX) maxX = max.x;
      if(max.y > maxY) maxY = max.y;
      if(max.z > maxZ) maxZ = max.z;

      mesh = mesh->next;
   }

   bbox.SetMax(maxX , maxY , maxZ);
   bbox.SetMin(minX , minY , minZ);
   bbox.SetVertices();
}

bool
C_MeshGroup::loadFromFile(const char *filename)
{
   glmReadOBJ(filename, this);
   calculateBbox();
   applyFrustumCulling = nTriangles > 10 ? true : false;

   return true;
}

void
C_MeshGroup::drawNormals(C_Camera *camera)
{
   glUseProgram(0);

   glPushMatrix();
   glTranslatef(position.x, position.y, position.z);

   C_Mesh *mesh = meshes;
   while(mesh) {
      mesh->drawNormals();
      mesh = mesh->next;
   }

   glPopMatrix();
}

void
C_Mesh::drawNormals(void)
{
   glBegin(GL_LINES);
   for(int i = 0; i < nTriangles; i++) {
      /// Draw normals
      glColor4f(0.0f, 0.0f, 1.0f, 1.0f);

      glVertex3f(vertices[3 * i].x, vertices[3 * i].y, vertices[3 * i].z);
      glVertex3f(vertices[3 * i].x + normals[3 * i].x, vertices[3 * i].y + normals[3 * i].y, vertices[3 * i].z + normals[3 * i].z);

      glVertex3f(vertices[3 * i + 1].x, vertices[3 * i + 1].y, vertices[3 * i + 1].z);
      glVertex3f(vertices[3 * i + 1].x + normals[3 * i + 1].x, vertices[3 * i + 1].y + normals[3 * i + 1].y, vertices[3 * i + 1].z + normals[3 * i + 1].z);

      glVertex3f(vertices[3 * i + 2].x, vertices[3 * i + 2].y, vertices[3 * i + 2].z);
      glVertex3f(vertices[3 * i + 2].x + normals[3 * i + 2].x, vertices[3 * i + 2].y + normals[3 * i + 2].y, vertices[3 * i + 2].z + normals[3 * i + 2].z);

      /// Draw tangents
      glColor4f(1.0f, 0.0f, 0.0f, 1.0f);

      glVertex3f(vertices[3 * i].x, vertices[3 * i].y, vertices[3 * i].z);
      glVertex3f(vertices[3 * i].x + tangents[3 * i].x, vertices[3 * i].y + tangents[3 * i].y, vertices[3 * i].z + tangents[3 * i].z);

      glVertex3f(vertices[3 * i + 1].x, vertices[3 * i + 1].y, vertices[3 * i + 1].z);
      glVertex3f(vertices[3 * i + 1].x + tangents[3 * i + 1].x, vertices[3 * i + 1].y + tangents[3 * i + 1].y, vertices[3 * i + 1].z + tangents[3 * i + 1].z);

      glVertex3f(vertices[3 * i + 2].x, vertices[3 * i + 2].y, vertices[3 * i + 2].z);
      glVertex3f(vertices[3 * i + 2].x + tangents[3 * i + 2].x, vertices[3 * i + 2].y + tangents[3 * i + 2].y, vertices[3 * i + 2].z + tangents[3 * i + 2].z);

      /// Draw binormals
      glColor4f(0.0f, 1.0f, 0.0f, 1.0f);

      glVertex3f(vertices[3 * i].x, vertices[3 * i].y, vertices[3 * i].z);
      glVertex3f(vertices[3 * i].x + binormals[3 * i].x, vertices[3 * i].y + binormals[3 * i].y, vertices[3 * i].z + binormals[3 * i].z);

      glVertex3f(vertices[3 * i + 1].x, vertices[3 * i + 1].y, vertices[3 * i + 1].z);
      glVertex3f(vertices[3 * i + 1].x + binormals[3 * i + 1].x, vertices[3 * i + 1].y + binormals[3 * i + 1].y, vertices[3 * i + 1].z + binormals[3 * i + 1].z);

      glVertex3f(vertices[3 * i + 2].x, vertices[3 * i + 2].y, vertices[3 * i + 2].z);
      glVertex3f(vertices[3 * i + 2].x + binormals[3 * i + 2].x, vertices[3 * i + 2].y + binormals[3 * i + 2].y, vertices[3 * i + 2].z + binormals[3 * i + 2].z);
   }
   glEnd();
}

void
C_Mesh::translate(C_Vertex *translation)
{
   translate(translation->x, translation->y, translation->z);
}

void
C_Mesh::translate(float x, float y, float z)
{
   /// Update group's bbox
   bbox.Translate(x, y, z);
}

void
C_Mesh::rotate(C_Vertex *rotation)
{
   rotate(rotation->x, rotation->y, rotation->z);
}

void
C_Mesh::rotate(float x, float y, float z)
{
   C_Vector3 position;

   if(group) {
      position = group->position;
   }

   bbox.Rotate(x, y, z, &position);
}

void
C_MeshGroup::translate(C_Vertex *translation)
{
   translate(translation->x, translation->y, translation->z);
}

void
C_MeshGroup::translate(float x, float y, float z)
{
   position.x += x;
   position.y += y;
   position.z += z;

   C_Mesh *mesh = meshes;
   while(mesh) {
      mesh->translate(x, y, z);
      mesh = mesh->next;
   }

   /// Update group's bbox
   bbox.Translate(x, y, z);
}

void
C_MeshGroup::rotate(C_Vertex *rotation)
{
   rotate(rotation->x, rotation->y, rotation->z);
}

void
C_MeshGroup::rotate(float x, float y, float z)
{
   C_Quaternion tempQuat;
   tempQuat.Rotate(x, y, z);

   rotationQuat.Mult(&tempQuat);
   rotated = true;

   C_Mesh *mesh = meshes;
   while(mesh) {
      mesh->rotate(x, y, z);
      mesh = mesh->next;
   }

   /// Rotate bbox
   C_Vector3 rotPoint(&position);
   bbox.Rotate(x, y, z, &rotPoint);
}

bool
C_MeshGroup::draw(C_Camera *camera)
{
   if(ENABLE_MESH_FRUSTUM_CULLING && applyFrustumCulling) {
      if(!camera->frustum->cubeInFrustum(&bbox)) {
         return false;
      }
   }

	shaderManager->pushShader(shader);
	ESMatrix mat;

   /// Apply camera transformation
	esMatrixMultiply(&mat, &matrix, &globalViewMatrix);
	/// Apply model translation
	esTranslate(&mat, position.x, position.y, position.z);
	/// Apply model rotation
	if(rotated) {
	   ESMatrix rotMat;
	   rotationQuat.QuaternionToMatrix16(&rotMat);
   	esMatrixMultiply(&mat, &rotMat, &mat);
      rotated = false;
   }

   /// Compute MVP matrix
	esMatrixMultiply(&globalMVPMatrix, &mat, &globalProjectionMatrix);

   shader->setUniformMatrix4fv(UNIFORM_VARIABLE_NAME_MODELVIEW_MATRIX, 1, GL_FALSE, (GLfloat *)&mat.m[0][0]);
//   shader->setUniformMatrix4fv(UNIFORM_VARIABLE_NAME_MODEL_MATRIX, 1, GL_FALSE, (GLfloat *)&matrix.m[0][0]);
//   shader->setUniformMatrix4fv(UNIFORM_VARIABLE_NAME_PROJECTION_MATRIX, 1, GL_FALSE, (GLfloat *)&globalProjectionMatrix.m[0][0]);
   shader->setUniformMatrix4fv(UNIFORM_VARIABLE_NAME_MVP_MATRIX, 1, GL_FALSE, (GLfloat *)&globalMVPMatrix.m[0][0]);

   if(shader->GetUniLoc(UNIFORM_VARIABLE_LIGHT_POSITION) >= 0)
      shader->setUniform3f(UNIFORM_VARIABLE_LIGHT_POSITION, lightPosition.x, lightPosition.y, lightPosition.z);

   if(shader->verticesAttribLocation >= 0)   glEnableVertexAttribArray(shader->verticesAttribLocation);
   if(shader->colorsAttribLocation >= 0)     glEnableVertexAttribArray(shader->colorsAttribLocation);
   if(shader->texCoordsAttribLocation >= 0)  glEnableVertexAttribArray(shader->texCoordsAttribLocation);
   if(shader->normalsAttribLocation >= 0)    glEnableVertexAttribArray(shader->normalsAttribLocation);
   if(shader->binormalsAttribLocation >= 0)  glEnableVertexAttribArray(shader->binormalsAttribLocation);
   if(shader->tangetsAttribLocation >= 0)    glEnableVertexAttribArray(shader->tangetsAttribLocation);

   C_Mesh *mesh = meshes;
   while(mesh) {
      /// If mesh has texture enable it
      if(mesh->texture_diffuse && shader->textureDiffuseLocation_0 >= 0) {
         glActiveTexture(GL_TEXTURE0);
         glBindTexture(GL_TEXTURE_2D, mesh->texture_diffuse->getGLtextureID());
         shader->setUniform1i(UNIFORM_VARIABLE_NAME_TEXTURE_DIFFUSE, 0);
      }

      if(mesh->texture_normal && shader->textureNormalMapLocation_1 >= 0) {
         glActiveTexture(GL_TEXTURE1);
         glBindTexture(GL_TEXTURE_2D, mesh->texture_normal->getGLtextureID());
         shader->setUniform1i(UNIFORM_VARIABLE_NAME_TEXTURE_NORMAL_MAP, 1);
      }

      if(mesh->texture_specular && shader->textureSpecularLocation_2 >= 0) {
         glActiveTexture(GL_TEXTURE2);
         glBindTexture(GL_TEXTURE_2D, mesh->texture_specular->getGLtextureID());
         shader->setUniform1i(UNIFORM_VARIABLE_NAME_TEXTURE_SPECULAR, 2);
      }

      if(!mesh->texture_diffuse && !mesh->texture_normal && !mesh->texture_specular) {
         glBindTexture(GL_TEXTURE_2D, 0);
      }

      mesh->draw(shader);
      mesh = mesh->next;
   }

//   bbox.Draw();

   if(shader->verticesAttribLocation >= 0)   glDisableVertexAttribArray(shader->verticesAttribLocation);
   if(shader->colorsAttribLocation >= 0)     glDisableVertexAttribArray(shader->colorsAttribLocation);
   if(shader->texCoordsAttribLocation >= 0)  glDisableVertexAttribArray(shader->texCoordsAttribLocation);
   if(shader->normalsAttribLocation >= 0)    glDisableVertexAttribArray(shader->normalsAttribLocation);
   if(shader->binormalsAttribLocation >= 0)  glDisableVertexAttribArray(shader->binormalsAttribLocation);
   if(shader->tangetsAttribLocation >= 0)    glDisableVertexAttribArray(shader->tangetsAttribLocation);

   shaderManager->popShader();

   return true;
}

void
C_Mesh::draw(C_GLShader *shader)
{
   if(shader->verticesAttribLocation >= 0) {
      glBindBuffer(GL_ARRAY_BUFFER, vbos[VERTICES_VBO]);
      glVertexAttribPointer(shader->verticesAttribLocation,  sizeof(C_Vertex)   / sizeof(float), GL_FLOAT, GL_FALSE, 0, 0);
   }

   if(shader->colorsAttribLocation >= 0) {
      glBindBuffer(GL_ARRAY_BUFFER, vbos[COLORS_VBO]);
      glVertexAttribPointer(shader->colorsAttribLocation,    sizeof(C_Vertex)   / sizeof(float), GL_FLOAT, GL_FALSE, 0, 0);
   }

   if(shader->texCoordsAttribLocation >= 0) {
      glBindBuffer(GL_ARRAY_BUFFER, vbos[TEXCOORDS_VBO]);
      glVertexAttribPointer(shader->texCoordsAttribLocation, sizeof(C_TexCoord) / sizeof(float), GL_FLOAT, GL_FALSE, 0, 0);
   }

   if(shader->normalsAttribLocation >= 0) {
      glBindBuffer(GL_ARRAY_BUFFER, vbos[NORMALS_VBO]);
      glVertexAttribPointer(shader->normalsAttribLocation,   sizeof(C_Vertex)   / sizeof(float), GL_FLOAT, GL_FALSE, 0, 0);
   }

   if(shader->binormalsAttribLocation >= 0) {
      glBindBuffer(GL_ARRAY_BUFFER, vbos[BINORMALS_VBO]);
      glVertexAttribPointer(shader->binormalsAttribLocation, sizeof(C_Vertex)   / sizeof(float), GL_FLOAT, GL_FALSE, 0, 0);
   }

   if(shader->tangetsAttribLocation >= 0) {
      glBindBuffer(GL_ARRAY_BUFFER, vbos[TANGENTS_VBO]);
      glVertexAttribPointer(shader->tangetsAttribLocation,   sizeof(C_Vertex)   / sizeof(float), GL_FLOAT, GL_FALSE, 0, 0);
   }

   if(!indices) {
      glDrawArrays(GL_TRIANGLES, 0, nVertices);
   } else {
      assert(0);
      glDrawElements(GL_TRIANGLES, 3 * nTriangles, GL_UNSIGNED_INT, indices);
   }
}

bool
C_MeshGroup::initVBOS(void)
{
   C_Mesh *mesh = meshes;

   while(mesh) {
      if(mesh->nVertices) {
         glGenBuffers(1, &mesh->vbos[VERTICES_VBO]);

         if(!mesh->vbos[VERTICES_VBO]) {
            assert(0);
            return false;
         }

         glBindBuffer(GL_ARRAY_BUFFER, mesh->vbos[VERTICES_VBO]);
         glBufferData(GL_ARRAY_BUFFER, mesh->nVertices * sizeof(C_Vertex), mesh->vertices, GL_STATIC_DRAW);

         delete[] mesh->vertices;
         mesh->vertices = NULL;
      }

      if(mesh->normals) {
         glGenBuffers(1, &mesh->vbos[NORMALS_VBO]);

         if(!mesh->vbos[NORMALS_VBO]) {
            assert(0);
            return false;
         }

         glBindBuffer(GL_ARRAY_BUFFER, mesh->vbos[NORMALS_VBO]);
         glBufferData(GL_ARRAY_BUFFER, mesh->nVertices * sizeof(C_Vertex), mesh->normals, GL_STATIC_DRAW);

         delete[] mesh->normals;
         mesh->normals = NULL;
      }

      if(mesh->tangents) {
         glGenBuffers(1, &mesh->vbos[TANGENTS_VBO]);

         if(!mesh->vbos[TANGENTS_VBO]) {
            assert(0);
            return false;
         }

         glBindBuffer(GL_ARRAY_BUFFER, mesh->vbos[TANGENTS_VBO]);
         glBufferData(GL_ARRAY_BUFFER, mesh->nVertices * sizeof(C_Vertex), mesh->tangents, GL_STATIC_DRAW);

         delete[] mesh->tangents;
         mesh->tangents = NULL;
      }

      if(mesh->binormals) {
         glGenBuffers(1, &mesh->vbos[BINORMALS_VBO]);

         if(!mesh->vbos[BINORMALS_VBO]) {
            assert(0);
            return false;
         }

         glBindBuffer(GL_ARRAY_BUFFER, mesh->vbos[BINORMALS_VBO]);
         glBufferData(GL_ARRAY_BUFFER, mesh->nVertices * sizeof(C_Vertex), mesh->binormals, GL_STATIC_DRAW);

         delete[] mesh->binormals;
         mesh->binormals = NULL;
      }

      if(mesh->textCoords) {
         glGenBuffers(1, &mesh->vbos[TEXCOORDS_VBO]);

         if(!mesh->vbos[TEXCOORDS_VBO]) {
            assert(0);
            return false;
         }

         glBindBuffer(GL_ARRAY_BUFFER, mesh->vbos[TEXCOORDS_VBO]);
         glBufferData(GL_ARRAY_BUFFER, mesh->nVertices * sizeof(C_TexCoord), mesh->textCoords, GL_STATIC_DRAW);

         delete[] mesh->textCoords;
         mesh->textCoords = NULL;
      }

      mesh = mesh->next;
   }

   return true;
}
