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

        //get parameters from GUI
        getParametersValues();

        // A code snippet from Demosmp.cpp to run the model

        using KBase::dSeed;
        auto sTime = KBase::displayProgramStart(DemoSMP::appName, DemoSMP::appVersion);
        uint64_t seed = dSeed;

        //Collect flags and Data from GUI
        if(seedRand->text().length()>0 && !seedRand->text().isNull())
        {
            seed = seedRand->text().toULongLong();
        }
        if (0 == seed)
        {
            PRNG * rng = new PRNG();
            seed = rng->setSeed(seed); // 0 == get a random number
            delete rng;
            rng = nullptr;
        }

        // JAH 20160730 vector of SQL logging flags for 5 groups of tables:
        // 0 = Information Tables, 1 = Position Tables, 2 = Challenge Tables,
        // 3 = Bargain Resolution Tables, 4 = VectorPosition table
        // JAH 20161010 added group 4 for only VectorPos so it can be logged alone
        std::vector<bool> sqlFlags = {true,true,true,true,true};

        //log minimum
        if(logMinimum->isChecked())
        {
            sqlFlags = {true,false,false,false,true};
        }

        // Notice that we NEVER use anything but the default seed.
        printf("Using PRNG seed:  %020llu \n", seed);
        printf("Same seed in hex:   0x%016llX \n", seed);
        currentScenarioId =QString::fromStdString(SMPLib::SMPModel::csvReadExec
                                                   (seed, csvPath.toStdString(),sqlFlags,
                                                    dbFilePath.toStdString(),parameters));
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
    useHistory=true;
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

//Model Parameters

void MainWindow::initializeModelParametersDock()
{
    modelParametersFrame = new QFrame(modelParametersDock);
    VLayout  = new QGridLayout(modelParametersFrame);

    frames = new QFrame(modelParametersDock);
    frames->setFrameShape(QFrame::StyledPanel);//change to flat if required
    VLayout->addWidget(frames,0,0);

    //LogMinimum
    logMinimum = new QCheckBox("Minimum Logging");
    logMinimum->setChecked(true);
    logMinimum->setToolTip("Please be aware the turning on Minimum Logging will result in\n"
                           " SMP not storing sufficient data to generate the quadmap.");
    VLayout->addWidget(logMinimum,1,0);

    //Seed
    //    QLabel * seedLabel = new QLabel("Randomizer Seed");
    // VLayout->addWidget(seedLabel,2,0);
    seedRand = new QLineEdit;
    seedRand->setPlaceholderText("Enter Random Seed");
    VLayout->addWidget(seedRand,2,0);

    //RUN Button
    runButton = new QPushButton;
    runButton->setText("RUN");
    runButton->setEnabled(false);
    VLayout->addWidget(runButton,3,0);

    connect(runButton,SIGNAL(clicked(bool)),this,SLOT(runPushButtonClicked(bool)));

    //Radio buttons //Section1
    QGridLayout *parametersGLayout = new QGridLayout(frames);

    victProbModelComboBox = new QComboBox;
    pCEModelComboBox = new QComboBox;
    stateTransitionsComboBox = new QComboBox;
    votingRuleComboBox = new QComboBox;
    bigRAdjustComboBox = new QComboBox;
    bigRRangeComboBox = new QComboBox;
    thirdPartyCommitComboBox = new QComboBox;
    interVecBrgnComboBox = new QComboBox;
    bargnModelComboBox = new QComboBox;

    QLabel *victProbLabel = new QLabel(" VictoryProbModel ");
    victProbLabel->setFrameStyle(QFrame::StyledPanel);
    victProbLabel->setAlignment(Qt::AlignHCenter);

    QLabel *pCELabel =  new QLabel(" ProbCondorcetElection ");
    pCELabel->setFrameStyle(QFrame::StyledPanel);
    pCELabel->setAlignment(Qt::AlignHCenter);

    QLabel *stateTransitionsLabel =  new QLabel(" StateTransition ");
    stateTransitionsLabel->setFrameStyle(QFrame::StyledPanel);
    stateTransitionsLabel->setAlignment(Qt::AlignHCenter);

    QLabel *votingRuleLabel =  new QLabel(" VotingRule ");
    votingRuleLabel->setFrameStyle(QFrame::StyledPanel);
    votingRuleLabel->setAlignment(Qt::AlignHCenter);

    QLabel *bigRAdjustLabel =  new QLabel(" BigRAdjust ");
    bigRAdjustLabel->setFrameStyle(QFrame::StyledPanel);
    bigRAdjustLabel->setAlignment(Qt::AlignHCenter);

    QLabel *bigRRangeLabel =  new QLabel(" BigRRange ");
    bigRRangeLabel->setFrameStyle(QFrame::StyledPanel);
    bigRRangeLabel->setAlignment(Qt::AlignHCenter);

    QLabel *thirdPartyCommitLabel =  new QLabel(" ThirdPartyCommit ");
    thirdPartyCommitLabel->setFrameStyle(QFrame::StyledPanel);
    thirdPartyCommitLabel->setAlignment(Qt::AlignHCenter);

    QLabel *interVecBrgnLabel =  new QLabel(" InterVecBrgn ");
    interVecBrgnLabel->setFrameStyle(QFrame::StyledPanel);
    interVecBrgnLabel->setAlignment(Qt::AlignHCenter);

    QLabel *bargnModelLabel =  new QLabel(" BargnModel ");
    bargnModelLabel->setFrameStyle(QFrame::StyledPanel);
    bargnModelLabel->setAlignment(Qt::AlignHCenter);

    QStringList victList;
    victList << "Linear" << "Square" << "Quartic" << "Octic" << "Binary";

    QStringList pceList;
    pceList << "Conditional" << "MarkovIncentive" << "MarkovUniform";

    QStringList stateList;
    stateList << "Deterministic" << "Stochastic";

    QStringList votingList;
    votingList << "Binary" << "PropBin"<< "Proportional"<< "PropCbc"<< "Cubic"<< "ASymProsp";

    QStringList bigAdjList;
    bigAdjList << "None" << "OneThird" << "Half"<< "TwoThirds"<< "Full";

    QStringList bigRangeList;
    bigRangeList << "Min" << "Mid" << "Max";

    QStringList thridpartyList;
    thridpartyList << "NoCommit" << "SemiCommit" << "FullCommit";

    QStringList interVecList;
    interVecList << "S1P1" << "S2P2" << "S2PMax";

    QStringList bargnList;
    bargnList << "InitOnlyInterp" << "InitRcvrInterp" << "PWCompInterp";

    QVBoxLayout * v1 = new QVBoxLayout;
    victProbModelComboBox->addItems(victList);
    v1->addWidget(victProbLabel,0,Qt::AlignBottom);
    v1->addWidget(victProbModelComboBox,0,Qt::AlignTop);
    parametersGLayout->addLayout(v1,0,0);

    QVBoxLayout * v2 = new QVBoxLayout;
    pCEModelComboBox->addItems(pceList);
    v2->addWidget(pCELabel,0,Qt::AlignBottom);
    v2->addWidget(pCEModelComboBox,0,Qt::AlignTop);
    parametersGLayout->addLayout(v2,0,1);

    QVBoxLayout * v3 = new QVBoxLayout;
    stateTransitionsComboBox->addItems(stateList);
    v3->addWidget(stateTransitionsLabel,0,Qt::AlignBottom);
    v3->addWidget(stateTransitionsComboBox,0,Qt::AlignTop);
    parametersGLayout->addLayout(v3,0,2);

    QVBoxLayout * v4 = new QVBoxLayout;
    votingRuleComboBox->addItems(votingList);
    v4->addWidget(votingRuleLabel,0,Qt::AlignBottom);
    v4->addWidget(votingRuleComboBox,0,Qt::AlignTop);
    parametersGLayout->addLayout(v4,1,0);

    QVBoxLayout * v5 = new QVBoxLayout;
    bigRAdjustComboBox->addItems(bigAdjList);
    v5->addWidget(bigRAdjustLabel,0,Qt::AlignBottom);
    v5->addWidget(bigRAdjustComboBox,0,Qt::AlignTop);
    parametersGLayout->addLayout(v5,1,1);

    QVBoxLayout * v6 = new QVBoxLayout;
    bigRRangeComboBox->addItems(bigRangeList);
    v6->addWidget(bigRRangeLabel,0,Qt::AlignBottom);
    v6->addWidget(bigRRangeComboBox,0,Qt::AlignTop);
    parametersGLayout->addLayout(v6,1,2);

    QVBoxLayout * v7 = new QVBoxLayout;
    thirdPartyCommitComboBox->addItems(thridpartyList);
    v7->addWidget(thirdPartyCommitLabel,0,Qt::AlignBottom);
    v7->addWidget(thirdPartyCommitComboBox,0,Qt::AlignTop);
    parametersGLayout->addLayout(v7,2,0);

    QVBoxLayout * v8 = new QVBoxLayout;
    interVecBrgnComboBox->addItems(interVecList);
    v8->addWidget(interVecBrgnLabel,0,Qt::AlignBottom);
    v8->addWidget(interVecBrgnComboBox,0,Qt::AlignTop);
    parametersGLayout->addLayout(v8,2,1);

    QVBoxLayout * v9 = new QVBoxLayout;
    bargnModelComboBox->addItems(bargnList);
    v9->addWidget(bargnModelLabel,0,Qt::AlignBottom);
    v9->addWidget(bargnModelComboBox,0,Qt::AlignTop);
    parametersGLayout->addLayout(v9,2,2);

    modelParametersDock->setWidget(modelParametersFrame);
    addDockWidget(Qt::LeftDockWidgetArea, modelParametersDock);
    viewMenu->addAction(modelParametersDock->toggleViewAction());

    modelParametersDock->setVisible(true);
}

void MainWindow::getParametersValues()
{
    victProbModel= victProbModelComboBox->currentIndex();
    pCEModel = pCEModelComboBox->currentIndex();
    stateTransitions = stateTransitionsComboBox->currentIndex();
    votingRule = votingRuleComboBox->currentIndex();
    bigRAdjust = bigRAdjustComboBox->currentIndex();
    bigRRange = bigRRangeComboBox->currentIndex();
    thirdPartyCommit= thirdPartyCommitComboBox->currentIndex();
    interVecBrgn = interVecBrgnComboBox->currentIndex();
    bargnModel = bargnModelComboBox->currentIndex();

    parameters={victProbModel,pCEModel,stateTransitions,votingRule,bigRAdjust,bigRRange,thirdPartyCommit,
                interVecBrgn,bargnModel};

}

//QStandardItemModel * MainWindow::initializeComboBox(int rows, QStringList items)
//{
//    QStandardItemModel * model= new QStandardItemModel(rows, 1); // 3 rows, 1 col
//    for (int r = 0; r < rows; ++r)
//    {
//        QStandardItem* item = new QStandardItem(QString(items.at(r)));

//        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
//        item->setData(Qt::Unchecked, Qt::CheckStateRole);

//        model->setItem(r, 0, item);
//    }

//    return model;
//}






// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
