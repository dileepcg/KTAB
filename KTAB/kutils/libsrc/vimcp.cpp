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
// Very basic variational inequalities and mixed complementarity.
// An excellent reference (e.g. to the Basic Gradient algorithms) is the two-volume set
// "Finite-Dimensional Variational Inequalities and Complementarity Problems" by Facchinei and Pang
// -------------------------------------------------

#include "vimcp.h"

namespace KBase {

using std::cout;
using std::endl;
using std::flush;
using std::tuple;

KMatrix projPos(const KMatrix & w) {
  auto pos = [&w](unsigned int i, unsigned int j){
    double x = w(i, j);
    x = (0 < x) ? x : 0;
    return x;
  };
  return KMatrix::map(pos, w.numR(), w.numC());
}


KMatrix projBox(const KMatrix & lb, const KMatrix & ub, const KMatrix & w) {
  auto box = [&lb, &ub, &w](unsigned int i, unsigned int j){
    double lij = lb(i, j);
    double uij = ub(i, j);
    assert(lij <= uij);
    double x = w(i, j);
    x = (lij < x) ? x : lij;
    x = (x < uij) ? x : uij;
    return x;
  };
  return KMatrix::map(box, w.numR(), w.numC());
}


// -------------------------------------------------
tuple<KMatrix, unsigned int, KMatrix> viABG(const KMatrix & xInit,
                                            function<KMatrix(const KMatrix & x)> F,
                                            function<KMatrix(const KMatrix & x)> P,
                                            double beta, double thresh, unsigned int iMax,
                                            bool extra) {

  if ((beta <= 0) || (1 <= beta) || (thresh <= 0)) {
    throw KException("viABG: invalid input parameters");
  }

  KMatrix e0;
  auto x0 = P(xInit);
  auto f0 = F(x0);
  assert(1 == x0.numC());
  assert(1 == f0.numC());
  double change = 10.0 * thresh;
  double estL = 1.0E8;
  unsigned int iter = 0;
  while (change > thresh) {
    e0 = x0 - P(x0 - f0);
    double gamma = beta / estL;
    auto x1 = P(x0 - gamma * f0);
    auto f1 = F(x1);
    estL = norm(f1 - f0) / norm(x1 - x0);

    if (extra) {
      const auto x1b = P(x0 - gamma * f1);
      const auto f1b = F(x1b);

      x1 = x1b;
      f1 = f1b;
    }

    change = maxAbs(x1 - x0) / gamma;
    x0 = x1;
    f0 = f1;
    iter++;
    if (iMax < iter) { throw(KException("viABG: iteration limit exceeded")); }
  }

  return tuple<KMatrix, unsigned int, KMatrix>(x0, iter, e0);
}


// -------------------------------------------------
// This is my implementation of BSHe96's method for solving linear variational inequalities:
// find x in K s.t. for all y in  K, (Mx+q)*(y-x)>=0
// This requires that M be positive semi-definite and K convex.
//
// "A Modified Projection and Contraction Method for a Class of Linear Complementarity Problems",
// B. S. He, Nanjing University, in Journal of Computational Mathematics, 1996

tuple<KMatrix, unsigned int, KMatrix> viBSHe96(const KMatrix & M, const KMatrix & q,
                                               function<KMatrix(const KMatrix &)> pK,
                                               KMatrix u0, const double eps, const unsigned int iMax) {
  if (false) {
    cout << "Received M:" << endl;
    M.mPrintf("%+.4f  ");
    cout << endl;
    cout << "Received q:" << endl;
    trans(q).mPrintf("%+.4f  ");
    cout << endl << endl << flush;
  }

  unsigned int n = q.numR();
  assert(1 == q.numC());
  assert(n == M.numR());
  assert(n == M.numC());
  assert(n == u0.numR());
  assert(1 == u0.numC());
  assert(eps > 0.0);

  double gamma = 1.8; // any 0<gamma<2 will do. Note that 1.618034 = (1+sqrt(5))/2
  KMatrix Mt = trans(M);
  double qMax = maxAbs(q);
  assert(qMax > 0.0);
  KMatrix I = iMat(n);

  // for general VI, e(u) = u - pK(u - F(u))
  // will have e(u)=0 iff u solves the VI.
  auto err = [&M, &q, pK](const KMatrix & u) {
    KMatrix proj = pK(u - (M*u + q));
    return (u - proj);
  };
  auto normS = [](const KMatrix & m) {
    double n = norm(m);
    return (n*n);
  };


  KMatrix u1 = pK(u0); // project onto K before first iteration
  KMatrix e1 = err(u1);
  double r = maxAbs(e1) / qMax;
  unsigned int iter = 0;

  while (r > eps) {
    KMatrix g1 = Mt*e1 + (M*u1 + q);
    double rho = normS(e1) / normS((I + Mt)*e1);
    KMatrix u2 = pK(u1 - gamma*rho*g1);
    KMatrix e2 = err(u2);

    iter++;
    assert(iter < iMax);
    u1 = u2;
    e1 = e2;
    r = maxAbs(e1) / qMax;
  }
  auto trpl = tuple<KMatrix, unsigned int, KMatrix>(u1, iter, e1);
  return trpl;
}

}; // namespace


// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
