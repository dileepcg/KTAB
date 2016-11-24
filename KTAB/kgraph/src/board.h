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
#ifndef TETRIS_BOARD_H
#define TETRIS_BOARD_H


#include "tutils.h"
#include "shape.h"

//---------------------------------------------

using std::cout;
using std::endl;
using std::flush;

using KBase::PRNG;
using KGraph::Canvas;

//---------------------------------------------
namespace Tetris {
using KGraph::Picture;

class Board : public Picture {
  // Board manages two things: falling pieces, and left over squares.
  // The 'frags' vector contains leftover fragments of old shapes that stopped falling.
  // It has rows*clms elements, and each cell is separately labeled with a kind of Piece.
  // That cell, and only that cell, is marked with the color of the Piece.
  // The falling Shape is not represented in 'frags' at all. The falling
  // shape just checks 'frags' to find limits on its movement. When it cannot fall
  // anymore, its four cells are copied into 'frags'.
  //
  // Thus, the 'frags' array is used in the following ways:
  // (*) Check limits on L/R motion
  // (*) Check limits on CW/CCW rotation
  // (*) Check limit on falling
  // (*) Add stopped Shape
  // (*) Detect filled line (at any level)
  // (*) Remove filled line (at any level)
public:
  Board(unsigned int r, unsigned int c);
  virtual ~Board();

  void randomizeFragments(double f); // fill board with this fraction of random fragments
  void rotateDown();

  //  virtual void connect(Canvas * c); //  initial configuration of the canvas
  virtual void update(Canvas * c) const; //  set the state of the canvas to whatever is needed

  // row 0 is the top, clm 0 is the left
  unsigned int rows = 0;
  unsigned int clms = 0;

  unsigned int nFromIJ(int i, int j) const;

  bool resetCurrPiece(); // true if success, false if failed (game over)
  unsigned int clearLines();
  unsigned int stepGame();

  bool tryLRot();
  bool tryRRot();

  bool tryLMove();
  bool tryRMove();

  bool testSDrop();
  bool trySDrop();
  bool tryHDrop();

  void drawShape(int i, int j, Canvas * cnvs) const;
  void drawUnitSquare(Fl_Color clr1, int i, int j, bool dotP, Fl_Color clr2, Canvas * cnvs) const;

  // current shape and location,
  // as well as the next shape.
  Shape currShape = Shape();
  int currI = 0;
  int currJ = 0;
  Shape nextShape = Shape();
protected:
  void drawBackground(Canvas * c) const;
  void drawCurrShape(Canvas * c) const;
  void drawFragments(Canvas * c) const;

  bool testShape(Shape s, int i, int j) const; // true IFF the piece can legally be placed there

  void randomizeRow(unsigned int i);
  vector<TCode> emptyBoard() const; // returns rows-by-clms array with N at each cell
  void placeShape(Shape s, int i, int j);

  bool clearOneLine(const unsigned int i);


private:
  vector<TCode> frags = {}; // fragments of old shapes
};

}; // end of namespace

//---------------------------------------------
#endif
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
