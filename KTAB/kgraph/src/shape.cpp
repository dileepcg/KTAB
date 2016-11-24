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

#include "kutils.h"
//#include <algorithm> // get min, max
//#include "tutils.h"
#include "shape.h"
#include "tmain.h"

//---------------------------------------------

namespace Tetris {
unsigned int Shape::shapeCounter = 0;

void Shape::setShape(TCode p) {
  static const int coordsTable[8][4][2] = {
    { { 0, 0 }, {  0,  0 }, { 0, 0 }, { 0, 0 } }, // N, occupying just one cell
    { { 0, 0 }, { -1,  0 }, {-2, 0},  {+1, 0 } }, // I
    { { 0, 0 }, { -1,  0 }, { 1, 0 }, { 1,-1 } }, // J
    { { 0, 0 }, { -1,  0 }, { 1, 0 }, {-1,-1 } }, // L
    { { 0, 0 }, { -1,  0 }, { 0,-1 }, {-1,-1 } }, // O
    { { 0, 0 }, {  0, -1 }, { 1, 0 }, {-1,-1 } }, // S
    { { 0, 0 }, { -1,  0 }, { 1, 0 }, { 0,-1 } }, // T
    { { 0, 0 }, {  0, -1 }, {-1, 0 }, { 1,-1 } }  // Z
  };


  const char * nameTable = "NIJLOSTZ";

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 2; ++j)
      coords[i][j] = coordsTable[p][i][j];
  }
  tName = p;
  name = nameTable[p];
}


void Shape::setRandomShape() {
  auto ta = TApp::theApp;
  assert(nullptr != ta);
  unsigned int i = 1 + (ta->rng->uniform() % 7);
  setShape(TCode(i));
}

int Shape::minX() const {
  int m = coords[0][0];
  for (unsigned int i = 0; i < 4; i++) {
    m = (x(i) < m) ? x(i) : m;
  }
  return m;
}

int Shape::maxX() const {
  int m = coords[0][0];
  for (unsigned int i = 0; i < 4; i++) {
    m = (m < x(i)) ? x(i) : m;
  }
  return m;
}

int Shape::minY() const {
  int m = coords[0][1];
  for (unsigned int i = 0; i < 4; i++) {
    m = (y(i) < m) ? y(i) : m;
  }
  return m;
}

int Shape::maxY() const {
  int m = coords[0][1];
  for (unsigned int i = 0; i < 4; i++) {
    m = (m < y(i)) ? y(i) : m;
  }
  return m;
}

void Shape::showCoords() const {
  printf("[%c %05u ", name, idNum);
  for (unsigned int k = 0; k < 4; k++){
    printf("(%+i, %+i)", x(k), y(k));
    if (3 != k) { printf(" "); }
  }
  printf("]");
  return;
}

Shape Shape::lrot() const {
  Shape result;
  result.tName = tName;
  for (int i = 0; i < 4; ++i) {
    result.setX(i, -y(i));
    result.setY(i, x(i));
  }
  return result;
}

Shape Shape::rrot() const {
  Shape result;
  result.tName = tName;
  for (int i = 0; i < 4; ++i) {
    result.setX(i, y(i));
    result.setY(i, -x(i));
  }
  return result;
}

}; // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
