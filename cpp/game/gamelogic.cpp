#include "../game/gamelogic.h"
#include "board.h"

/*
 * gamelogic.cpp
 * Logics of game rules
 * Some other game logics are in board.h/cpp
 */

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

using namespace std;

void DfsHelper2(const Board& board, int x, int y, Color color) {
  const int loc = Location::getLoc(x, y, board.x_size);
  if(board.dfs_buf[loc] & 2) {
    return;
  }
  const int board_clr = board.colors[loc];
  const int color_op = getOpp(color);
  if (!(board_clr == color || board_clr == color_op + 3 || (board_clr == C_EMPTY && ((board.dfs_buf[loc] & 1) == 1)))) {
    return;
  }
  board.dfs_buf[loc] |= 2;

  if(x > 0) {
    DfsHelper2(board, x - 1, y, color);
  }
  if(x + 1 < board.x_size) {
    DfsHelper2(board,x + 1, y, color);
  }
  if(y > 0) {
    DfsHelper2(board, x, y - 1, color);
  }
  if(y + 1 < board.y_size) {
    DfsHelper2(board,x, y + 1, color);
  }
  return;
}

Color GameLogic::checkWinnerAfterPlayed(
  const Board& board,
  const BoardHistory& hist,
  Player pla,
  Loc loc) {
    if (loc == 1)  { // PASS
        ASSERT_UNREACHABLE;
    }
    const Color color = pla;
    const Color opp_clr = getOpp(pla);
    
  for(int i = 0; i < board.MAX_ARR_SIZE; i++) {
    board.dfs_buf[i] = 0;
  }
  for(int y = 0; y < board.y_size; y++) {
    for(int x = 0; x < board.x_size; x++) {
      const Loc lc = Location::getLoc(x, y, board.x_size);
      if(board.colors[lc] == C_BLACK_CAPTURED) {
        board.bufDfs(x, y, C_WHITE);
      }
    }
  }

  for(int i = 0; i < board.x_size; i++) {
    DfsHelper2(board, i, 0, C_WHITE);
    DfsHelper2(board, i, board.y_size - 1, C_WHITE);
  }
  for(int i = 0; i < board.y_size; i++) {
    DfsHelper2(board, 0, i, C_WHITE);
    DfsHelper2(board, board.x_size - 1, i, C_WHITE);
  }
  int notsafe_white = 0;
  for(int i = 0; i < board.MAX_ARR_SIZE; i++) {
        if (board.colors[i] == C_WHITE && ((board.dfs_buf[i] & 2) == 0) ) {
            notsafe_white++;
        }
    }

  int captured_black = 0;
  int maybe_captured_black = 0;
  for(int i = 0; i < board.MAX_ARR_SIZE; i++) {
        if (board.colors[i] == C_BLACK_CAPTURED  ) {
            if ((board.dfs_buf[i] & 2) == 2) {
                captured_black++;
            } else {
                maybe_captured_black++;
            }
        }
    }




  for(int i = 0; i < board.MAX_ARR_SIZE; i++) {
    board.dfs_buf[i] = 0;
  }
  for(int y = 0; y < board.y_size; y++) {
    for(int x = 0; x < board.x_size; x++) {
      const Loc lc = Location::getLoc(x, y, board.x_size);
      if(board.colors[lc] == C_WHITE_CAPTURED) {
        board.bufDfs(x, y, C_BLACK);
      }
    }
  }


  for(int i = 0; i < board.x_size; i++) {
    DfsHelper2(board, i, 0, C_BLACK);
    DfsHelper2(board, i, board.y_size - 1, C_BLACK);
  }
  for(int i = 0; i < board.y_size; i++) {
    DfsHelper2(board, 0, i, C_BLACK);
    DfsHelper2(board, board.x_size - 1, i, C_BLACK);
  }
  int notsafe_black = 0;
  for(int i = 0; i < board.MAX_ARR_SIZE; i++) {
        if (board.colors[i] == C_BLACK && ((board.dfs_buf[i] & 2) == 0) ) {
            notsafe_black++;
        }
    }

  int captured_white = 0;
  int maybe_captured_white = 0;

  for(int i = 0; i < board.MAX_ARR_SIZE; i++) {
        if (board.colors[i] == C_WHITE_CAPTURED) {
            if ((board.dfs_buf[i] & 2) == 2) {
                captured_white++;
            } else {
                maybe_captured_white++;
            }
        }
    }
  if (captured_white > captured_black + maybe_captured_black + notsafe_black) {
        return C_BLACK;
    }

  if (captured_black > captured_white + maybe_captured_white + notsafe_white) {
        return C_WHITE;
    }
    if (notsafe_white == 0 && notsafe_black == 0) {
        assert(maybe_captured_black == 0);
        assert(maybe_captured_white == 0);
        return C_WHITE;
    }
    return C_WALL;
}
