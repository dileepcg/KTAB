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
// start of a simple set of common graphics for KTAB
//---------------------------------------------

#include <FL/Fl.H>
#include <FL/Enumerations.H> // make it easy to open and study
#include "kgraph.h"

//---------------------------------------------
namespace KGraph {


CoordMap::CoordMap(int s1, double d1, int s2, double d2) {
  ad = (d2 - d1) / ((double)(s2 - s1));
  bd = ((s2*d1) - (s1*d2)) / ((double)(s2 - s1));

  as = ((double)(s2 - s1)) / (d2 - d1);
  bs = ((s1*d2) - (s2*d1)) / (d2 - d1);

  if (testMap) {
    // quickly test the transforms
    const double dTol = fabs(d2 - d1) / 1.0E6;

    auto s2dTest = [dTol, this](double dA, int sA) {
      const double da2 = s2d(sA);
      assert(fabs(dA - da2) < dTol);
      return;
    };

    s2dTest(d1, s1);
    s2dTest(d2, s2);

    auto d2sTest = [this](int sA, double dA) {
      const int sa2 = d2s(dA);
      assert(sA == sa2);
      return;
    };

    d2sTest(s1, d1);
    d2sTest(s2, d2);
  }
}
CoordMap::~CoordMap() {
  as = 0;
  bs = 0;
  ad = 0;
  bd = 0;
}
int CoordMap::d2s(double d) {
  int s = cmRound(as*d + bs);
  return s;
}
double CoordMap::s2d(int s) {
  double d = ad*s + bd;
  return d;
}
int CoordMap::cmRound(double x) {
  int y = ((int)(x + 0.5));
  if (x < 0.0) {
    y = -cmRound(-x);
  }
  return y;
}
//---------------------------------------------
Canvas::Canvas(int x, int y, int w, int h, const char * l) : Fl_Box(x, y, w, h, l) {
  xMap = nullptr;
  yMap = nullptr;
  pict = nullptr;
}
Canvas::~Canvas() {
  clearMaps();
  pict = nullptr;
}
void Canvas::end() {
  Fl_Group::current(this->parent());
  return;
}
int Canvas::handle(int ev) {
  //printf("Canvas::handle event %i \n", ev);
  int r = 1; // return 0 if unhandled

  // Note well the following interactions.
  // You can also figure them out from Enumerations.H
  //
  // You must return non-zero for ENTER events,
  // even if you really do not handle them,
  // or else FLTK will lose track of all the MOVE events.
  // Logically, you cannot MOVE in a window unless you ENTER it first.
  //
  // You must return non-zero for PUSH events,
  // even if you really do not handle them,
  // or else FLTK will lose track of all DRAG events.
  // Logically, you cannot DRAG in a window unless you PUSH there first.
  //
  // You must return non-zero for FOCUS events,
  // or else FLTK will lose track of all KEYDOWN and KEYUP events.
  // You cannot direct keyboard input to a window unless focussed upon it.

  switch (ev) {
  case FL_SHOW:
    break;
  case FL_HIDE:
    break;
  case FL_FOCUS:
    break;
  case FL_UNFOCUS:
    break;
  case FL_ENTER:
    break;
  case FL_LEAVE:
    break;
  case FL_KEYUP:
    break;

  case FL_MOVE:
    onMove(Fl::event_x(), Fl::event_y());
    break;
  case FL_DRAG:
    onDrag(Fl::event_x(), Fl::event_y());
    break;
  case FL_PUSH:
    onPush(Fl::event_x(), Fl::event_y(), Fl::event_button());
    break;
  case FL_RELEASE:
    onRelease(Fl::event_x(), Fl::event_y(), Fl::event_button());
    break;
  case FL_KEYDOWN:
    onKeyDown(Fl::event_x(), Fl::event_y(), Fl::event_key());
    break;
  default: // not handled
    r = 0;
    break;
  }
  return r;
}
void Canvas::onMove(int x, int y) {
  printf("Canvas::onMove at %i, %i \n", x, y);
  cout << flush;
  return;
}
void Canvas::onDrag(int x, int y) {
  printf("Canvas::onDrag at %i, %i \n", x, y);
  cout << flush;
  return;
}
void Canvas::onPush(int x, int y, int b) {
  printf("Canvas::onPush %i at %i, %i \n", b, x, y);
  cout << flush;
  return;
}
void Canvas::onRelease(int x, int y, int b) {
  printf("Canvas::onRelease %i at %i, %i \n", b, x, y);
  cout << flush;
  return;
}
void Canvas::onKeyDown(int x, int y, int k) {
  printf("Canvas::onKeyDown %i at %i, %i \n", k, x, y);
  cout << flush;
}
void Canvas::clearMaps() {
  if (nullptr != xMap) {
    delete xMap;
    xMap = nullptr;
  }
  if (nullptr != yMap) {
    delete yMap;
    yMap = nullptr;
  }
  return;
}
void Canvas::updateMaps() {
  clearMaps();
  assert(pict->minX <= pict->maxX);
  assert(pict->minY <= pict->maxY);

  // left of screen is lowest domain coordinate
  int s1 = x();
  double d1 = pict->minX;
  int s2 = x() + w();
  double d2 = pict->maxX + pict->minW;
  xMap = new CoordMap(s1, d1, s2, d2);

  // BOTTOM of screen is lowest domain coordinate
  s1 = y() + h();
  d1 = pict->minY;
  s2 = y();
  d2 = pict->maxY + pict->minH;
  yMap = new CoordMap(s1, d1, s2, d2);
  return;
}
//---------------------------------------------


Picture::Picture() {
  canvases = vector<Canvas*>();

  // basic picture is a unit square
  minX = 0.0;
  maxX = 1.0;
  minW = 1E-6;
  minY = 0.0;
  maxY = 1.0;
  minH = 1E-6;
}


Picture::~Picture() {
  // nothing yet
}

void Picture::add(Canvas * c) {
  // just adds to list: you will need to connect
  assert(nullptr != c);
  canvases.push_back(c);
  connect(c);
  return;
}


void Picture::update() const {
  for (auto c : canvases) {
    update(c);
  }
  return;
}


void Picture::connect(Canvas * c) {
  c->pict = this;
  return;
}


void Picture::update(Canvas * c) const {
  // nothing, yet
  return;
}


}; // end of namespace


// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
