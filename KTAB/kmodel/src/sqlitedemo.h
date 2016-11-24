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

#ifndef DEMO_SQLITE_H
#define DEMO_SQLITE_H

#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include <string>
#include <sqlite3.h> 
#include <tuple>
#include <vector>


#include "kutils.h"
#include "prng.h"
#include "kmatrix.h"

namespace MDemo {
using std::string;
using std::tuple;
using std::vector;


void demoDBObject();

class SQLDB {
public:
  explicit SQLDB(char* filename);
  virtual ~SQLDB();
  bool open(char* filename);

  // returns a vector of rows, where each row is a vector of (string) values
  tuple<unsigned int, vector<vector<string>>> query(const char* query);

  void close();

private:
  sqlite3 *database = nullptr;
  bool dbOpen = false;
};


}; // end of namespace

// -------------------------------------------------
#endif

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
