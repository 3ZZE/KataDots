#include "../game/randomopening.h"
#include <algorithm>
#include <cassert>
#include <cmath>
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
      double w = std::exp(-(dx * dx + dy * dy)* exponent);
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

static int getNumStones(Rand& gameRand) {

  std::vector<float> randomMoveNumProb = vector<float>{0.0, 0.0, 5., 1.5, 10., 3, 8, 1, 2};
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

void RandomOpening::initopening2(
  Board& board,
  BoardHistory& hist,
  Player& nextPlayer,
  Rand& gameRand
  ) {

    std::vector<float> exponents = vector<float>{20., 17.0, 17., 7., 14., 7., 14., 7., 7., 7.};
    int numStones = getNumStones(gameRand);


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

void RandomOpening::initializeBalancedRandomOpening(
  Search* botB,
  Search* botW,
  Board& board,
  BoardHistory& hist,
  Player& nextPlayer,
  Rand& gameRand,
  bool forSelfplay) {
  int xh = board.x_size / 2;
  int yh = board.x_size / 2;
  int loc = Location::getLoc(xh, yh, board.x_size);
  hist.makeBoardMoveAssumeLegal(board, loc, P_BLACK);
  loc = Location::getLoc(xh - 1, yh, board.x_size);
  hist.makeBoardMoveAssumeLegal(board, loc, P_WHITE);
  loc = Location::getLoc(xh - 1, yh - 1, board.x_size);
  hist.makeBoardMoveAssumeLegal(board, loc, P_BLACK);
  loc = Location::getLoc(xh, yh - 1, board.x_size);
  hist.makeBoardMoveAssumeLegal(board, loc, P_WHITE);
  nextPlayer = P_BLACK;
}
