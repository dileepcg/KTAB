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
#ifndef TETRIS_CANVAS_H
#define TETRIS_CANVAS_H

#include "kutils.h"
#include "kgraph.h"

//---------------------------------------------

using std::cout;
using std::endl;
using std::flush;

using KGraph::Canvas;

//---------------------------------------------
namespace Tetris {


class TCanvas : public Canvas {
public:
  TCanvas(int x, int y, int w, int h, const char * l = 0);
  virtual ~TCanvas();

  void onMove(int x, int y);
  void onDrag(int x, int y);
  void onPush(int x, int y, int b);
  void onRelease(int x, int y, int b);
  void onKeyDown(int x, int y, int k);

protected:
  virtual void draw();
  // again, user should never call 'draw' directly.
  // just mark damage and/or call 'redraw'
  
private:
};

}; // end of namespace

//---------------------------------------------
#endif
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
