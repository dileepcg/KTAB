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

#include "tinyxml2demo.h"


// ------------------------------------------------------------------------------------

namespace TXDemo {
using std::cout;
using std::endl;
using std::flush;
using std::string;
using std::vector;
using std::tuple;

using KBase::KException;

using tinyxml2::XMLDocument;
using tinyxml2::XMLElement;

void demoTX2(string fileName) {
  XMLDocument d1;
  try {
    d1.LoadFile(fileName.c_str());
    auto eid = d1.ErrorID();
    if (0 != eid) {
      cout << "ErrorID: " << eid  << endl;
      throw KException(d1.GetErrorStr1());
    }
    else {
      // missing data causes the missing XMLElement* to come back as nullptr,
      // so we get a segmentation violation (not a catchable error).
      // To catch them all, testThrow would have to alternate with "->".
      auto testThrow = [] (void * pt, string msg) {
        if (nullptr == pt) {
          throw (KException(msg));
        }
        return;
      };

      XMLElement* tableEl = d1.FirstChildElement( "Workbook" )->FirstChildElement( "Worksheet" )->FirstChildElement( "Table" );

      // read the first row
      XMLElement* rowEl = nullptr;
      int numAct = 0;
      int numDim = 0;
      try {
        rowEl = tableEl->FirstChildElement("Row");
        XMLElement* sNameEl = rowEl->FirstChildElement("Cell");
        const char * sName = sNameEl->FirstChildElement("Data")->GetText();
        XMLElement* sDescEl = sNameEl->NextSiblingElement("Cell");

        XMLElement* numActEl = sDescEl->NextSiblingElement("Cell");
        numAct = atoi(numActEl->FirstChildElement("Data")->GetText());

        XMLElement* numDimEl = numActEl->NextSiblingElement("Cell");
        numDim = atoi(numDimEl->FirstChildElement("Data")->GetText());
        printf( "Name of scenario: %s\n", sName );
        printf( "Desc of scenario: %s\n", sDescEl->FirstChildElement("Data")->GetText() );
        printf( "Number of actors: %i\n", numAct);
        printf( "Number of dims:   %i\n", numDim);
      }
      catch (...) {
        throw (KException("Error reading file header"));
      }

      // read the second row, which holds the column names
      rowEl = rowEl->NextSiblingElement("Row");
      unsigned int numRetAct = 0;

      // read the third and later rows, each of which holds one actor
      // name, desc, power, pos1, sal1, pos2, sal2, pos3, sal3
      rowEl = rowEl->NextSiblingElement("Row");
      try {
        while (nullptr != rowEl) {
          XMLElement* aNameEl = rowEl->FirstChildElement("Cell");
          XMLElement* aDescEl = aNameEl->NextSiblingElement("Cell");
          XMLElement* aCapEl = aDescEl->NextSiblingElement("Cell");
          const char * aName = aNameEl->FirstChildElement("Data")->GetText();
          const char * aDesc = aDescEl->FirstChildElement("Data")->GetText();
          const double aCap = atof(aCapEl->FirstChildElement("Data")->GetText());

          printf("Actor name: %s \n", aName);
          printf("Actor desc: %s \n", aDesc);
          printf("Actor cap: %+.3f \n", aCap);
          XMLElement* psEl = aCapEl->NextSiblingElement("Cell");
          for (unsigned int i=0; i<numDim; i++) {
            const double posI = atof(psEl->FirstChildElement("Data")->GetText());
            printf("  Pos %i: %.3f \n", i, posI);
            psEl = psEl->NextSiblingElement("Cell");
            const double salI = atof(psEl->FirstChildElement("Data")->GetText());
            printf("  Sal %i: %.3f \n", i, salI);
            cout << "  -------------" << endl << flush;

            psEl = psEl->NextSiblingElement("Cell");
          }
          cout << endl << flush;
          numRetAct++;
          rowEl = rowEl->NextSiblingElement("Row");
        }
        cout << "Retrieved " << numRetAct << " actors" << flush;
        assert (numAct == numRetAct);
        cout << " as expected" << endl;
      }
      catch (...) {
        throw (KException("Error reading actor data"));
      }
    }
  }
  catch (const KException& ke) {
    cout << "Caught KException in demoTX2: "<< ke.msg <<endl<<flush;
  }
  /*
    catch(const std::exception& e) {
        // this executes if f() throws std::logic_error (base class rule)
    }
    */
  catch(...) {
    cout << "Caught unidentified exception in demoTX2"<<endl<<flush;
  }

  return;
}

}; // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
