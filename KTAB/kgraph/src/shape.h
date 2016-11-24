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
#ifndef TETRIS_SHAPE_H
#define TETRIS_SHAPE_H
//---------------------------------------------

namespace Tetris {

enum TCode { // Character codes for the canonical Tetronimoes
  N = 0, I, J, L, O, S, T, Z
};

class Shape { // any valid transformation of a canonical Tetronimo
public:
  static unsigned int shapeCounter;
  Shape() { idNum = shapeCounter++; setShape(TCode::N); }
  explicit Shape(TCode p) { idNum = shapeCounter++; setShape(p); }
  void setShape(TCode p);
  void setRandomShape();
  TCode getShape() const { return tName; }
  char getName() const { return name;}
  int x(int index) const { assert(0 <= index); assert(index <= 3); return coords[index][0]; }
  int y(int index) const { assert(0 <= index); assert(index <= 3); return coords[index][1]; }

  int minX() const;
  int maxX() const;
  int minY() const;
  int maxY() const;

  void showCoords() const;

  Shape lrot() const;
  Shape rrot() const;

  unsigned int idNum = 0;

private:
  void setX(int index, int x) { coords[index][0] = x; }
  void setY(int index, int y) { coords[index][1] = y; }
  TCode tName = N;
  char name = 'N';
  int coords[4][2];
};

}; // end of namespace

//---------------------------------------------
#endif
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
