#include "../game/randomopening.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include "../search/asyncbot.h"
#include "board.h"
#include "boardhistory.h"
using namespace RandomOpening;
using namespace std;

static Loc getRandomGaussianCenterEmptyLoc(Board& board, Rand& gameRand, double exponent) {
  int xsize = board.x_size;
  int ysize = board.y_size;
  const double centerX = 0.5 * (xsize - 1);
  const double centerY = 0.5 * (ysize - 1);
  const double sigma = 0.5 * std::max(xsize, ysize);

  std::vector<Loc> locs;
  std::vector<double> weights;
  double totalWeight = 0.0;

  for(int y = 0; y < ysize; y++) {
    for(int x = 0; x < xsize; x++) {
      Loc loc = Location::getLoc(x, y, xsize);
      if(board.colors[loc] != C_EMPTY)
        continue;
      if(x == 0 || x == xsize - 1 || y == 0 || y == ysize - 1)
        continue;
      double dx = (x - centerX) / sigma;
      double dy = (y - centerY) / sigma;
      double w = std::exp(-(dx * dx + dy * dy) * exponent);
      locs.push_back(loc);
      weights.push_back(w);
      totalWeight += w;
    }
  }

  if(locs.empty() || totalWeight <= 0.0)
    return Board::NULL_LOC;

  double r = gameRand.nextDouble(totalWeight);
  double cum = 0.0;
  for(size_t i = 0; i < locs.size(); i++) {
    cum += weights[i];
    if(cum >= r)
      return locs[i];
  }
  return locs[locs.size() - 1];
}

static double getBoardDisbalance(
  Search* bot,
  const Board& board,
  const BoardHistory& hist,
  Player nextPlayer,
  Player playerToBalance) {
  NNEvaluator* nnEval = bot->nnEvaluator;
  MiscNNInputParams nnInputParams;
  NNResultBuf buf;
  nnEval->evaluate(board, hist, nextPlayer, nnInputParams, buf, false);
  std::shared_ptr<NNOutput> nnOutput = std::move(buf.result);
  if(playerToBalance == P_WHITE) {
    return std::fabs(nnOutput->whiteLossProb - 0.5);
  } else {
    return std::fabs(nnOutput->whiteWinProb - 0.5);
  }
}

static void makeEqualMove(Search* bot, Board& board, BoardHistory& hist, Player& nextPlayer, Player& playerToBalance) {
  int xsize = board.x_size;
  int ysize = board.y_size;

  Loc bestLoc = Board::NULL_LOC;
  double bestAbsValue = 1e30;

  for(int x = 0; x < xsize; x++) {
    for(int y = 0; y < ysize; y++) {
      Loc loc = Location::getLoc(x, y, xsize);
      if(!board.isLegal(loc, nextPlayer))
        continue;

      Board boardCopy(board);
      BoardHistory histCopy(hist);
      histCopy.makeBoardMoveAssumeLegal(boardCopy, loc, nextPlayer);
      if(histCopy.isGameFinished)
        continue;

      double value = getBoardDisbalance(bot, boardCopy, histCopy, getOpp(nextPlayer), playerToBalance);
      double absValue = std::fabs(value);
      if(absValue < bestAbsValue) {
        bestAbsValue = absValue;
        bestLoc = loc;
      }
    }
  }

  if(bestLoc == Board::NULL_LOC)
    return;

  hist.makeBoardMoveAssumeLegal(board, bestLoc, nextPlayer);
  nextPlayer = getOpp(nextPlayer);
}

static int getNumStones(Rand& gameRand, std::vector<float>& randomMoveNumProb) {
  int maxRandomMoveNum = randomMoveNumProb.size();
  double randomMoveNumProbTotal = 0;
  for(int i = 0; i < maxRandomMoveNum; i++)
    randomMoveNumProbTotal += randomMoveNumProb[i];
  double randomMoveNumProbSum = 0;
  double randomMoveNumProbRandomDouble = gameRand.nextDouble() * randomMoveNumProbTotal - 1e-7;
  int randomMoveNum = -1;
  for(int i = 0; i < maxRandomMoveNum; i++) {
    randomMoveNumProbSum += randomMoveNumProb[i];
    if(randomMoveNumProbSum >= randomMoveNumProbRandomDouble) {
      randomMoveNum = i;
      break;
    }
  }
  if(randomMoveNum == -1)
    ASSERT_UNREACHABLE;

  return randomMoveNum;
}

void RandomOpening::initRandomOpening(Board& board, BoardHistory& hist, Player& nextPlayer, Rand& gameRand) {
  std::vector<float> exponents = vector<float>{20., 17.0, 17., 7., 14., 7., 14., 7., 7., 7.};
  std::vector<float> randomMoveNumProb = vector<float>{0.0, 0.0, 5., 1.5, 10., 3, 8, 1, 2};
  int numStones = getNumStones(gameRand, randomMoveNumProb);

  Player pla = P_BLACK;
  for(int i = 0; i < numStones; i++) {
    Loc loc = getRandomGaussianCenterEmptyLoc(board, gameRand, exponents[i]);
    if(loc == Board::NULL_LOC)
      break;
    if(!board.isLegal(loc, pla)) {
      break;
    }
    hist.makeBoardMoveAssumeLegal(board, loc, pla);
    nextPlayer = getOpp(pla);
    if(hist.isGameFinished)
      break;
    pla = nextPlayer;
  }
}

void RandomOpening::initializeCross(Board& board, BoardHistory& hist, Player& nextPlayer, Rand& gameRand) {
  assert(board.x_size > 9 && board.y_size > 9);
  int dx = gameRand.nextUInt(7) - 3;
  int dy = gameRand.nextUInt(7) - 3;
  int xh = board.x_size / 2 + dx;
  int yh = board.y_size / 2 + dy;

  if(gameRand.nextBool(0.5)) {
    int loc = Location::getLoc(xh, yh, board.x_size);
    hist.makeBoardMoveAssumeLegal(board, loc, P_BLACK);
    loc = Location::getLoc(xh - 1, yh, board.x_size);
    hist.makeBoardMoveAssumeLegal(board, loc, P_WHITE);
    loc = Location::getLoc(xh - 1, yh - 1, board.x_size);
    hist.makeBoardMoveAssumeLegal(board, loc, P_BLACK);
    loc = Location::getLoc(xh, yh - 1, board.x_size);
    hist.makeBoardMoveAssumeLegal(board, loc, P_WHITE);
  } else {
    int loc = Location::getLoc(xh - 1, yh, board.x_size);
    hist.makeBoardMoveAssumeLegal(board, loc, P_BLACK);
    loc = Location::getLoc(xh, yh, board.x_size);
    hist.makeBoardMoveAssumeLegal(board, loc, P_WHITE);
    loc = Location::getLoc(xh, yh - 1, board.x_size);
    hist.makeBoardMoveAssumeLegal(board, loc, P_BLACK);
    loc = Location::getLoc(xh - 1, yh - 1, board.x_size);
    hist.makeBoardMoveAssumeLegal(board, loc, P_WHITE);
  }
  nextPlayer = P_BLACK;
}

static bool
tryInitBalanced(Search* botB, Search* botW, Board& board, BoardHistory& hist, Player& nextPlayer, Rand& gameRand) {
  Board boardCopy(board);
  BoardHistory histCopy(hist);
  Player nextPlayerCopy = nextPlayer;

  std::vector<float> randomMoveNumProb = vector<float>{0.0, 0.0, 1., 10., 2., 8., 2., 0., 0.};
  int randomMoveNum = getNumStones(gameRand, randomMoveNumProb);

  std::vector<float> exponents = vector<float>{20., 17.0, 17., 7., 14., 7., 14., 7., 7., 7.};

  Player pla = nextPlayerCopy;
  for(int i = 0; i < randomMoveNum; i++) {
    Loc loc = getRandomGaussianCenterEmptyLoc(boardCopy, gameRand, exponents[i]);
    if(loc == Board::NULL_LOC)
      return false;
    if(!boardCopy.isLegal(loc, pla))
      return false;
    histCopy.makeBoardMoveAssumeLegal(boardCopy, loc, pla);
    nextPlayerCopy = getOpp(pla);
    pla = nextPlayerCopy;
  }

  Player oppla = gameRand.nextBool(0.5) ? P_BLACK : P_WHITE;
  Search* bot = gameRand.nextBool(0.5) ? botB : botW;
  for(int i = 0; i < 3; i++) {
    makeEqualMove(bot, boardCopy, histCopy, nextPlayerCopy, oppla);
  }

  board = boardCopy;
  hist = histCopy;
  nextPlayer = nextPlayerCopy;
  return true;
}

void RandomOpening::initBalanced(
  Search* botB,
  Search* botW,
  Board& board,
  BoardHistory& hist,
  Player& nextPlayer,
  Rand& gameRand) {
  const int maxTryTimes = 100;
  int tryTimes = 0;
  while(!tryInitBalanced(botB, botW, board, hist, nextPlayer, gameRand)) {
    tryTimes++;
    if(tryTimes > maxTryTimes) {
      tryTimes = 0;
      std::cout << "Reached max trying times for finding balanced openings, Rule=" << hist.rules.toString()
                << std::endl;
      return;
    }
  }
}
Opening::Opening(int x, int y) {
  board = Board(x, y);
  hist = BoardHistory();
  nextPlayer = P_BLACK;
}

std::vector<Opening> RandomOpening::getOpenings(Search* botB, Search* botW, Rand& gameRand, int cnt, int x, int y) {
  std::vector<Opening> vecres;
  for(int i = 0; i < cnt; i++) {
    Opening opening = Opening(x, y);
    RandomOpening::initBalanced(botB, botW, opening.board, opening.hist, opening.nextPlayer, gameRand);
    vecres.push_back(opening);
  }
  return vecres;
}

void RandomOpening::initHub(
  Search* botB,
  Search* botW,
  Board& board,
  BoardHistory& hist,
  Player& nextPlayer,
  Rand& gameRand) {
  double rand = gameRand.nextDouble();
  if(rand < 0.0) {
    RandomOpening::initBalanced(botB, botW, board, hist, nextPlayer, gameRand);
  } else if(rand < 0.5) {
    RandomOpening::initRandomOpening(board, hist, nextPlayer, gameRand);
  } else {
    RandomOpening::initializeCross(board, hist, nextPlayer, gameRand);
  }
}
