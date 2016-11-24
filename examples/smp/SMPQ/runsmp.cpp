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


} // end of namespace

void MainWindow::runPushButtonClicked(bool bl)
{
    Q_UNUSED(bl)

    QDateTime UTC = QDateTime::currentDateTime().toTimeSpec(Qt::UTC);
    QString name (UTC.toString());

    name.replace(" ","_").replace(":","_");

    logSMPDataOptionsAnalysis();

    QString dbFilePath;

    QFileDialog fileDialog(this);
    dbFilePath = fileDialog.getSaveFileName(this, tr("Save DB file as "),name,tr("DB File (*.db)"),0,QFileDialog::DontConfirmOverwrite);

    if(!dbFilePath.isEmpty())
    {
        runButton->setEnabled(false);

        if(!dbFilePath.endsWith(".db"))
            dbFilePath.append(".db");

        statusBar()->showMessage(" Please wait !! Executing SMP ....  This may take a while ....");

        QApplication::setOverrideCursor(QCursor(QPixmap("://images/hourglass.png")));
        QApplication::processEvents();

        // A code snippet from Demosmp.cpp to run the model

        using KBase::dSeed;
        auto sTime = KBase::displayProgramStart(DemoSMP::appName, DemoSMP::appVersion);
        uint64_t seed = dSeed;

        // JAH 20160730 vector of SQL logging flags for 5 groups of tables:
        // 0 = Information Tables, 1 = Position Tables, 2 = Challenge Tables,
        // 3 = Bargain Resolution Tables, 4 = VectorPosition table
        // JAH 20161010 added group 4 for only VectorPos so it can be logged alone
        std::vector<bool> sqlFlags = {true,true,true,true,true};

        // Notice that we NEVER use anything but the default seed.
        printf("Using PRNG seed:  %020llu \n", seed);
        printf("Same seed in hex:   0x%016llX \n", seed);
        SMPLib::SMPModel::csvReadExec(seed, csvPath.toStdString(),sqlFlags, dbFilePath.toStdString());
        KBase::displayProgramEnd(sTime);

        QApplication::restoreOverrideCursor();

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

void MainWindow::logSMPDataOptionsAnalysis()
{
    QDateTime UTC = QDateTime::currentDateTime().toTimeSpec(Qt::UTC);
    QString name (UTC.toString());
    name.replace(" ","_").replace(":","_");

    if(logDefaultAct->isChecked()==true)
    {
        if(logFileName.isEmpty())
        {
            fclose(stdout);
            fp_old = *stdout;
            QString logFileNewName = name;
            logFileNewName.append("DefaultLog.txt");
            logFileName = logFileNewName;
        }
        stream = freopen(logFileName.toStdString().c_str(),"a+",stdout);
    }
    else if(logNewAct->isChecked()==true)
    {
        fclose(stdout);
        fp_old = *stdout;
        QString logFileNewName = name;
        logFileNewName.append("Log");
        QString saveLogFilePath = QFileDialog::getSaveFileName(this, tr("Save Log File to "),logFileNewName,
                                                               tr("TextT File (*.txt)"),0,QFileDialog::DontConfirmOverwrite);
        if(!saveLogFilePath.isEmpty())
        {
            if(!saveLogFilePath.endsWith(".txt"))
                saveLogFilePath.append(".txt");

            logFileNewName=saveLogFilePath;
        }
        stream = freopen(logFileNewName.toStdString().c_str(),"a+",stdout);
    }
    else
    {
        if(logNoneAct->isChecked()==true)
        {
            fclose(stdout);
            FILE* outfile = fopen ("/dev/null", "w");
            if (outfile != NULL)
            {
                *stdout = *outfile;
                stream=freopen("/dev/null", "w" ,stdout);
            }
            else
            {
                outfile = fopen ("NUL", "w");
                if (outfile != NULL)
                {
                    *stdout = *outfile;
                    stream=freopen("nul", "w" ,stdout);
                }
            }
        }
    }

}


// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
