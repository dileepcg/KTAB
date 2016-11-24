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
// Define a few generally useful functions.
// --------------------------------------------

#include <assert.h>
#include <iostream>
#include <tuple>

#include "kutils.h"
#include "prng.h"

namespace KBase {

using std::cout;
using std::endl;
using std::flush;

// -------------------------------------------------

// for historical reasons, the second, quadratic power is called "square"
double sqr(const double& x) {
  return (x*x);
}

// the fourth power is the quartic
double qrtc(const double& x) {
  return (x*x*x*x);
}

double quadUfromV(double v, double bigR) {
  assert (-1.0 <= bigR);
  assert(bigR <= +1.0);

  // We tolerate some round-off error, but not much.
  // In SMP,  v>1 corresponds to distance < 0, which is impossible.
  // In general von Neumann rescaling, v=(X-minX)/(maxX-minX), so
  // v>1 corresponds to X > maxX, which is similarly impossible.
  const double tol = 1E-10;
  if (1 < v) {
    assert (v <= 1.0 + tol);
    v=1.0;
  }
  // because searches try silly values, we have to deal with
  // distances like 1.01, on a scale of [0,1], hence v=1-d of about -0.01

  double u = 0.0;
  if (0< v) {
    u = v + (bigR * v * (1.0-v));
    assert (0.0 <= u);
  }
  else {
    // straightline interpolation past v=0
    u = (1.0+bigR)*v;
  }
  assert (u <= 1.0);
  return u;
}

double nProd(double x, double y) {
  if ((0.0 < x) && (0.0 < y)) {
    return (x * y);
  }
  if ((x < 0.0) && (y < 0.0)) {
    return (x + y);
  }
  return ((x < y) ? x : y);
}



double trim(double x, double minX, double maxX, bool strict) {
  double x2 = x;

  if (strict) {
    assert (minX <= x);
    assert (x <= maxX);
  }

  if (x < minX) {
    x2 = minX;
  }
  if (maxX < x) {
    x2 = maxX;
  }

  return x2;
}


VUI uiSeq(const unsigned int n1, const unsigned int n2, const unsigned int ns) {
  VUI uis = {};
  assert (n1 <= n2);
  assert (0 < ns);
  for (unsigned int i = n1; i <= n2; i=i+ns)
    uis.push_back(i);
  return uis;
}

void printVUI(const VUI& p) {
  cout << "[VUI ";
  for (auto i : p) {
    printf("%2u ", i);
  }
  cout << "]";
  return;
}

// --------------------------------------------

void groupThreads(function<void(unsigned int)> tfn,
                  unsigned int numLow, unsigned int numHigh, unsigned int numPar) {
  const auto rl = ReportingLevel::Silent;
  using std::thread;
  const unsigned int threadsPerHWC = 4;
  unsigned int numHWC = 0;
  unsigned int dfltNumThreads = 10;
  if (0 == numPar) { // no specific number requested, so guess

    // As of OCt. 2016, this function might or might not have one or two
    // of the following problems, depending on your implementation.
    // 1: It might not be implemented, and just return 0.
    // 2: It might not take into account "hyperthreading", and return
    //    half the expected number.
    numHWC = std::thread::hardware_concurrency();
    if (0 == numHWC) {
      numPar = dfltNumThreads; // arbitrary default
    }
    else {
      // four times the number of threads is not too inefficient,
      // and 2 times the number of hyperthreads is not too wasteful.
      numPar = threadsPerHWC * numHWC;
    }
  }
  if (ReportingLevel::Silent < rl) {
    if (0 == numHWC) {
      cout << "Could not detect hardware concurrency" << endl;
    }
    else {
      cout << "Detected hardware concurrency: " << numHWC << endl;
    }
    cout << "Using groups of " << numPar << endl;
  }

  unsigned int cntr = numLow;
  while (cntr <= numHigh) {
    vector<thread> myThreads = {};
    for (unsigned int i = 0; ((i < numPar) && (cntr <= numHigh)); i++) {
      if (ReportingLevel::Medium < rl) {
        printf("Launching thread %3u / %3u / [%3u,%3u]", i, cntr, numLow, numHigh);
        cout << endl << flush;
      }
      myThreads.push_back(thread(tfn, cntr));
      cntr++;
    }
    if (ReportingLevel::Low < rl) {
      cout << "Joining ..." << endl;
    }
    for (auto& ti : myThreads) {
      ti.join();
    }
  }
  return;
}

// --------------------------------------------

std::chrono::time_point<std::chrono::system_clock>  displayProgramStart(string appName, string appVersion) {
  std::chrono::time_point<std::chrono::system_clock> st;
  st = std::chrono::system_clock::now();
  std::time_t start_time = std::chrono::system_clock::to_time_t(st);
  cout << "  Software version: " << appName << " " << appVersion <<endl;
  cout << "  Start time: " << std::ctime(&start_time) << endl;
  return st;
}

void displayProgramEnd(std::chrono::time_point<std::chrono::system_clock> st) {
  std::chrono::time_point<std::chrono::system_clock> ft;
  ft = std::chrono::system_clock::now();
  std::chrono::duration<double> eTime = ft - st;
  std::time_t fTime = std::chrono::system_clock::to_time_t(ft);
  cout << "  Finish time: " << std::ctime(&fTime) << endl << flush;
  printf("  Elapsed time: %.4f seconds \n", eTime.count());
  return;
}


char* newChars(unsigned int len) {
  auto rslt = new char[len];
  for (unsigned int i = 0; i < len; i++) {
    rslt[i] = ((char)0);
  }
  return rslt;
};

double rescale(double x, double x0, double x1, double y0, double y1) {
  assert((x0 < x1) || (x1 < x0));
  const double f = (x - x0) / (x1 - x0);
  return y0 + (y1 - y0)*f;
}

// -------------------------------------------------

KException::KException(string m) {
  msg = m;
}

KException::~KException() {
  msg = "";
}

/*

EnumType::EnumType(int i) {
  assert(0 <= i);
  assert(i < names->size());
  ndx = i;
}

EnumType::EnumType(const char* n) {
  // nothing yet
}

EnumType::EnumType(const string& s) {
  // nothing yet
}

EnumType::~EnumType() {
  names = nullptr;
  ndx = 0;
}


unsigned int EnumType::addName(string etn) {
  assert(nullptr != names);
  names->push_back(etn);
  return names->size();
}

string EnumType::name(const EnumType& et) {
  const unsigned int en = et.ndx;
  assert(0 <= en);
  assert(en < names->size());
  return (*names)[en];
}
*/


}; 
// end of namespace


// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
