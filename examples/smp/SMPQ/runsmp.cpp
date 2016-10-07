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
// -------------------------------------------------

#include "smp.h"
#include "demosmp.h"
#include <functional>
#include "mainwindow.h"

// Copied this code snippet from demosmp.cpp

using KBase::PRNG;
using KBase::KMatrix;
using KBase::Actor;
using KBase::Model;
using KBase::Position;
using KBase::State;
using KBase::VotingRule;
using KBase::VPModel;

namespace DemoSMP {

  using std::cout;
  using std::endl;
  using std::flush;
  using std::function;
  using std::get;
  using std::string;
  using std::vector;

  using KBase::ReportingLevel;

  using SMPLib::SMPModel;
  using SMPLib::SMPActor;
  using SMPLib::SMPState;

  // -------------------------------------------------
  // this binds the given parameters and returns the λ-fn necessary to stop the SMP appropriately
  function<bool(unsigned int, const State *)>
  smpStopFn(unsigned int minIter, unsigned int maxIter, double minDeltaRatio, double minSigDelta) {
    auto  sfn = [minIter, maxIter, minDeltaRatio, minSigDelta](unsigned int iter, const State * s) {
        bool tooLong = (maxIter <= iter);
        bool longEnough = (minIter <= iter);
        bool quiet = false;
        auto sf = [](unsigned int i1, unsigned int i2, double d12) {
            printf("sDist [%2i,%2i] = %.2E   ", i1, i2, d12);
            return;
          };
        auto s0 = ((const SMPState*)(s->model->history[0]));
        auto s1 = ((const SMPState*)(s->model->history[1]));
        auto d01 = SMPModel::stateDist(s0, s1) + minSigDelta;
        sf(0, 1, d01);
        auto sx = ((const SMPState*)(s->model->history[iter - 0]));
        auto sy = ((const SMPState*)(s->model->history[iter - 1]));
        auto dxy = SMPModel::stateDist(sx, sy);
        sf(iter - 1, iter - 0, dxy);
        const double aRatio = dxy / d01;
        quiet = (aRatio < minDeltaRatio);
        printf("\nFractional change compared to first step: %.4f  (target=%.4f) \n\n", aRatio, minDeltaRatio);
        return tooLong || (longEnough && quiet);
      };
    return sfn;
  };

  // JAH 20160802 added this new function to actually run a model
  // this is all copied from old readEUSpatial and demoEUSpatial
  void executeSMP(SMPModel * md0)
  {
    // setup the stopping criteria and lambda function
    const unsigned int minIter = 2;
    const unsigned int maxIter = 100;
    const double minDeltaRatio = 0.02;
    // suppose that, on a [0,100] scale, the first move was the most extreme possible,
    // i.e. 100 points. One fiftieth of that is just 2, which seems to about the limit
    // of what people consider significant.
    const double minSigDelta = 1E-4;
    // typical first shifts are on the order of numAct/10, so this is low
    // enough not to affect anything while guarding against the theoretical
    // possiblity of 0/0 errors
    md0->stop = [maxIter](unsigned int iter, const State * s) {
        return (maxIter <= iter);
      };
    md0->stop = smpStopFn(minIter, maxIter, minDeltaRatio, minSigDelta);

    // execute
    cout << "Starting model run" << endl << flush;
    md0->run();
    const unsigned int nState = md0->history.size();

    // log data, or not
    // JAH 20160731 added to either log all information tables or none
    // this takes care of info re. actors, dimensions, scenario, capabilities, and saliences
    if (md0->sqlFlags[0])
      {
        md0->LogInfoTables();
      }
    // JAH 20160802 added logging control flag for the last state
    // also added the sqlPosVote and sqlPosEquiv calls to get the final state
    if (md0->sqlFlags[1])
      {
        md0->sqlAUtil(nState - 1);
        md0->sqlPosProb(nState - 1);
        md0->sqlPosEquiv(nState-1);
        md0->sqlPosVote(nState-1);
      }

    cout << "Completed model run" << endl << endl;
    printf("There were %u states, with %i steps between them\n", nState, nState - 1);
    cout << "History of actor positions over time" << endl;
    md0->showVPHistory();

    return;
  }

  void readEUSpatial(uint64_t seed, string inputCSV,  vector<bool> f, string dbFilePath) {
    SMPModel::setDBPath(dbFilePath); // setting DB Name
    // JAH 20160711 added rng seed 20160730 JAH added sql flags
    auto md0 = SMPModel::readCSV(inputCSV, seed, f);

    // JAH 20160802 added call to executeSMP
    executeSMP(md0);
    // output what R needs for Sankey diagrams
    md0->sankeyOutput(inputCSV);

    delete md0;
    return;
  }
} // end of namespace

void MainWindow::runPushButtonClicked(bool bl)
{
  Q_UNUSED(bl)

  QDateTime UTC = QDateTime::currentDateTime().toTimeSpec(Qt::UTC);
  QString name (UTC.toString());

  name.replace(" ","_").replace(":","_");

  QString dbFilePath;

  QFileDialog fileDialog(this);
  dbFilePath = fileDialog.getSaveFileName(this, tr("Save DB file as "),name,tr("DB File (*.db)"),0,QFileDialog::DontConfirmOverwrite);

  if(!dbFilePath.isEmpty())
    {
      runButton->setEnabled(false);

      if(!dbFilePath.endsWith(".db"))
        dbFilePath.append(".db");

      statusBar()->showMessage(" Please wait !! Executing SMP ....  This may take a while ....");

      // A code snippet from Demosmp.cpp to run the model

      using KBase::dSeed;
      auto sTime = KBase::displayProgramStart(DemoSMP::appName, DemoSMP::appVersion);
      uint64_t seed = dSeed;

      // JAH 20160730 vector of SQL logging flags for 4 groups of tables:
      // 0 = Information Tables, 1 = Position Tables, 2 = EDU Tables, 3 = Bargain Resolution Tables
      std::vector<bool> sqlFlags = {true,true,true,true};

     
      // Notice that we NEVER use anything but the default seed.
      printf("Using PRNG seed:  %020llu \n", seed);
      printf("Same seed in hex:   0x%016llX \n", seed);

      DemoSMP::readEUSpatial(seed, csvPath.toStdString(),sqlFlags, dbFilePath.toStdString());
       
      KBase::displayProgramEnd(sTime);
      statusBar()->showMessage(" Process Completed !! ");
      smpDBPath(dbFilePath);

    }
  else
    {
       statusBar()->showMessage("SMP Model Run Cancelled !! ");
      QMessageBox::warning(this,"Warning", "SMP Model Run Cancelled !! ",QMessageBox::Ok);
      runButton->setEnabled(true);
    }
}

void MainWindow::smpDBPath(QString smpdbPath)
{
  dbGetFilePAth(true,smpdbPath,true);
}

void MainWindow::disableRunButton(QTableWidgetItem *itm)
{
  Q_UNUSED(itm)
  runButton->setEnabled(false);
  disconnect(csvTableWidget,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(disableRunButton(QTableWidgetItem*)));

}


// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
