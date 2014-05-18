#include "battleTile.h"

C_BattleTile::C_BattleTile(C_BattleMap *battleMap, float x, float y, float z, float height) :
C_BattleStaticObject(battleMap, height, (height == 0.f))
{
   m_position.x = x;
   m_position.y = y;
   m_position.z = z;

//   setSize(128.f);
}
