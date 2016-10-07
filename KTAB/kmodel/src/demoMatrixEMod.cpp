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


#include "demoMatrixEMod.h"


namespace eModKEM {
  using std::cout;
  using std::endl;
  using std::flush;
  using std::string;
  using std::vector;
  using KBase::KMatrix;
  using KBase::EModel;
  using KBase::EState;
  using KBase::EActor;
  using KBase::EPosition;


  const KMatrix weightMat = KMatrix::arrayInit(weightArray, numActKEM, 1);
  const KMatrix utilMat = KMatrix::arrayInit(utilArray, numActKEM, numPolKEM);
  
// --------------------------------------------
  
EKEModel::EKEModel(string d, uint64_t s, vector<bool> vb) : EModel< char >(d,s,vb) {
    // nothing yet
}


EKEModel::~EKEModel() {
    // nothing yet
}


// --------------------------------------------

  void demoEKem(uint64_t s) {
    cout << endl;
    printf("Using PRNG seed: %020llu \n", s);

    printf("Creating EModel objects ... \n");

    string nChar = "EModel-KEM-Policy";
    auto ekm = new EModel<char>(nChar, s);
    cout << "Populating " << nChar << endl;

    // with such a small set of options, the enumerator
    // function just returns a fixed vector of names. 
    ekm->enumOptions = []() { return thetaKEM; };
    ekm->setOptions();
    cout << "Now have " << ekm->numOptions() << " enumerated options" << endl;

    // these just to force instantiation during compilation.
    // It is NOT yet ready to run.   
    auto eks = new EState<char>(ekm);
    auto ekp = new EPosition<char>(ekm, 0); // needs ::print
    auto eka = new EActor<char>(ekm, "Bob", "The second cryptographer");
    
    // cannot delete this, because it would try to delete some
    // constant strings, and crash.
    // printf("Deleting %s ... \n", nChar.c_str());
    // delete ekm;

    printf("Creating EKEModel objects ... \n");
    
    auto eKEM = new EKEModel();
    
    delete eKEM;

    return;
  }

} // end of namespace eModKEM


// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
