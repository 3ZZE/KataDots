/*
 * gamelogic.h
 * Logics of game rules
 * Some other game logics are in board.h/cpp
 * 
 * Gomoku as a representive
 */

#ifndef GAME_GAMELOGIC_H_
#define GAME_GAMELOGIC_H_

#include "../game/boardhistory.h"

/*
* Other game logics:
* Board::
*/

namespace GameLogic {

  //C_EMPTY = draw, C_WALL = not finished 
  Color checkWinnerAfterPlayed( const Board& board, const BoardHistory& hist, Player pla, Loc loc);

}




#endif // GAME_RULELOGIC_H_
