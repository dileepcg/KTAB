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
#include "kgraph.h"
#include "prng.h"
#include "tmain.h"
#include "board.h"
#include "pvcanvas.h"
#include "tetrisUI.h"


namespace Tetris {
using KGraph::Picture;

Board::Board(unsigned int r, unsigned int c) : Picture() {
  assert(4 < c);    // not too narrow
  rows = r;
  clms = c;
  // the first column occupies the interval [0.0, 1.0] in the domain
  minX = 0; // left of screen
  maxX = c; // right of screen
  // the first row occupies the interval [0.0, 1.0] in the domain
  minY = 0; // BOTTOM of screen
  maxY = r; // TOP of screen
  frags = emptyBoard();
  currShape = Shape(N);
  nextShape = Shape(N);
  resetCurrPiece();
}


Board::~Board() {
  // nothing, yet
}

void Board::randomizeFragments(double f) {
  frags = emptyBoard();
  for (unsigned int i = 0; i < rows; i++) {
    for (unsigned int j = 0; j < clms; j++) {
      double x = TApp::theApp->rng->uniform(0.0, 1.0);
      TCode tij = N;
      if (x < f) {
        unsigned int tn = ((unsigned int) (1 + (TApp::theApp->rng->uniform() % 7)));
        int n = nFromIJ(i,j);
        frags[n] = TCode(tn);
      }
    }
  }
  return;
}

// For now, this just directly puts them into 'frags'
void Board::randomizeRow(unsigned int i) {
  unsigned int maxInt = 8; // the higher, the more blank spaces
  //maxInt = 400; // good to keep whole pieces separate
  //    maxInt = 8;  // good for individual squares
  PRNG* rng = Tetris::TApp::theApp->rng;
  int j = rng->uniform() % clms;
  auto k = nFromIJ(i, j);
  unsigned int pi = rng->uniform() % maxInt;
  TCode p = ((1 <= pi) && (pi <= 7)) ? ((TCode)pi) : N;
  if (N != p) {
    Shape s = Shape(p);
    s.setRandomShape();
    bool ok = testShape(s, i, j);
    if (ok) {
      placeShape(s,i,j);
      s.showCoords();
      cout << endl << flush;
    }
  }
  return;
}


vector<TCode> Board::emptyBoard() const {
  auto b2 = vector<TCode>();
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < clms; j++) {
      b2.push_back(N); // each N just occupies one cell
    }
  }
  return b2;
}


void Board::placeShape(Shape s, int i, int j) {
  const TCode p = s.getShape();
  for (unsigned int k=0; k<4; k++) {
    const int i2 = i + s.y(k);
    const int j2 = j + s.x(k);
    const int n = nFromIJ(i2, j2);
    frags[n] = p;
  }
  return;
}

unsigned int Board::stepGame() {
  trySDrop(); // if possible, do it
  bool ok2 = testSDrop(); // test if more dropping is possible
  unsigned int clc = 0;
  bool spaceInBox = true;
  if (!ok2) { // Current shape reached bottom
    placeShape(currShape, currI, currJ);
    currShape = Shape(N);
    spaceInBox = resetCurrPiece();
  }
  if ((!spaceInBox) || (TetrisUI::theUI->timeProgress->value() >= 1.0)) {
    TApp::theApp->pause(); // as the game is over
    TetrisUI::theUI->notice->nw->show();
  }
  else {
    clc = clearLines();
  }
  return clc;
}

void Board::rotateDown() {
  auto dRot = [this](int i1) {
    int i2 = ((i1 + rows) - 1) % rows;
    return i2;
  };
  auto b2 = emptyBoard();
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < clms; j++) {
      auto n1 = nFromIJ(i, j);
      auto n2 = nFromIJ(dRot(i), j);
      b2[n2] = frags[n1];
    }
  }
  frags = b2;
  for (unsigned int j=0; j<clms; j++) {
    auto n = nFromIJ(rows-1, j);
    frags[n] = N;
  }
  randomizeRow(rows - 1);// randomize top
  return;
}


bool Board::tryLRot() {
  bool ok = (O == currShape.getShape()); // TCode "oh", not zero or char "oh"
  if (!ok) {
    auto s2 = currShape.lrot();
    ok = testShape(s2, currI, currJ);
    if (ok) {
      currShape = s2;
    }
  }
  return ok;
}
bool Board::tryRRot() {
  bool ok = (O == currShape.getShape()); // TCode "oh", not zero or char "oh"
  if (!ok) {
    auto s2 = currShape.rrot();
    ok = testShape(s2, currI, currJ);
    if (ok) {
      currShape = s2;
    }
  }
  return ok;
}

bool Board::tryLMove() {
  bool ok = testShape(currShape, currI, currJ-1);
  if (ok) {
    currJ = currJ-1;
  }
  return ok;
}
bool Board::tryRMove() {
  bool ok = testShape(currShape, currI, currJ+1);
  if (ok) {
    currJ = currJ+1;
  }
  return ok;
}


bool Board::testSDrop() {
  bool ok = testShape(currShape, currI-1, currJ);
  return ok;
}

bool Board::trySDrop() {
  bool ok = testSDrop();
  if (ok) {
    currI = currI-1;
  }
  return ok;
}
bool Board::tryHDrop() {
  bool ok = true;
  while (ok) {
    ok = trySDrop();
  }
  return ok;
}

bool Board::resetCurrPiece() {
  while (N == currShape.getShape()) {
    // starting location for next shape is top middle
    currShape = nextShape;
    currI = rows-1;
    currJ = clms/2;
    if (TApp::theApp->randomPlacement) {
      currJ = 2 + (TApp::theApp->rng->uniform() % (clms-3));
      // allow two spaces on left to I and one on the right for everything
    }

    nextShape.setRandomShape(); // get a new shape

    if (nextShape.getShape() == currShape.getShape()) {
      // get a new, probably different piece
      nextShape.setRandomShape();
    }
  }
  // at this point, we have two shapes to draw.

  if (nullptr != TetrisUI::theUI){
    TetrisUI::theUI->previewW->redraw();
  }

  bool ok = testShape(currShape, currI, currJ);
  return ok;
}

unsigned int Board::clearLines() {
  unsigned int count = 0;
  unsigned int i = 0;
  while (i < rows) {
    bool cleared = clearOneLine(i);
    if (cleared) {
      count++;
      // do not increment i, because it might have
      // filled up with fragments from above.
    }
    else {
      i++;
    }
  }
  return count;
}

bool Board::clearOneLine(const unsigned int i) {
  bool cleared = false;
  bool solidP = true;

  // see if this row is fully occupied
  for (unsigned int j = 0; j< clms; j++) {
    unsigned int n = nFromIJ(i,j);
    solidP = solidP && (N != frags[n]);
  }

  if (solidP) {
    // slide each row down, overwriting the lower
    for (unsigned int i2 = i+1; i2<rows; i2++) {
      for (unsigned int j = 0; j< clms; j++) {
        unsigned int nLow = nFromIJ(i2-1, j);
        unsigned int nHigh = nFromIJ(i2,j);
        frags[nLow] = frags[nHigh];
      }
    }

    // fill top row with N
    for (unsigned int j = 0; j< clms; j++) {
      unsigned int n = nFromIJ(rows-1,j);
      frags[n] = N;
    }
    cleared = true;
  }

  return cleared;
}

void Board::update(Canvas * c) const {
  if (nullptr != c) {
    c->updateMaps();
    drawBackground(c);
    drawCurrShape(c);
    drawFragments(c);

    if (false) { // draw test pattern?
      // Red on the left, green on the right.
      // Black on the top, white on the bottom.
      drawUnitSquare(FL_RED, 0, 0, true, FL_WHITE, c);
      drawUnitSquare(FL_GREEN, 0, clms - 1, true, FL_WHITE, c);
      drawUnitSquare(FL_RED, rows - 1, 0, true, FL_BLACK, c);
      drawUnitSquare(FL_GREEN, rows - 1, clms - 1, true, FL_BLACK, c);
    }
  }
  return;
}


void Board::drawBackground(Canvas * c) const {
  // for now, we cheat and just draw in screen coordinates
  const int cx = c->x();
  const int cy = c->y();
  const int cw = c->w();
  const int ch = c->h();

  auto bgEven = TApp::theApp->color(9);
  auto bgOdd = TApp::theApp->color(10);

  fl_push_clip(cx, cy, cw, ch);
  for (unsigned int j = 0; j < clms; j++) {
    unsigned int x1 = cx + (j*cw) / clms;
    unsigned int x2 = cx + ((j + 1)*cw) / clms;
    auto bg = (0 == (j % 2)) ? bgEven : bgOdd;
    fl_color(bg);
    fl_rectf(x1, cy, x2 - x1, ch);
  }
  fl_pop_clip();
  return;
}

/*
   The row index i is zero at the bottom.
   The clm index j is zero at the left.
   (1,0)  (1,1) (1,2)
   (0,0)  (0,1) (0,2)
   They are counted off in this order:
   3      4     5
   0      1     2
   */
unsigned int Board::nFromIJ(int i, int j) const {
  assert (0 <= i);
  assert (i < rows);
  assert (0 <= j);
  assert (j < clms);
  unsigned int n = i*clms + j;
  return n;
}

void Board::drawFragments(Canvas * c) const {
  auto pij = [this](unsigned int i, unsigned int j) {
    return frags[nFromIJ(i, j)];
  };
  auto cij = [pij](unsigned int i, unsigned int j) {
    TCode p = pij(i, j);
    auto clr = TApp::theApp->color(p);
    return clr;
  };
  const int cx = c->x();
  const int cy = c->y();
  const int cw = c->w();
  const int ch = c->h();
  fl_push_clip(cx, cy, cw, ch);

  for (unsigned int i = 0; i < rows; i++) {
    for (unsigned int j = 0; j < clms; j++) {
      auto p = pij(i, j);
      if (N != p) {
        drawUnitSquare(cij(i, j), i, j, false, FL_BLACK, c);
      }
    }
  }

  fl_pop_clip();
  return;
}

void Board::drawCurrShape(Canvas * cnvs) const {
  unsigned int i = currI;
  unsigned int j = currJ;
  unsigned int p = currShape.getShape();
  const Fl_Color clr = TApp::theApp->color((unsigned int)p);
  for (int k = 0; k < 4; k++) {
    drawUnitSquare(clr,
                   i + currShape.y(k), j + currShape.x(k),
                   false, FL_WHITE, cnvs);
  }
  //drawUnitSquare(clr, i, j, true, FL_BLACK, cnvs);

  return;
}

void Board::drawShape(int i, int j, Canvas * cnvs) const {
  TCode p = frags[nFromIJ(i, j)];
  if (N != p) {
    auto s = Shape(p);
    const Fl_Color clr = TApp::theApp->color((unsigned int)p);
    for (int k = 0; k < 4; k++) {
      drawUnitSquare(clr,
                     i + s.y(k), j + s.x(k),
                     false, FL_WHITE, cnvs);
    }
    drawUnitSquare(clr,
                   i, j, true, FL_BLACK, cnvs);
  }
  return;
}

void Board::drawUnitSquare(Fl_Color clr1, int i, int j, bool dotP, Fl_Color clr2, Canvas * cnvs) const {
  // i is the row, so it controls y
  // j is the clm, so it controls x

  // the edges of the box in column zero are at domain coords 0.0 and 1.0
  assert(nullptr != cnvs->xMap);
  int x1 = cnvs->xMap->d2s(j);
  int x2 = cnvs->xMap->d2s(j + 1);
  const int stripeW = x2 - x1;
  assert(0 < stripeW);

  // the edges of the box in row zero are at domain coords 0.0 and 1.
  // The screen coords are small at the top, and large at the bottom.
  assert(nullptr != cnvs->yMap);
  const int y1 = cnvs->yMap->d2s(i);
  const int y2 = cnvs->yMap->d2s(i + 1);
  const int stripeH = y1 - y2;
  assert(0 < stripeH);

  // Suppose each vertical stripe is 20 pixels wide.
  // To leave one pixel empty on each side, and a 2-pixel stripe
  // between two squares, we offset by 1/20 and make the square 18/20
  // of the stripe-width.
  const int sqrW = (18 * stripeW) / 20;
  const int sqrH = (18 * stripeH) / 20;

  //printf("(%i,%i) -> (%i, %i, %i, %i) \n", i, j, x1, y1, sqrW, sqrH);

  fl_color(clr1);
  fl_rectf(x1, y1 - sqrH, sqrW, sqrH);
  if (dotP) {
    fl_color(clr2);
    fl_rectf(x1 + sqrW / 4, y1 - sqrH + (sqrH / 4), sqrW / 2, sqrH / 2);
  }
  return;
}

bool Board::testShape(Shape s, int i, int j) const {
  // the vector 'frags' contains only fragments from other
  // shapes, so we can easily check for collision
  bool validX = ((0 <= j+s.minX()) && (j+s.maxX()<clms));
  bool validY = ((0 <= i+s.minY()) && (i+s.maxY()<rows));

  bool ok = validX && validY;
  if (ok) {
    for (unsigned int k=0; k<4; k++) {
      // the vector 'frags' contains only fragments from other
      // shapes, so we can easily check for collision
      const int i2 = i + s.y(k);
      const int j2 = j + s.x(k);

      auto pk = frags[ nFromIJ(i2, j2) ];
      ok = ok && (N == pk);
    }
  }

  return ok;
}


}; // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
