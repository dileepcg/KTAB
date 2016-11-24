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
#include "tcanvas.h"
#include "tmain.h"
#include "tetrisUI.h"


namespace Tetris {

TCanvas::TCanvas(int x, int y, int w, int h, const char * l) : Canvas(x, y, w, h, l) {
  // nothing, yet
}


TCanvas::~TCanvas() {
  // nothing, yet
}

void TCanvas::onMove(int x, int y) {
  auto tui = TetrisUI::theUI;
  if (nullptr != tui) {
    tui->playW->take_focus(); // at least try to get the keyboard events
    auto buff1 = KBase::newChars(20); //newChar(20);
    sprintf(buff1, "%3i, %3i", x, y);
    std::string buff2 = buff1;
    tui->textCoordXY->value(buff2.c_str());
    tui->playW->redraw();
    delete buff1;
    buff1 = nullptr;
  }
  else {
    // printf("TCanvas::onMove at %i, %i \n", x, y);
  }

  return;
}
void TCanvas::onDrag(int x, int y) {
  //    printf("TCanvas::onDrag at %i, %i \n", x, y);
  return;
}
void TCanvas::onPush(int x, int y, int b) {
  //printf("TCanvas::onPush %i at %i, %i \n", b, x, y);
  auto tui = TetrisUI::theUI;
  if (nullptr != tui) {
    tui->playW->take_focus(); // at least try to get the keyboard events
  }
  return;
}
void TCanvas::onRelease(int x, int y, int b) {
  //    printf("TCanvas::onRelease %i at %i, %i \n", b, x, y);
}
void TCanvas::onKeyDown(int x, int y, int k) {
  //printf("TCanvas::onKeyDown %i at %i, %i \n", k, x, y);
  Tetris::TApp::theApp->processKey(x, y, k);
}
void TCanvas::draw() {
  Canvas::draw();
  if (nullptr != pict) {
    pict->update(this);
  }
  return;
}

}; // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
