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

#ifndef RP2_MATRIX_H
#define RP2_MATRIX_H

#include "kutils.h"
#include "prng.h"
#include "kmatrix.h"
#include "kmodel.h"

#include "emodel.h"
#include "emodel.cpp"




namespace RfrmPri2 {
using std::string;
using std::vector;
using std::tuple;

using KBase::VUI;
using KBase::KMatrix;
using KBase::ReportingLevel;
using KBase::EModel;
using KBase::EPosition;
using KBase::EState;

// --------------------------------------------

void initScen(uint64_t sd);

// --------------------------------------------
class RP2State;
// --------------------------------------------
// The RP2Model uses a pre-specified matrix
// of actor-vs-policy utilities, utility of a different
// order of reform priority items.
// The unsigned int in the template is just the column of a policy.
class RP2Model : public EModel<unsigned int> {
public:
  explicit RP2Model(string d = "", uint64_t s = KBase::dSeed, vector<bool> = {});
  virtual ~RP2Model();

  // check the matrix and set as utilities,
  // if valid for [numAct,numOpt] utilities in [0,1] range.
  void setRP2(const KMatrix & pm0);

  // check the row-matrix and set as weight,
  // if valid for non-negative [numAct,1] weights
  void  setWeights(const KMatrix & w0);

  // must set weights and RP2 first
  void  setActors(vector<string> names, vector<string> descriptions);

  KMatrix getWghtVect() const {
    return wghtVect; // row vector
  };

  KMatrix getPolUtilMat() const {
    return polUtilMat; // rectangular
  };


protected:
  KMatrix wghtVect; // column vector of actor weights
  KMatrix polUtilMat; // if set, the basic util(actor,option) matrix

private:
};



class RP2Pos : public EPosition<unsigned int> {
public:
  explicit RP2Pos(RP2Model* pm, int n);
  virtual ~RP2Pos();

protected:

private:

};



class RP2State : public EState<unsigned int> {
public:
  explicit RP2State(RP2Model* pm);
  virtual ~RP2State();

  // We expect hundreds or even thousands policies,
  // so we cannot use all of Theta.
  virtual VUI similarPol(unsigned int ti, unsigned int numPol = 0) const;

protected:
  EState<unsigned int>* makeNewEState() const override;
  void setAllAUtil(ReportingLevel rl) override;
  vector<double> actorUtilVectFn(int h, int tj) const override;

private:

};

// --------------------------------------------
RP2Model* pmmCreation(uint64_t sd);
RP2Pos*  pmpCreation(RP2Model* pmm);
RP2State* pmsCreation(RP2Model * pmm);

string genName(const string & prefix, unsigned int i);

}
// end of namespace


#endif
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
