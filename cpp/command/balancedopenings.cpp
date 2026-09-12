#include "../core/global.h"
#include "../core/config_parser.h"
#include "../core/makedir.h"
#include "../dataio/loadmodel.h"
#include "../search/search.h"
#include "../game/randomopening.h"
#include "../program/setup.h"
#include "../command/commandline.h"
#include "../main.h"

#include <fstream>
#include <string>
#include <vector>

using namespace std;

// Write a bare SGF of a single position (no move history) using AB/AW,
// plus a PL tag indicating the side to move.
static void writeSgfPosition(ostream& out, const Board& board, Player nextPla) {
  int xSize = board.x_size;
  int ySize = board.y_size;
  out << "(;FF[4]GM[1]";
  if(xSize == ySize)
    out << "SZ[" << xSize << "]";
  else
    out << "SZ[" << xSize << ":" << ySize << "]";
  out << "PL[" << (nextPla == P_BLACK ? "B" : "W") << "]";

  auto writeLoc = [&](Loc loc) {
    int x = Location::getX(loc, xSize);
    int y = Location::getY(loc, xSize);
    static const char* chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    out << chars[x] << chars[y];
  };

  bool hasAB = false;
  for(int y = 0; y < ySize; y++) {
    for(int x = 0; x < xSize; x++) {
      Loc loc = Location::getLoc(x, y, xSize);
      Color c = board.colors[loc];
      if(c == C_BLACK || c == C_BLACK_CAPTURED) {
        if(!hasAB) {
          out << "AB";
          hasAB = true;
        }
        out << "[";
        writeLoc(loc);
        out << "]";
      }
    }
  }

  bool hasAW = false;
  for(int y = 0; y < ySize; y++) {
    for(int x = 0; x < xSize; x++) {
      Loc loc = Location::getLoc(x, y, xSize);
      Color c = board.colors[loc];
      if(c == C_WHITE || c == C_WHITE_CAPTURED) {
        if(!hasAW) {
          out << "AW";
          hasAW = true;
        }
        out << "[";
        writeLoc(loc);
        out << "]";
      }
    }
  }

  out << ")";
}

int MainCmds::genbalancedopenings(const vector<string>& args) {
  Board::initHash();
  Rand seedRand;

  ConfigParser cfg;
  string modelsDir;
  string sgfDir;
  int numToGen;
  int xSize;
  int ySize;
  try {
    KataGoCommandLine cmd("Generate balanced-opening SGFs");
    cmd.addConfigFileArg("","");
    cmd.addOverrideConfigArg();

    TCLAP::ValueArg<string> modelsDirArg("","models-dir","Dir to poll and load latest model from",true,string(),"DIR");
    TCLAP::ValueArg<string> sgfDirArg("","sgf-dir","Dir to output SGFs to",true,string(),"DIR");
    TCLAP::ValueArg<int> numToGenArg("","num","Number of openings to generate",false,8,"N");
    TCLAP::ValueArg<int> xSizeArg("","xsize","Board x size",false,20,"N");
    TCLAP::ValueArg<int> ySizeArg("","ysize","Board y size",false,20,"N");
    cmd.add(modelsDirArg);
    cmd.add(sgfDirArg);
    cmd.add(numToGenArg);
    cmd.add(xSizeArg);
    cmd.add(ySizeArg);
    cmd.parseArgs(args);

    modelsDir = modelsDirArg.getValue();
    sgfDir = sgfDirArg.getValue();
    numToGen = numToGenArg.getValue();
    xSize = xSizeArg.getValue();
    ySize = ySizeArg.getValue();

    auto checkDirNonEmpty = [](const char* flag, const string& s) {
      if(s.length() <= 0)
        throw StringError("Empty directory specified for " + string(flag));
    };
    checkDirNonEmpty("models-dir",modelsDir);
    checkDirNonEmpty("sgf-dir",sgfDir);

    cmd.getConfigAllowEmpty(cfg);
  }
  catch (TCLAP::ArgException &e) {
    cerr << "Error: " << e.error() << " for argument " << e.argId() << endl;
    return 1;
  }

  MakeDir::make(sgfDir);
  MakeDir::make(modelsDir);

  Logger logger(&cfg, true);

  Setup::initializeSession(cfg);

  string modelName;
  string modelFile;
  string modelDir;
  time_t modelTime;
  LoadModel::findLatestModel(modelsDir, logger, modelName, modelFile, modelDir, modelTime);
  if(modelFile == "/dev/null") {
    logger.write("No model found in models-dir: " + modelsDir + ", doing nothing");
    return 0;
  }

  NNEvaluator* nnEval = Setup::initializeNNEvaluator(
    modelName,modelFile,"",cfg,logger,seedRand,
    32,1,
    Board::MAX_LEN,Board::MAX_LEN,8,
    false,false,
    Setup::SETUP_FOR_GTP
  );
  logger.write("Loaded latest neural net " + modelName + " from: " + modelFile);

  SearchParams params;
  params.numThreads = 1;
  Search* search = new Search(params, nnEval, &logger, Global::uint64ToString(seedRand.nextUInt64()));

  Rand rand;

  std::vector<RandomOpening::Opening> res = RandomOpening::getOpenings(search, search, rand, numToGen, xSize, ySize);

  int i = 0;
  for(const RandomOpening::Opening& elem: res) {
    bool captured = false;
    for(Color c: elem.board.colors) {
      if(c == C_BLACK_CAPTURED || c == C_WHITE_CAPTURED) {
        captured = true;
        break;
      }
    }
    if(!captured) {
      ofstream out(sgfDir + "/opening_b" + std::to_string(xSize) + "x" + std::to_string(ySize) + "_" + std::to_string(i) + ".sgf");
      writeSgfPosition(out, elem.board, elem.nextPlayer);
      i++;
    }
  }

  nnEval->killServerThreads();
  delete search;
  delete nnEval;

  logger.write("Done, wrote " + Global::intToString(i) + " openings to " + sgfDir);
  return 0;
}
