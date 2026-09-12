#pragma once
#include "../search/asyncbot.h"
#include "boardhistory.h"

class Search;

namespace RandomOpening {

  struct Opening {
    Board board;
    BoardHistory hist;
    Player nextPlayer;
    Opening(int x_size, int y_size);
  };
  std::vector<Opening> getOpenings(Search* botB, Search* botW, Rand& gameRand, int cnt, int x, int y);

  void initRandomOpening(Board& board, BoardHistory& hist, Player& nextPlayer, Rand& gameRand);

  void initializeCross(Board& board, BoardHistory& hist, Player& nextPlayer, Rand& gameRand);

  void initBalanced(Search* botB, Search* botW, Board& board, BoardHistory& hist, Player& nextPlayer, Rand& gameRand);
  void initHub(Search* botB, Search* botW, Board& board, BoardHistory& hist, Player& nextPlayer, Rand& gameRand);


}  // namespace RandomOpening
