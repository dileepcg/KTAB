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

#include <string.h>
#include "tutils.h"
#include "pvcanvas.h"
#include "tmain.h"


namespace Tetris {

PVCanvas::PVCanvas(int x, int y, int w, int h, const char * l) : Canvas(x, y, w, h, l) {
  // nothing, yet
}


PVCanvas::~PVCanvas() {
  // nothing, yet
}

void PVCanvas::onMove(int x, int y) {
  return;
}
void PVCanvas::onDrag(int x, int y) {
  return;
}
void PVCanvas::onPush(int x, int y, int b) {
  return;
}
void PVCanvas::onRelease(int x, int y, int b) {
  return;
}
void PVCanvas::onKeyDown(int x, int y, int k) {
  return;
}
void PVCanvas::draw() {
  const int px = x();
  const int py = y();
  const int pw = w();
  const int ph = h();
  clearMaps();
  xMap = new KGraph::CoordMap(px, 0.5, px+pw, 5.5);
  yMap = new KGraph::CoordMap(py + ph, 0.5, py, 3.5);
  Canvas::draw();

  fl_push_clip(px, py, pw, ph);
  Shape ns = TApp::theApp->board->nextShape;
  TCode nc = ns.getShape();
  const Fl_Color clr = TApp::theApp->color((unsigned int)nc);
  const int ci = 2;
  const int cj = 3;
  for (int k = 0; k < 4; k++) {
    TApp::theApp->board->drawUnitSquare(clr,
                                        ci + ns.y(k), cj + ns.x(k),
                                        false, FL_WHITE, this);
  }
  fl_pop_clip();
  return;
}

}; // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
