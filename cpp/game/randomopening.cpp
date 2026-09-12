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

static double getBoardValue(Search* bot, const Board& board, const BoardHistory& hist, Player nextPlayer) {
  NNEvaluator* nnEval = bot->nnEvaluator;
  MiscNNInputParams nnInputParams;
  NNResultBuf buf;
  nnEval->evaluate(board, hist, nextPlayer, nnInputParams, buf, false);
  std::shared_ptr<NNOutput> nnOutput = std::move(buf.result);
  double value = nnOutput->whiteWinProb - nnOutput->whiteLossProb;
  if(nextPlayer == C_BLACK)
    return -value;
  else
    return value;
}

static void makeEqualMove(Search* bot, Board& board, BoardHistory& hist, Player& nextPlayer) {
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

      double value = getBoardValue(bot, boardCopy, histCopy, getOpp(nextPlayer));
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
  int yh = board.x_size / 2 + dy;

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

static Loc getBalanceMove(
  Search* botB,
  Search* botW,
  const Board& board,
  const BoardHistory& hist,
  Player nextPlayer,
  Rand& gameRand,
  bool forSelfplay,
  double rejectProb) {
  int xsize = board.x_size;
  int ysize = board.y_size;

  Search* bot = gameRand.nextBool(0.5) ? botB : botW;
  double maxProb = 0;

  double rootValuePla = getBoardValue(bot, board, hist, nextPlayer);
  if(rootValuePla < 0) {
    double rejectFactor = 1 - std::exp(-3 * rootValuePla * rootValuePla);
    if(gameRand.nextBool(rejectFactor) && gameRand.nextBool(rejectProb))
      return Board::NULL_LOC;
  }

  double rootValueOpp = getBoardValue(bot, board, hist, getOpp(nextPlayer));
  if(rootValueOpp < 0) {
    double rejectFactor = 1 - std::exp(-3 * rootValueOpp * rootValueOpp);
    if(gameRand.nextBool(rejectFactor) && gameRand.nextBool(rejectProb))
      return Board::NULL_LOC;
  }

  bool shouldCheckNearbyStones = rootValueOpp > 0 && board.stonenum > 0;

  std::vector<double> prob(xsize * ysize, 0);
  for(int x = 0; x < xsize; x++) {
    for(int y = 0; y < ysize; y++) {
      Loc loc = Location::getLoc(x, y, xsize);

      if(!board.isLegal(loc, nextPlayer))
        continue;

      if(shouldCheckNearbyStones) {
        bool nearExistingStone = false;
        for(int x1 = x - 3; x1 <= x + 3; x1++) {
          for(int y1 = y - 3; y1 <= y + 3; y1++) {
            if(x1 < 0 || x1 >= xsize || y1 < 0 || y1 >= ysize)
              continue;
            Loc loc1 = Location::getLoc(x1, y1, xsize);
            if(board.colors[loc1] != C_EMPTY)
              nearExistingStone = true;
            if(nearExistingStone)
              break;
          }
          if(nearExistingStone)
            break;
        }
        if(!nearExistingStone)
          continue;
      }

      Board boardCopy(board);
      BoardHistory histCopy(hist);

      histCopy.makeBoardMoveAssumeLegal(boardCopy, loc, nextPlayer);
      if(histCopy.isGameFinished)
        continue;

      double value = getBoardValue(bot, boardCopy, histCopy, getOpp(nextPlayer));

      double p = forSelfplay ? std::pow(1 - value * value, 4) : std::pow(1 - value * value, 10);
      maxProb = std::max(maxProb, p);
      prob[y * xsize + x] = p;
    }
  }

  if(gameRand.nextBool(1 - maxProb) && gameRand.nextBool(rejectProb))
    return Board::NULL_LOC;

  double totalProb = 0;
  for(int x = 0; x < xsize; x++)
    for(int y = 0; y < ysize; y++)
      totalProb += prob[y * xsize + x];

  if(totalProb <= 0.0)
    return Board::NULL_LOC;

  double randomDouble = gameRand.nextDouble(totalProb);
  double probSum = 0;
  for(int x = 0; x < xsize; x++)
    for(int y = 0; y < ysize; y++) {
      probSum += prob[y * xsize + x];
      if(probSum >= randomDouble)
        return Location::getLoc(x, y, xsize);
    }

  return Board::NULL_LOC;
}

static bool tryInitBalanced(
  Search* botB,
  Search* botW,
  Board& board,
  BoardHistory& hist,
  Player& nextPlayer,
  Rand& gameRand
  ) {
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

  Search* bot = gameRand.nextBool(0.5) ? botB : botW;
  for(int i = 0; i < 3; i++) {
    makeEqualMove(bot, boardCopy, histCopy, nextPlayerCopy);
    // Loc loc = getBalanceMove(bot, bot, boardCopy, hist, nextPlayerCopy, gameRand, false, 0.8);
    //     hist.makeBoardMoveAssumeLegal(boardCopy, loc, nextPlayerCopy);
    //     nextPlayerCopy = getOpp(nextPlayerCopy);
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
  while(!tryInitBalanced(botB, botW, board, hist, nextPlayer, gameRand )) {
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

std::vector<Opening> RandomOpening::getOpenings(
  Search* botB,
  Search* botW,
  Rand& gameRand,
    int cnt,
    int x, 
    int y
) {
  std::vector<Opening> vecres;
  for(int i = 0; i < cnt; i++) {
    Opening opening = Opening(x, y);
    RandomOpening::initBalanced(botB, botW, opening.board, opening.hist, opening.nextPlayer, gameRand);
    vecres.push_back(opening);
  }
  return vecres;
}

void RandomOpening::initHub(Search* botB, Search* botW, Board& board, BoardHistory& hist, Player& nextPlayer, Rand& gameRand) {
    double rand = gameRand.nextDouble();
    RandomOpening::initBalanced(botB, botW , board, hist, nextPlayer, gameRand);
    // if (rand < 0.8) {
    // } else if (rand < 0.95) {
    //     RandomOpening::initRandomOpening(board, hist, nextPlayer, gameRand);
    // } else {
    //     RandomOpening::initializeCross(board, hist, nextPlayer, gameRand);
    // }
}
