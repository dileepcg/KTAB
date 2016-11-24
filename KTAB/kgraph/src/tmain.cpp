// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2015 King Abdullah Petroleum Studies and Research Center
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// --------------------------------------------
// start of a simple Tetris clone
//---------------------------------------------

#include "kutils.h"
#include "tutils.h"
#include "prng.h"
#include "tmain.h"
#include "tetrisUI.h"


namespace Tetris {

void tetrisTimer(void*) {
  unsigned int lvl = TApp::theApp->level;
  assert(1 <= lvl);
  assert(lvl <= 10);

  auto app = TApp::theApp;

  app->stepGame();
  TetrisUI::theUI->playW->redraw();
  double dt = TApp::theApp->setDt();


  if (!(app->paused)) {
    double pt = app->playTime + dt;
    double mpt = app->maxPlayTime;
    double frc = pt / mpt;
    app->playTime = pt;
    TetrisUI::theUI->timeProgress->value(frc);
    Fl::check();
  }

  Fl::repeat_timeout(dt, tetrisTimer, nullptr);
  return;
}

TApp::TApp(uint64_t s) {
  rng = new PRNG();
  rng->setSeed(s);
  theApp = this;
  setLevel(defaultLevel);
  setRC(defaultRows, defaultClms);
  setRandom(defaultRP);
  paused = true;
  playTime = 0.0;
  maxPlayTime = 5 * 60.0; // maximum seconds per game
  lineCount = 0;
  score = 0;

}


void TApp::setLevel(unsigned int lvl) {
  assert(1 <= lvl);
  assert(lvl <= 10);
  level = lvl;

  auto tui = TetrisUI::theUI;
  if (nullptr != tui) {
    tui->counterLevel->value(lvl);
    tui->playW->redraw();
  }
  return;
}

void TApp::setRandom(bool rp) {
  randomPlacement = rp;

  auto tui = TetrisUI::theUI;
  if (nullptr != tui) {
    //tui->r
    tui->playW->redraw();
  }
  return;
}

void TApp::setRC(unsigned int r, unsigned int c) {
  rows = r;
  clms = c;
  board = new Board(rows, clms);

  auto tui = TetrisUI::theUI;
  if (nullptr != tui) {
    tui->counterRows->value(rows);
    tui->counterClms->value(clms);
    board->add(tui->playW); // connect to it
    tui->playW->redraw();
  }
  return;
}

double TApp::setDt() {
  /// set the time between updates, given the current level
  const double tSlow = 15.0;  // seconds to fall at slowest level,  1
  const double tFast = 1.0;  // seconds to fall at fastest level, 10
  // max level is fixed at 10

  // This is the basic equation:
  //   tFall = rows * dt
  //         = rows * s0 * (1/a) * a^level
  //
  // At levels 1 and 10, we have the following:
  //   tSlow = rows * s0 * (1/a) * a^1
  //   tFast = rows * s0 * (1/a) * a^10

  // Mirabile dictu, the 'a' cancel in the first equation:
  //    s0 = tSlow / rows.
  double s0 = tSlow / ((double)(board->rows));

  // If you take the ratio of the equations, we see that
  // we need to get the speedup in 9 steps:
  //    tFast/tSlow = a^9
  const double a = exp(log(tFast / tSlow) / 9.0);
  const double acc = exp(level*log(a));

  dt = s0 * (1.0 / a) * acc;
  return dt;
}

void TApp::applyControlState(ControlState cs) {

  auto tw = TetrisUI::theUI;
  if (nullptr != tw) {

    applyColorScheme(cs);

    switch (cs.gt) { // set the game time
    case 0: // 1 minute
      maxPlayTime = 60.0;
      break;

    case 2: // 10 minutes
      maxPlayTime = 10 * 60.0;
      break;

    case 1: // 5 minutes
    default:
      maxPlayTime = 5 * 60.0;
      break;
    }

    switch (cs.rt) { // set randomization or not
    case 1:
      randomPlacement = true;
      tw->controls->randomToggle->value(1);
      break;

    case 0:
    default:
      randomPlacement = false;
      tw->controls->randomToggle->value(0);
      break;
    }



  }
  // nothing yet
  return;
}

void TApp::run() {
  auto tw = new TetrisUI(defaultRows, defaultClms);
  setLevel(defaultLevel);
  setRC(defaultRows, defaultClms);
  tw->mainW->show();
  applyControlState(ControlState()); // use factory defaults
  Fl::run();
  return;
}

TApp::~TApp() {
  pause();
  delete rng;
  rng = nullptr;
  theApp = nullptr;
}

void TApp::stepGame() {


  // TApp::theApp->board->rotateDown();

  // this does stuff separate from handling key presses:
  // (*) try to soft-drop the current piece one row.
  // If that does not succeed, move the current piece to 'frags'
  // and start a new one.
  // (*) Clear lines
  //

  int clc = 0;
  if (!paused) {
    clc = board->stepGame();
  }
  lineCount = lineCount + clc;
  score = score + scoreFn(clc);

  char* buff = nullptr;

  buff = KBase::newChars(20);
  sprintf(buff, "%u", lineCount);
  TetrisUI::theUI->outputLines->value(std::string(buff).c_str());
  delete buff;


  buff = KBase::newChars(20);
  sprintf(buff, "%u", score);
  TetrisUI::theUI->outputScore->value(std::string(buff).c_str());
  delete buff;
  buff = nullptr;

  //if (0 < clc) {
  //    cout << "Lines cleared: " << clc << endl;
  //    cout << "New line count: " << lineCount << endl;
  //    cout << "New score: " << score << endl << flush;
  //}
  return;
}

void TApp::newGame() {
  setRC(rows, clms);
  playTime = 0.0;
  lineCount = 0;
  score = 0;
  return;
}

unsigned int TApp::scoreFn(unsigned int clc) {
  unsigned int s = 0;
  switch (clc) {
  case 0:
    break;
  case 1:
    s = 100;
    break;
  case 2:
    s = 250;
    break;
  case 3:
    s = 475;
    break;
  case 4:
    s = 800;
    break;
  default: // in case I ever implement 'gravity' and chain reaction clearing
    s = clc * 250;
  }
  return s;
}


void TApp::pause() {
  Fl::remove_timeout(tetrisTimer, nullptr);
  paused = true;
  return;
}

void TApp::resume(double delay) {
  assert(0.0 < delay);
  paused = false;
  Fl::add_timeout(delay, tetrisTimer);
  return;
}

Fl_Color TApp::color(unsigned int i) const {
  assert(i < colors.size());
  return colors[i];
}

void TApp::applyColorScheme(ControlState cs) {
  auto tw = TetrisUI::theUI;
  assert(nullptr != tw); // should check before getting here

  // define my own little color palette
  const Fl_Color amber = fl_rgb_color(0xFF, 0xC0, 0x00);
  const Fl_Color blue = fl_rgb_color(0x00, 0x00, 0xFF);
  const Fl_Color brown = fl_rgb_color(0xAA, 0x55, 0x00);
  const Fl_Color cyan = fl_rgb_color(0x00, 0xFF, 0xFF);
  const Fl_Color darkBeige = fl_rgb_color(0xFF, 0xFF, 0xBF);
  const Fl_Color darkGreen = fl_rgb_color(0x00, 0xAA, 0x00);
  const Fl_Color lightBeige = fl_rgb_color(0xFF, 0xFF, 0xEE);
  const Fl_Color lightGrey = fl_rgb_color(0xCC, 0xCC, 0xCC);
  const Fl_Color lime = fl_rgb_color(0x80, 0xFF, 0x00);
  const Fl_Color magenta = fl_rgb_color(0xFF, 0x00, 0xFF);
  const Fl_Color maroon = fl_rgb_color(0xAA, 0x00, 0x00);
  const Fl_Color navyBlue = fl_rgb_color(0x00, 0x00, 0xAA);
  const Fl_Color offBlack = fl_rgb_color(0x10, 0x10, 0x10);
  const Fl_Color offWhite = fl_rgb_color(0xFC, 0xFC, 0xFC);
  const Fl_Color olive = fl_rgb_color(0x80, 0x80, 0x00);
  const Fl_Color orange = fl_rgb_color(0xFF, 0xA5, 0x00);
  const Fl_Color pureBlack = FL_BLACK;
  const Fl_Color pureWhite = FL_WHITE;
  const Fl_Color purple = fl_rgb_color(0xAA, 0x00, 0xAA);
  const Fl_Color red = fl_rgb_color(0xFF, 0x00, 0x00);
  const Fl_Color teal = fl_rgb_color(0x00, 0xAA, 0xAA);
  const Fl_Color yellow = fl_rgb_color(0xFF, 0xFF, 0x00);

  // The following 11 colors must be set:
  // no shape at all: 0
  // the seven pieces: 1 to 7
  // main background: 8
  // even-numbered columns: 9
  // odd-numbered columns: 10
  colors = vector<Fl_Color>();
  for (unsigned int i = 0; i < 11; i++) {
    colors.push_back(pureWhite);
  }


  switch (cs.bg) { // color the background of the play areas
  case 0: // beige
    colors[9] = lightBeige;
    colors[10] = darkBeige;
    break;

  case 2: // white
    colors[9] = pureWhite;
    colors[10] = offWhite;
    break;

  case 1: // black
  default:
    colors[9] = pureBlack;
    colors[10] = offBlack;
    break;
  }

  switch (cs.pc)  { // color the play pieces
  // the color schemes are roughly inspired by those in
  // https://en.wikipedia.org/wiki/Tetris
  case 0:  // Game Boy
    colors[1] = orange; // I
    colors[2] = cyan; // J
    colors[3] = red; // L
    colors[4] = yellow; // O
    colors[5] = magenta; // S
    colors[6] = lime; // T
    colors[7] = amber; // Z
    break;

  case 1: // Gerasimov's Tetris 3.12
    colors[1] = maroon; // I
    colors[2] = lightGrey; // J
    colors[3] = purple; // L
    colors[4] = navyBlue; // O
    colors[5] = darkGreen; // S
    colors[6] = brown; // T
    colors[7] = teal; // Z
    break;

  case 3: // The Soviet Mind Game
    colors[1] = red; // I
    colors[2] = orange; // J
    colors[3] = magenta; // L
    colors[4] = blue; // O
    colors[5] = lime; // S
    colors[6] = olive; // T
    colors[7] = cyan; // Z
    break;

  case 4: // Tetris Company
    colors[1] = cyan; // I
    colors[2] = blue; // J
    colors[3] = orange; // L
    colors[4] = yellow; // O
    colors[5] = lime; // S
    colors[6] = purple; // T
    colors[7] = red; // Z

    break;

  case 2: // Sega
  default:
    colors[1] = red; // I
    colors[2] = blue; // J
    colors[3] = orange; // L
    colors[4] = yellow; // O
    colors[5] = magenta; // S
    colors[6] = cyan; // T
    colors[7] = lime; // Z
    break;
  }


  // color the main widgets
  tw->mainW->color(darkBeige);
  tw->about->aw->color(darkBeige);
  tw->notice->nw->color(darkBeige);
  tw->controls->mw->color(darkBeige);
  tw->mainW->redraw();

  tw->previewW->color(colors[9]);
  tw->previewW->redraw();

  tw->playW->redraw();

  return;
}


void TApp::processKey(int x, int y, int k) {
  //    printf( "TApp::processKey %i  %i   %i ", x, y, k);
  //    cout << endl << flush;

  switch (k) {

  case 49:    // 1
  case 65457: // num-1
  case 65367: // num-1
  case 55:    // 7
  case 65463: // num-7
  case 65360: // num-7
  case 122:   // z
    if (!paused) {
      //printf("Try CCW/LRot on current piece\n");
      board->tryLRot();
    }
    break;

  case 50:    // 2
  case 65458: // num-2
  case 65364: // down arrow
    if (!paused) {
      // printf("Try SDrop on current piece \n");
      board->trySDrop();
    }
    break;

  case 51:    // 3
  case 65459: // num-3
  case 65366: // num-3
  case 57:    // 9
  case 65365: // num-9
  case 65465: // num-9
  case 120:   // x
    if (!paused) {
      // printf("Try CW/RRot on current piece\n");
      board->tryRRot();
    }
    break;

  case 52:    // 4
  case 65460: // num-4
  case 65361: // left arrow
    if (!paused) {
      // printf("Try LMov on current piece\n");
      board->tryLMove();
    }
    break;

  case 110:   // 'n' for new game
    // printf("New game \n");
    newGame();
    break;

  case 114:   // 'r' for resume with delay
    // printf("Resume \n");
    if (paused) {
      resume(1.0); // resume with 1 second delay
      // TetrisUI::theUI->btnPauseResume->value(0); // button up
    }
    break;

  case 53:    // 5, p/r toggle
  case 65461: // num-5, p/r toggle
  case 65291: // num-5, p/r toggle
    if (paused) {
      resume(0.001);
    }
    else {
      pause();
    }
    break;

  case 112:   // 'p' for pause
    pause();
    break;

  case 113:   // 'q' for quit
    quit();
    break;

  case 54:    // 6
  case 65462: // num-6
  case 65363: // right arrow
    if (!paused) {
      //printf("Try RMov on current piece\n");
      board->tryRMove();
    }
    break;

  case 32:    // space
  case 56:    // 8
  case 65464: // num-8
  case 65362: // up arrow
    if (!paused) {
      //printf("Try HDrop on current piece \n");
      board->tryHDrop();
    }
    break;



  default:
    break;
  }
  //board->clearLines();
  board->Picture::update(); // crucial to allow fast spinning
  return;
}


void TApp::quit() {
  auto tw = TetrisUI::theUI;
  if (nullptr != tw) {
    tw->mainW->hide();
    tw->about->aw->hide();
    tw->controls->mw->hide();
    tw->notice->nw->hide();
  }
  return;
}




void demoCoords(PRNG* rng) {
  using KGraph::CoordMap;

  const unsigned int iterLim = 500 * 1000;
  cout << "Testing CoordMap "<< iterLim <<" times ... " << flush;

  auto make = [rng] () {
    int s1 = ((int) (rng->uniform(-1000, +1000)));
    int s2 = s1;
    while (s1 == s2) {
      s2 = ((int) (rng->uniform(-1000, +1000)));
    }

    double d1 = rng->uniform(-1000, +1000);
    double d2 = rng->uniform(-1000, +1000);

    auto cm1 = new CoordMap(s1, d1, s2, d2);
    // this tests the end points internally.

    // this must be exact, s->d does not lose information.
    int s3A = ((int) (rng->uniform(s1, s2)));
    double d3 = cm1->s2d(s3A);
    int s3B = cm1->d2s(d3);
    assert (s3A == s3B);

    // Because we lose information going from d->s,
    // we have to show that we do not lose too much.
    double d4A = rng->uniform(d1, d2);
    int s4A = cm1->d2s(d4A);
    double d4B = cm1->s2d(s4A);
    // now d4A and db4 are not necessarily the same, as everything within 1 pixel rounds to the center.
    // But they will be closer to each other than to the center of the adjoining pixels.

    // check the left pixel
    int s5 = s4A-1;
    double d5 = cm1->s2d(s5);
    assert (fabs(d4A-d4B) < fabs(d4A - d5));

    // check the right pixel
    int s6 = s4A+1;
    double d6 = cm1->s2d(s6);
    assert (fabs(d4A-d4B) < fabs(d4A - d6));
    delete cm1;
    return;
  };

  for (unsigned int i=0; i<iterLim; i++) {
    make();
  }

  cout << "done"<<endl<<flush;
  return;
}



}; // end of namespace


//---------------------------------------------
using Tetris::TApp;

TApp* TApp::theApp = nullptr;
TetrisUI* TetrisUI::theUI = nullptr;

int main(int ac, char ** av) {
  using std::cout;
  using std::endl;
  using std::string;
  using KBase::PRNG;

  auto sTime = KBase::displayProgramStart();
  const uint64_t dSeed = 0; // default is to be random and irrepeatable
  uint64_t seed = dSeed;
  bool testP = false;
  bool run = true;
  auto showHelp = [dSeed]() {
    printf("\n");
    printf("Usage: specify one or more of these options\n");
    printf("--help            print this message\n");
    printf("--test            test some basic utilities\n");
    printf("--seed <n>        set a 64bit seed\n");
    printf("                  0 means truly random\n");
    printf("                  default: %020llu \n", dSeed);
  };
  if (ac > 1) {
    for (int i = 1; i < ac; i++) {
      if (strcmp(av[i], "--seed") == 0) {
        i++;
        seed = std::stoull(av[i]);
      }
      else if (strcmp(av[i], "--test") == 0) {
        testP = true;
      }
      else if (strcmp(av[i], "--help") == 0) {
        run = false;
      }
      else {
        run = false;
        printf("Unrecognized argument %s\n", av[i]);
      }
    }
  }

  if (!run) {
    showHelp();
    return 0;
  }

  PRNG * rng = new PRNG();
  seed = rng->setSeed(seed); // 0 == get a random number
  printf("Using PRNG seed:  %020llu \n", seed);
  printf("Same seed in hex:   0x%016llX \n", seed);

  if (testP) {
    Tetris::demoCoords(rng);
  }
  
  auto t = new Tetris::TApp(seed);
  t->run();

  delete t;
  KBase::displayProgramEnd(sTime);
  return 0;
}


// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
