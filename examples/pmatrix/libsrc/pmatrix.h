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

#ifndef POLMATRIX_H
#define POLMATRIX_H

#include "kutils.h"
#include "prng.h"
#include "kmatrix.h"
#include "kmodel.h"

#include "emodel.h"
#include "emodel.cpp"




namespace PMatDemo {
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

typedef tuple <vector<string>,  // aNames,
vector<bool>,           // maxVect,
KMatrix,                // outcomes,
KMatrix,                // caseWeights,
KMatrix,                // probWeight,
vector<double>,         // threshVal,
vector<bool>>           // overThresh
FittingParameters;

// --------------------------------------------
//class PMatrixPos;
class PMatrixState;
// --------------------------------------------
// The Policy Matrix model uses a pre-specified matrix
// of actor-vs-policy utilities.
// The utility matrix provides a (basic) linkage to offline
// models such as KEM/GAMS.
// The unsigned int in the template is just the column of a policy.
class PMatrixModel : public EModel<unsigned int> {
public:
  explicit PMatrixModel(string d = "", uint64_t s = KBase::dSeed, vector<bool> = {});
  virtual ~PMatrixModel();

  // check the matrix and set as utilities,
  // if valid for [numAct,numOpt] utilities in [0,1] range.
  void setPMatrix(const KMatrix & pm0);

  // check the row-matrix and set as weight,
  // if valid for non-negative [numAct,1] weights
  void  setWeights(const KMatrix & w0);

  // must set weights and pMatrix first
  void  setActors(vector<string> names, vector<string> descriptions);

  KMatrix getWghtVect() const {
    return wghtVect; // row vector
  };

  KMatrix getPolUtilMat() const {
    return polUtilMat; // rectangular
  };

  static KMatrix utilFromFP(const FittingParameters & fParams, double bigR);

  static tuple<double, KMatrix, KMatrix> minProbError(
      const FittingParameters & fParams,
      double bigR, double errWeight);

protected:
  KMatrix wghtVect; // column vector of actor weights
  KMatrix polUtilMat; // if set, the basic util(actor,option) matrix

  // This assess and returns the sum of three kinds of costs.
  // The 'pnt' specifies an adjustment vector and incurs
  // a cost to the extent that the components differ from 0.
  // The adjusted wMat is then separately adjusted for case 1 and case 2,
  // and a cost is incurred to the extend that selected probabilities
  // fall below their respective thresholds.
  static tuple<double, KMatrix, KMatrix> probCost(const KMatrix& pnt,
                                                  const KMatrix& wMat, const KMatrix& uMat,
                                                  const KMatrix& wAdj1, const KMatrix& pSel1, double thresh1,
                                                  const KMatrix& wAdj2, const KMatrix& pSel2, double thresh2,
                                                  double errWeight, ReportingLevel rl);

private:
};



class PMatrixPos : public EPosition<unsigned int> {
public:
  explicit PMatrixPos(PMatrixModel* pm, int n);
  virtual ~PMatrixPos();

protected:

private:

};



class PMatrixState : public EState<unsigned int> {
public:
  explicit PMatrixState(PMatrixModel* pm);
  virtual ~PMatrixState();

  // As we expect fairly few policies (dozens, not thousands),
  // we just return indices to all of theta, ignoring ti and numPol
  virtual VUI similarPol(unsigned int ti, unsigned int numPol = 0) const override;

protected:
  EState<unsigned int>* makeNewEState() const override;
  void setAllAUtil(ReportingLevel rl) override;
  vector<double> actorUtilVectFn(int h, int tj) const override;

private:

};

// --------------------------------------------
PMatrixModel* pmmCreation(uint64_t sd);
PMatrixPos*  pmpCreation(PMatrixModel* pmm);
PMatrixState* pmsCreation(PMatrixModel * pmm);

string genName(const string & prefix, unsigned int i);

}
// end of namespace


#endif
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
