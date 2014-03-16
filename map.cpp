#include <stdio.h>
#include "map.h"

bool
C_Map::readMap(const char *filename)
{
   printf("*********\n");
   printf("Reading map file \"%s\"\n", filename);

	int _xTiles, _yTiles, type, area, nTiles, _x, _y;
	int i, c, sum = 0, nWalkable = 0, nVoid = 0;
	char buf[MAX_PARAMETER_LENGTH];
	int counters[N_TILE_TYPES] = {0};

	FILE *fd;

	if((fd = fopen(filename, "r")) == NULL)
		return false;

	fscanf(fd, "%d", &_xTiles);		/// Read size on x.
	fscanf(fd, "%d", &_yTiles);		/// Read size on y.
	fscanf(fd, "%d", &nTiles);		   /// Read number of tiles stored in file.

   assert(_xTiles == TILES_ON_X);
   assert(_yTiles == TILES_ON_Y);
   assert(nTiles == TILES_ON_X * TILES_ON_Y);

	for(i = 0; i < nTiles; i++) {
		fscanf(fd, "%d", &_x);			/// Read x coords
		fscanf(fd, "%d", &_y);			/// Read y coords
		fscanf(fd, "%d", &type);	   /// Read tile type
		c = fscanf(fd, "%d", &area);	/// Read tile area

		assert(_x < TILES_ON_X);
		assert(_y < TILES_ON_Y);
		assert(type < N_TILE_TYPES);
		assert(area < N_AREA_TYPES);

		tiles[_x][_y].setType((tileTypes_t)type);
		tiles[_x][_y].setArea((areaTypes_t)area);
		tiles[_x][_y].setCoordX(_x);
		tiles[_x][_y].setCoordY(_y);

		/// Count tiles
		++counters[type];
		++sum;
		if(area == AREA_VOID) {
		   ++nVoid;
      } else if(area == AREA_WALKABLE) {
         ++nWalkable;
      }

   	/// There's a parameter. Read it.
		if(!c) {
		   /// fgets doesn't stop at white spaces but it stops at '\n'
			fgets(buf, MAX_PARAMETER_LENGTH, fd);
			tiles[_x][_y].setParameter(buf);
		   /// One loop is lost everytime a parameter is read.
         if(area == AREA_VOID) {
            --nVoid;
         } else if(area == AREA_WALKABLE) {
            --nWalkable;
         }

			--i;
			--sum;
         --counters[type];
		}
	}

	fclose(fd);

	printf("\tFound:\n");
	for(int i = 0; i < N_TILE_TYPES; i++) {
	   if(counters[i])
         printf("\tType %d: %d tiles\n", i, counters[i]);
	}
	printf("\t%d walkable and %d void tiles\n", nWalkable, nVoid);

	printf("\tTotal %d tiles\n", sum);
	printf("Done.\n");
   printf("*********\n");

	return true;
}