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

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow()
{
    initializeCentralViewFrame();

    createActions();
    createStatusBar();

    createModelParametersDockWindow();
    createLinePlotsDockWindows();
    createBarPlotsDockWindows();
    createQuadMapDockWindows();

    setWindowTitle(tr("KTAB"));

    //CSV class obj
    csvObj = new CSV;
    //connect statement to pass the csv file path
    connect(this,SIGNAL(csvFilePath(QString)), csvObj, SLOT(readCSVFile(QString)));
    //connect statement to receive processed model
    connect(csvObj,SIGNAL(csvModel(QStandardItemModel*,QStringList)),
            this,SLOT(setCSVItemModel(QStandardItemModel*,QStringList)));
    //To display any message to user
    connect(csvObj,SIGNAL(sendMessage(QString,QString)),this,SLOT(displayMessage(QString,QString)));

    //Database
    dbObj = new Database();
    //To open database by passing the path
    connect(this,SIGNAL(dbFilePath(QString,bool)),dbObj, SLOT(openDB(QString,bool)));
    //To open database by passing the path
    connect(this,SIGNAL(dbEditFilePath(QString)),dbObj,SLOT(openDBEdit(QString)));
    //To get Database item Model to show on GUI
    connect(dbObj,SIGNAL(dbModel(QStandardItemModel*)),this,SLOT(setDBItemModel(QStandardItemModel*)));
    //To get Database item Model to show on Edit
    //    connect(dbObj,&Database::dbModelEdit,this,&MainWindow::setDBItemModelEdit);
    //To display any message to user
    connect(dbObj,SIGNAL(Message(QString,QString)),this,SLOT(displayMessage(QString,QString)));
    //To get vector positions of actors to plot a graph on GUI
    connect(dbObj,SIGNAL(vectorPosition(QVector<double>,QVector<double>,QString,int))
            ,this,SLOT(addGraphOnModule1(QVector<double>,QVector<double>,QString,int)));
    //To get number of states to set the max values of slider
    connect(dbObj,SIGNAL(statesCount(int)),this,SLOT(updateStateCountSliderRange(int)));

    //To get number of actors
    connect(dbObj,SIGNAL(actorCount(int)),this,SLOT(updateActorCount(int)));

    //To get all scenarios from Database to update ScenarioComboBox
    connect(dbObj,SIGNAL(scenarios(QStringList*,QStringList*,QStringList*,int)),
            this,SLOT(updateScenarioListComboBox(QStringList*,QStringList*,QStringList*,int)));
    //To get initial scenario vector positions and Item model to update graph and Table view
    connect(this, SIGNAL(getScenarioRunValues(int,QString,int)),dbObj,SLOT(getScenarioData(int,QString,int)));
    //To store scenario value, for edit database to save as csv reference
    connect(this, SIGNAL(getScenarioRunValuesEdit(QString)),dbObj,SLOT(getScenarioDataEdit(QString)));
    //To get state count
    connect(this, SIGNAL(getStateCountfromDB()),dbObj,SLOT(getStateCount()));
    //To get dimensions count
    connect(this,SIGNAL(getDimensionCountfromDB()),dbObj,SLOT(getDimensionCount()));
    //received dimension Values
    connect(dbObj,SIGNAL(dimensionsCount(int, QStringList*)),this,SLOT(updateDimensionCount(int, QStringList*)));

    //DB to CSV
    connect(this, SIGNAL(getActorsDesc()),dbObj,SLOT(getActorsDescriptionDB()));
    connect(dbObj,SIGNAL(actorsNameDesc(QList <QString> ,QList <QString>)),this,SLOT(actorsNameDesc(QList  <QString> ,QList  <QString>)));
    connect(this, SIGNAL(getInfluence(int)),dbObj,SLOT(getInfluenceDB(int)));
    connect(dbObj,SIGNAL(actorsInflu(QList<QString>)),this,SLOT(actorsInfluence(QList  <QString>)));
    connect(this, SIGNAL(getPosition(int,int)),dbObj,SLOT(getPositionDB(int,int)));
    connect(dbObj,SIGNAL(actorsPostn(QList<QString>,int)),this,SLOT(actorsPosition(QList<QString>,int)));
    connect(this, SIGNAL(getSalience(int,int)),dbObj,SLOT(getSalienceDB(int,int)));
    connect(dbObj,SIGNAL(actorsSalnce(QList<QString>,int)),this,SLOT(actorsSalience(QList<QString>,int)));
    connect(dbObj,SIGNAL(actorsAffinity(QList<QString>,QList<int>,QList<int>)),
            this,SLOT(actAffinity(QList<QString>,QList<int>,QList<int>)));

    //BAR Charts
    connect(this,SIGNAL(getActorIdsInRange(double,double,int,int)),dbObj,SLOT(getActorsInRangeFromDB(double,double,int,int)));
    connect(dbObj,SIGNAL(listActorsSalienceCapability(QList<int>,QList<double>,QList<double>,double,double)),this,
            SLOT(barGraphActorsSalienceCapability(QList<int>,QList<double>,QList<double>,double,double)));

    //LINE GRAPHS
    //To get dimensions count
    connect(this,SIGNAL(getDimforBar()),dbObj,SLOT(getDims()));
    //received dimension Values
    connect(dbObj,SIGNAL(dimensList(QStringList*)),this,SLOT(updateBarDimension(QStringList*)));
    connect(dbObj,SIGNAL(dimensList(QStringList*)),this,SLOT(updateLineDimension(QStringList*)));

    //QUAD MAPS
    //to get Util_Chlg and Util_SQ values
    connect(this,SIGNAL(getUtilChlgAndUtilSQfromDB(QList<int>)),
            dbObj,SLOT(getUtilChlgAndSQvalues(QList<int>)));

    //received Chlg and sq values
    connect(dbObj,SIGNAL(utilChlngAndSQ(int , double , double  , int )),
            this,SLOT(quadMapUtilChlgandSQValues(int , double , double  , int )));


    //XML Class signal slots
    xmlparser = new Xmlparser;
    connect(this,SIGNAL(openXMLFile(QString)),xmlparser,SLOT(openXmlFile(QString)));
    connect(this,SIGNAL(readXMLFile()),xmlparser,SLOT(readXmlFile()));
    connect(xmlparser,SIGNAL(openXMLStatus(bool)),this,SLOT(openStatusXml(bool)));
    connect(xmlparser,SIGNAL(xmlParsedData(QStringList,QStringList,QStringList,QStandardItemModel*,
                                           QList<QStringList>)),this,
            SLOT(xmlDataParsedFromFile(QStringList,QStringList,QStringList,QStandardItemModel*,
                                       QList<QStringList>)));
    connect(this,SIGNAL(saveXMLDataToFile(QStringList,QStandardItemModel*,QStandardItemModel*)),
            xmlparser,SLOT(saveToXmlFile(QStringList,QStandardItemModel*,QStandardItemModel*)));
    connect(xmlparser,SIGNAL(newXmlFilePath(QString)),this,SLOT(savedXmlName(QString)));
    connect(this,SIGNAL(saveNewSMPDataToXMLFile(QStringList,QTableWidget*,QTableWidget*)),
            xmlparser,SLOT(saveNewDataToXmlFile(QStringList,QTableWidget*,QTableWidget*)));


    //editable headers of TableWidget and TableView
    headerEditor = 0;
    barsCount=0;
    yAxisLen=100;
    prevScenario="None";
    initiatorTip=0;
    prevTurn=0;
    firstVal=false;
    useHistory =true;
    currentScenarioId = "dummy";
}

MainWindow::~MainWindow()
{

}

void MainWindow::csvGetFilePAth(bool bl)
{
    Q_UNUSED(bl)
    statusBar()->showMessage(tr("Looking for CSV file"));
    //Get  *.csv file path
    QString csvFilePth;
    csvFilePth = QFileDialog::getOpenFileName(this,tr("Open CSV File"), QDir::homePath() , tr("CSV File (*.csv)"));

    //emit path to csv class for processing
    if(!csvFilePth.isEmpty())
    {
        modeltoCSV->clear();
        emit csvFilePath(csvFilePth);

        // to pass csvfile path to smp
        csvPath = csvFilePth;
        clearAllGraphs();
    }
    statusBar()->showMessage(tr(" "));
}

void MainWindow::dbGetFilePAth(bool bl, QString smpDBPath, bool run)
{
    clearAllLabels();

    Q_UNUSED(bl)
    statusBar()->showMessage(tr("Looking for Database file ..."));

    if(smpDBPath.isEmpty())
    {
        //Get  *.db file path
        dbPath = QFileDialog::getOpenFileName(this,tr("Database File"), QDir::homePath() , tr("Database File (*.db)"));
    }
    else
        dbPath = smpDBPath;

    //emit path to db class for processing
    if(!dbPath.isEmpty())
    {
        lineGraphDock->setEnabled(true);
        barGraphDock->setEnabled(true);

        disconnect(scenarioComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(scenarioComboBoxValue(int)));
        disconnect(turnSlider,SIGNAL(valueChanged(int)),this,SLOT(sliderStateValueToQryDB(int)));
        mScenarioDesc.clear();
        mScenarioName.clear();
        mScenarioIds.clear();
        scenarioComboBox->clear();
        connect(scenarioComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(scenarioComboBoxValue(int)));
        connect(turnSlider,SIGNAL(valueChanged(int)),this,SLOT(sliderStateValueToQryDB(int)));

        modeltoDB->clear();
        emit dbFilePath(dbPath,run);

        reconnectPlotWidgetSignals();
        //To populate Line Graph Dimensions combo box
        populateLineGraphDimensions(dimensionsLineEdit->text().toInt());
        //To populate Bar Graph Dimensions combo box
        populateBarGraphDimensions(dimensionsLineEdit->text().toInt());

    }
    statusBar()->showMessage(tr(" "));
}

void MainWindow::dbEditGetFilePAth(bool bl)
{
    Q_UNUSED(bl)
    if(dbPath.isEmpty())
    {
        displayMessage("Database File", "Import a Database first, then Click on \n - Edit Database to Save as CSV");
    }
    else
    {
        emit getActorsDesc();
        emit getInfluence(0);

        for(int i = 0 ; i < dimensionsLineEdit->text().toInt();++i)
        {
            emit getSalience(i,0);
            emit getPosition(i,0);
        }
        setDBItemModelEdit();
    }
    clearAllGraphs();
}

void MainWindow::updateStateCountSliderRange(int states)
{
    turnSlider->setRange(0,states);

    //To set slider Range for line Graph
    populateLineGraphStateRange(states);
    //To set slider Range for Bar Graph
    populateBarGraphStateRange(states);
    //To set slider Range for Quad Map
    populateQuadMapStateRange(states);
}

void MainWindow::updateScenarioListComboBox(QStringList * scenarios,QStringList* scenarioIds,QStringList* scenarioDesc, int indx)
{
    for(int index=0;index<scenarios->length();++index)
    {
        mScenarioDesc.append(scenarioDesc->at(index));
        mScenarioIds.append(scenarioIds->at(index));
        mScenarioName.append(scenarios->at(index));

        disconnect(scenarioComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(scenarioComboBoxValue(int)));
        scenarioComboBox->addItem(scenarios->at(index));
        connect(scenarioComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(scenarioComboBoxValue(int)));

    }
    scenarioComboBox->setCurrentIndex(indx);
    scenarioBox = mScenarioIds.at(indx);
    scenarioDescriptionLineEdit->setText(mScenarioDesc.at(indx));

    //qDebug() <<scenarioBox;
    //    sliderStateValueToQryDB(0);//when new database is opened, start from zero
}

void MainWindow::updateDimensionCount(int dim, QStringList * dimList)
{
    dimensionList.clear();
    dimensions = dim;

    for(int i = 0 ; i < dimList->length(); ++i)
        dimensionList.append( dimList->at(i));
}

void MainWindow::updateActorCount(int actNum)
{
    numAct=actNum+1;
}

void MainWindow::sliderStateValueToQryDB(int value)
{
    if(mScenarioIds.length()>0)
    {
        scenarioBox = mScenarioIds.at(scenarioComboBox->currentIndex());
        lineCustomGraph->clearGraphs();
        if(turnSlider->value()!=numStates)
            lineCustomGraph->xAxis->setRange(-1,turnSlider->value()+1);
        else
            lineCustomGraph->xAxis->setRange(-1,turnSlider->value());
        emit getScenarioRunValues(value,scenarioBox,dimension);

        lineGraphTitle->setText(QString(lineGraphDimensionComboBox->currentText()
                                        + " vs Time, Iteration " +
                                        QString::number(value)));
        lineCustomGraph->yAxis->setLabel(lineGraphDimensionComboBox->currentText());
        lineCustomGraph->replot();

        //csv- DB
        emit getInfluence(value);
        for(int i = 0 ; i < dimensionsLineEdit->text().toInt();++i)
        {
            emit getSalience(i,value);
            emit getPosition(i,value);
        }

        updateDBViewColumns();
    }
}

void MainWindow::scenarioComboBoxValue(int scenario)
{
    removeAllGraphs();
    clearAllLabels();

    if(scenario>=0 && mScenarioIds.length()>0)
    {
        scenarioBox = mScenarioIds.at(scenario);
        if(currentScenarioId==scenarioBox)
        {
            useHistory=true;
        }
        else
        {
            useHistory=false;
        }
        scenarioDescriptionLineEdit->setText(mScenarioDesc.at(scenario));
        if(tableType=="Database")
        {
            emit getScenarioRunValues(turnSlider->value(),scenarioBox,0);
            lineCustomGraph->replot();
            emit getStateCountfromDB();

            emit getDimforBar();
            barGraphTurnSliderChanged(barGraphTurnSlider->value());

            if (prevScenario != scenarioBox)
            {
                prevScenario=scenarioBox;
                populateInitiatorsAndReceiversRadioButtonsAndCheckBoxes();
                return;
            }
            else
            {
                quadMapTurnSliderChanged(quadMapTurnSlider->value());
            }
        }
    }
}

void MainWindow::cellSelected(QStandardItem* in)
{
    if(runButton->isEnabled())
        runButton->setEnabled(false);

    if(0==in->column())
    {
        csvAffinityModel->setHorizontalHeaderItem(in->row(),new QStandardItem(in->text()));
        csvAffinityModel->setVerticalHeaderItem(in->row(),new QStandardItem(in->text()));
    }
}

void MainWindow::xmlCellSelected(QStandardItem* in)
{
    if(runButton->isEnabled())
        runButton->setEnabled(false);

    if(0==in->column())
    {
        xmlAffinityMatrixModel->setHorizontalHeaderItem(in->row(),new QStandardItem(in->text()));
        xmlAffinityMatrixModel->setVerticalHeaderItem(in->row(),new QStandardItem(in->text()));
    }
}

void MainWindow::insertNewRowCSV()
{
    if(tableType=="NewSMPData" )
    {
        while(csvTableWidget->rowCount()!=actorsLineEdit->text().toInt()
              && actorsLineEdit->text().toInt()>csvTableWidget->rowCount()
              && affinityMatrix->rowCount() != actorsLineEdit->text().toInt()
              && actorsLineEdit->text().toInt()>affinityMatrix->rowCount())
        {
            csvTableWidget->insertRow(csvTableWidget->rowCount());

            QString actorHeader;
            affinityMatrix->insertColumn(affinityMatrix->columnCount());
            affinityMatrix->insertRow(affinityMatrix->rowCount());
            actorHeader.append(" Actor ")/*.append(QString::number(affinityMatrix->columnCount()))*/;
            affinityMatrix->setHorizontalHeaderItem(affinityMatrix->columnCount()-1,new QTableWidgetItem(actorHeader));
            affinityMatrix->setVerticalHeaderItem(affinityMatrix->rowCount()-1,new QTableWidgetItem(actorHeader));

            initializeAffinityMatrixRowCol(affinityMatrix->rowCount()-1,"NewSMPData");
        }
    }
    else if(tableType=="DatabaseEdit")
    {
        while(affinityMatrix->rowCount() != actorsLineEdit->text().toInt()
              && actorsLineEdit->text().toInt()>affinityMatrix->rowCount())
        {
            csvTableWidget->insertRow(csvTableWidget->rowCount());

            QString actorHeader;
            affinityMatrix->insertColumn(affinityMatrix->columnCount());
            affinityMatrix->insertRow(affinityMatrix->rowCount());
            actorHeader.append(" Actor ")/*.append(QString::number(affinityMatrix->columnCount()))*/;
            affinityMatrix->setHorizontalHeaderItem(affinityMatrix->columnCount()-1,new QTableWidgetItem(actorHeader));
            affinityMatrix->setVerticalHeaderItem(affinityMatrix->rowCount()-1,new QTableWidgetItem(actorHeader));

            initializeAffinityMatrixRowCol(affinityMatrix->rowCount()-1,"DatabaseEdit");
        }
    }
    else if(tableType=="CSV")
    {
        if(modeltoCSV->rowCount() < actorsLineEdit->text().toInt())
            modeltoCSV->insertRows(modeltoCSV->rowCount(),(actorsLineEdit->text().toInt() - modeltoCSV->rowCount()));

        //        int rowCount = csvAffinityModel->rowCount();
        int rowCount = csvAffinityModel->rowCount();
        if(csvAffinityModel->rowCount() < actorsLineEdit->text().toInt() ||
                csvAffinityModel->columnCount() < actorsLineEdit->text().toInt())
        {
            csvAffinityModel->insertRows(csvAffinityModel->rowCount(),(actorsLineEdit->text().toInt()
                                                                       - csvAffinityModel->rowCount()));
            csvAffinityModel->insertColumns(csvAffinityModel->columnCount(),(actorsLineEdit->text().toInt()
                                                                             - csvAffinityModel->columnCount()));
        }
        QString actorHeader;
        //        qDebug()<<rowCount <<"rowcount" << csvAffinityModel->rowCount();
        for(int rows = rowCount; rows < csvAffinityModel->rowCount();++ rows)
        {
            actorHeader.append(" Actor ").append(QString::number(rows+1));
            csvAffinityModel->setHorizontalHeaderItem(rows, new QStandardItem(actorHeader));
            csvAffinityModel->setVerticalHeaderItem(rows, new QStandardItem(actorHeader));
            initializeAffinityMatrixRowCol(rows,"CSV");
            actorHeader.clear();
        }

    }
    else if(tableType=="XML")
    {
        if(xmlSmpDataModel->rowCount() < actorsLineEdit->text().toInt())
            xmlSmpDataModel->insertRows(xmlSmpDataModel->rowCount(),(actorsLineEdit->text().toInt()
                                                                     - xmlSmpDataModel->rowCount()));

        int rowCount = xmlAffinityMatrixModel->rowCount();
        if(xmlAffinityMatrixModel->rowCount() < actorsLineEdit->text().toInt() ||
                xmlAffinityMatrixModel->columnCount() < actorsLineEdit->text().toInt())
        {
            xmlAffinityMatrixModel->insertRows(xmlAffinityMatrixModel->rowCount(),(actorsLineEdit->text().toInt()
                                                                                   - xmlAffinityMatrixModel->rowCount()));
            xmlAffinityMatrixModel->insertColumns(xmlAffinityMatrixModel->columnCount(),(actorsLineEdit->text().toInt()
                                                                                         - xmlAffinityMatrixModel->columnCount()));
        }
        QString actorHeader;
        //        qDebug()<<rowCount <<"rowcount" << xmlAffinityMatrixModel->rowCount();
        for(int rows = rowCount; rows < xmlAffinityMatrixModel->rowCount();++ rows)
        {
            actorHeader.append(" Actor ").append(QString::number(rows+1));
            xmlAffinityMatrixModel->setHorizontalHeaderItem(rows, new QStandardItem(actorHeader));
            xmlAffinityMatrixModel->setVerticalHeaderItem(rows, new QStandardItem(actorHeader));
            initializeAffinityMatrixRowCol(rows,"XML"); // NOTE : Still Working on this
            actorHeader.clear();
        }
    }
}

void MainWindow::insertNewColumnCSV()
{
    if(tableType=="NewSMPData" || tableType=="DatabaseEdit")
    {
        while(((csvTableWidget->columnCount()-3)/2)!=dimensionsLineEdit->text().toInt()
              && ((csvTableWidget->columnCount()-3)/2) <= dimensionsLineEdit->text().toInt())
        {
            if(csvTableWidget->columnCount()%2!=0)
            {
                QTableWidgetItem * pos = new QTableWidgetItem("Position");
                createSeperateColumn(pos);

                QTableWidgetItem * sal = new QTableWidgetItem("Saliance");
                createSeperateColumn(sal);
            }
            else
                return;
        }
    }
    else if(tableType=="CSV")
    {
        while(((modeltoCSV->columnCount()-3)/2)!=dimensionsLineEdit->text().toInt()
              && ((modeltoCSV->columnCount()-3)/2) <= dimensionsLineEdit->text().toInt())
        {
            if(modeltoCSV->columnCount()%2!=0)
            {
                modeltoCSV->insertColumns(modeltoCSV->columnCount(),1);
                modeltoCSV->setHorizontalHeaderItem(modeltoCSV->columnCount()-1,new QStandardItem("Position"));

                modeltoCSV->insertColumns(modeltoCSV->columnCount(),1);
                modeltoCSV->setHorizontalHeaderItem(modeltoCSV->columnCount()-1,new QStandardItem("Salience"));
            }
            else
                return;
        }
    }
}

void MainWindow::createSeperateColumn(QTableWidgetItem * hdr)
{
    csvTableWidget->insertColumn(csvTableWidget->columnCount());
    csvTableWidget->setHorizontalHeaderItem(csvTableWidget->columnCount()-1,hdr);
}

void MainWindow::donePushButtonClicked(bool bl)
{
    Q_UNUSED(bl)
    if(tableType=="CSV")
        saveTableViewToCSV();
    else if (tableType=="NewSMPData"|| tableType=="DatabaseEdit")
        saveAsDialog();
    else if(tableType=="XML")
        saveTableViewToXML();
}

void MainWindow::displayMessage(QString cls, QString message)
{
    QMessageBox::warning(0,cls,message);
}

void MainWindow::dockWindowChanged()
{
    if(true==quadMapDock->isVisible() && actorsName.length()>0)
    {
        quadMapTurnSliderChanged(quadMapTurnSlider->value());
    }
}

void MainWindow::setCSVItemModel(QStandardItemModel *model, QStringList scenarioName)
{
    savedAsXml=false;

    if (stackWidget->count()>1)
    {
        stackWidget->removeWidget(smpDataTab);
        stackWidget->removeWidget(xmlTabWidget);
    }

    for(int i=0; i <= csvTableViewTabWidget->count(); ++i)
    {
        csvTableViewTabWidget->removeTab(0);
    }

    tableType="CSV";

    csvTableView = new QTableView;
    csvTableViewTabWidget->addTab(csvTableView,"Actor Data");

    csvTableAffinityView = new QTableView;
    csvTableViewTabWidget->addTab(csvTableAffinityView, "Affinity Matrix");

    stackWidget->addWidget(csvTableViewTabWidget);

    csvTableView->setShowGrid(true);
    stackWidget->setCurrentIndex(1);
    //    csv_tableView->horizontalHeader()->viewport()->installEventFilter(this);
    //    csv_tableView->verticalHeader()->viewport()->installEventFilter(this);
    csvTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(csvTableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayMenuTableView(QPoint)));

    //Enable Editing
    csvTableView->setEditTriggers(QAbstractItemView::AllEditTriggers);

    turnSlider->hide();
    tableControlsFrame->show();

    scenarioComboBox->setEditable(true);
    turnSlider->setVisible(false);
    scenarioDescriptionLineEdit->setVisible(true);
    scenarioDescriptionLineEdit->setEnabled(true);

    actorsLineEdit->setEnabled(true);
    dimensionsLineEdit->setEnabled(true);
    actorsPushButton->setEnabled(true);
    dimensionsPushButton->setEnabled(true);
    donePushButton->setEnabled(true);

    setDefaultParameters();//Default Model Parameters

    modeltoCSV = model;
    emit getDimensionCountfromDB();

    //update: received model to widget
    csvTableView->setModel(modeltoCSV);
    csvTableView->showMaximized();
    csvTableView->resizeColumnsToContents();
    csvTableView->resizeColumnToContents(1);

    //    csv_tableView->setAlternatingRowColors(true);
    //    csv_tableView->resizeRowsToContents();
    //    for (int c = 0; c < csv_tableView->horizontalHeader()->count(); ++c)
    //        csv_tableView->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);

    connect(modeltoCSV, SIGNAL(itemChanged(QStandardItem*)),this, SLOT(cellSelected(QStandardItem*))); // disable run button

    //Enable run button, which is disabled by default
    runButton->setEnabled(true);

    //update scenario name
    mScenarioDesc.clear();
    mScenarioName.clear();
    mScenarioIds.clear();
    scenarioComboBox->clear();

    scenarioComboBox->addItem(scenarioName.at(0));
    scenarioDescriptionLineEdit->clear();
    scenarioDescriptionLineEdit->setText(scenarioName.at(1));

    actorsLineEdit->setText(QString::number(modeltoCSV->rowCount()));
    dimensionsLineEdit->setText(QString::number((modeltoCSV->columnCount()-3)/2));


    csvAffinityModel = new QStandardItemModel;
    csvTableAffinityView->setModel(csvAffinityModel);
    //Affinity Matrix

    while(csvAffinityModel->rowCount() != actorsLineEdit->text().toInt()
          && actorsLineEdit->text().toInt()>csvAffinityModel->rowCount())
    {
        int rowCount = csvAffinityModel->rowCount();
        if(csvAffinityModel->rowCount() < actorsLineEdit->text().toInt() ||
                csvAffinityModel->columnCount() < actorsLineEdit->text().toInt())
        {
            csvAffinityModel->insertRows(csvAffinityModel->rowCount(),(actorsLineEdit->text().toInt()
                                                                       - csvAffinityModel->rowCount()));
            csvAffinityModel->insertColumns(csvAffinityModel->columnCount(),(actorsLineEdit->text().toInt()
                                                                             - csvAffinityModel->columnCount()));
        }
        QString actorHeader;
        //        qDebug()<<rowCount <<"rowcount" << csvAffinityModel->rowCount();
        for(int rows = rowCount; rows < csvAffinityModel->rowCount();++ rows)
        {
            actorHeader=modeltoCSV->item(rows,0)->text();
            csvAffinityModel->setHorizontalHeaderItem(rows, new QStandardItem(actorHeader));
            csvAffinityModel->setVerticalHeaderItem(rows, new QStandardItem(actorHeader));
            initializeAffinityMatrixRowCol(rows,"CSV");
            actorHeader.clear();
        }
    }


    csvTableAffinityView->setContextMenuPolicy(Qt::CustomContextMenu);
    //    connect(csvTableAffinityView, SIGNAL(customContextMenuRequested(QPoint)),
    //            this, SLOT(displayCsvAffinityMenuTableView(QPoint)));

    csvTableAffinityView->setEditTriggers(QAbstractItemView::AllEditTriggers);

    //    connect(csvAffinityModel, SIGNAL(itemChanged(QStandardItem*)),this, SLOT(cellSelected(QStandardItem*))); // disable run button

}

void MainWindow::setDBItemModelEdit(/*QSqlTableModel *modelEdit*/)
{

    if(tableType=="Database")
    {
        if(stackWidget->count()>1) // 1 is csv_table view
        {
            stackWidget->removeWidget(smpDataTab);
            stackWidget->removeWidget(xmlTabWidget);
        }
        for(int i =0 ; i <= smpDataTab->count(); ++i)
            smpDataTab->removeTab(0);

        csvTableWidget = new QTableWidget(central);
        affinityMatrix= new QTableWidget(central);

        tableType="DatabaseEdit";

        affinityMatrix = new QTableWidget(central);
        affinityMatrix->setContextMenuPolicy(Qt::CustomContextMenu);
        //        connect(affinityMatrix, SIGNAL(customContextMenuRequested(QPoint)), this,
        //                SLOT(displayAffinityMenuTableWidget(QPoint)));

        smpDataTab->addTab(csvTableWidget,"Actor Data ");
        smpDataTab->addTab(affinityMatrix," Affinity Matrix ");

        //        csv_tableWidget->horizontalHeader()->viewport()->installEventFilter(this);
        //        csv_tableWidget->verticalHeader()->viewport()->installEventFilter(this);

        csvTableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(csvTableWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayMenuTableWidget(QPoint)));
        connect(csvTableWidget,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(disableRunButton(QTableWidgetItem*)));

        stackWidget->addWidget(smpDataTab);
        stackWidget->setCurrentIndex(1);

        removeAllGraphs();

        actorsLineEdit->setEnabled(true);
        dimensionsLineEdit->setEnabled(true);
        actorsPushButton->setEnabled(true);
        dimensionsPushButton->setEnabled(true);
        donePushButton->setEnabled(true);

        QString currentScenario = scenarioComboBox->currentText();

        mScenarioName.clear();
        mScenarioIds.clear();
        mScenarioDesc.clear();
        scenarioComboBox->clear();

        scenarioComboBox->addItem(currentScenario);
        //        scenarioDescriptionLineEdit->clear();
        //        scenarioDescriptionLineEdit->setText(mScenarioDesc.at(index));

        scenarioComboBox->setEditable(true);
        scenarioDescriptionLineEdit->setEnabled(true);
        turnSlider->setVisible(false);
        scenarioDescriptionLineEdit->setVisible(true);
        scenarioComboBox->lineEdit()->setPlaceholderText("Enter Scenario...");
        scenarioDescriptionLineEdit->setPlaceholderText("Enter Scenario Description here ... ");

        csvTableWidget->resizeColumnsToContents();

        turnSlider->hide();
        tableControlsFrame->show();
        csvTableWidget->setShowGrid(true);

        for(int row = 0 ; row < actorsName.length();++row)
            csvTableWidget->insertRow(row);
        for(int col =0; col < 3+(dimensionsLineEdit->text().toInt())*2; ++col)
            csvTableWidget->insertColumn(col);

        //Headers Label
        csvTableWidget->setHorizontalHeaderItem(0,new QTableWidgetItem("Actor"));
        csvTableWidget->setHorizontalHeaderItem(1,new QTableWidgetItem("Description"));
        csvTableWidget->setHorizontalHeaderItem(2,new QTableWidgetItem("Influence"));

        int k=0;
        for(int i=3 ; i <csvTableWidget->columnCount(); i=i+2)
        {
            QString heading = dimensionList.at(k);
            heading = checkForHeaderString(heading);

            csvTableWidget->setHorizontalHeaderItem(i ,new QTableWidgetItem(heading +" Position"));
            csvTableWidget->setHorizontalHeaderItem(i+1,new QTableWidgetItem(heading +" Salience"));
            ++k;
        }
        //Updating values
        for(int row=0 ; row < actorsName.length(); ++row)
        {
            int col=0;
            csvTableWidget->setItem(row,col,new QTableWidgetItem(actorsName.at(row)));
            csvTableWidget->setItem(row,++col,new QTableWidgetItem(actorsDescription.at(row)));
            csvTableWidget->setItem(row,++col,new QTableWidgetItem(actorsInfl.at(row)));

            if(dimensionsLineEdit->text().toInt()>=1)
            {
                csvTableWidget->setItem(row,++col,
                                        new QTableWidgetItem(QString::number((actorsPos[0].at(row).toDouble())*100)));
                csvTableWidget->setItem(row,++col,
                                        new QTableWidgetItem(QString::number((actorsSal[0].at(row).toDouble())*100)));
            }
            if(dimensionsLineEdit->text().toInt()>=2)
            {
                csvTableWidget->setItem(row,++col,
                                        new QTableWidgetItem(QString::number((actorsPos[1].at(row).toDouble())*100)));
                csvTableWidget->setItem(row,++col,
                                        new QTableWidgetItem(QString::number((actorsSal[1].at(row).toDouble())*100)));
            }
            if(dimensionsLineEdit->text().toInt()==3)
            {
                csvTableWidget->setItem(row,++col,
                                        new QTableWidgetItem(QString::number((actorsPos[2].at(row).toDouble())*100)));
                csvTableWidget->setItem(row,++col,
                                        new QTableWidgetItem(QString::number((actorsSal[2].at(row).toDouble())*100)));
            }
        }
        runButton->setEnabled(false);
    }
    else
    {
        displayMessage("Database File",
                       "Import a Database first, then Click on \n - Edit Database to Save as CSV");
    }

    //Affinity Matrix
    for(int i =0 ; i < actorsLineEdit->text().toInt(); ++i)
    {
        QString actorHeader;
        actorHeader=csvTableWidget->item(i,0)->text();
        affinityMatrix->insertColumn(affinityMatrix->columnCount());
        affinityMatrix->insertRow(affinityMatrix->rowCount());
        affinityMatrix->setHorizontalHeaderItem(affinityMatrix->columnCount()-1,new QTableWidgetItem(actorHeader));
        affinityMatrix->setVerticalHeaderItem(affinityMatrix->rowCount()-1,new QTableWidgetItem(actorHeader));

        //         initializeAffinityMatrixRowCol(affinityMatrix->rowCount()-1,"DatabaseEdit");
    }

    int index=0;
    for(int acti =0 ; acti < actorsName.length(); ++acti)
    {
        for ( int actj =0; actj < actorsName.length(); ++actj)
        {
            affinityMatrix->setItem(acti,actj,new QTableWidgetItem(actorAffinity.at(index++)));
        }
    }
    //csv_tableWidget->hideColumn(0); //hiding the scenario column

    //updating scenario combobox with scenario name
    //    QModelIndex  id = modelEdit->index(0, 0, QModelIndex());
    //    scenarioComboBox->addItem(modelEdit->data(id).toString());

    //    for(int col =0; col < modelEdit->columnCount(); ++col)
    //    {
    //        csv_tableWidget->setHorizontalHeaderItem(
    //                    col, new QTableWidgetItem(modelEdit->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString()));
    //    }

}

void MainWindow::setDBItemModel(QStandardItemModel *model)
{
    tableType="Database";

    if( stackWidget->count()>1 ) // 1 is csv_table view
    {
        stackWidget->removeWidget(smpDataTab);
        stackWidget->removeWidget(xmlTabWidget);
    }

    for(int i=0; i<=csvTableViewTabWidget->count(); ++i)
    {
        csvTableViewTabWidget->removeTab(0);
    }

    csvTableView = new QTableView;
    affinityMatrix = new QTableWidget(central);

    affinityMatrix->setContextMenuPolicy(Qt::CustomContextMenu);
    //        connect(affinityMatrix, SIGNAL(customContextMenuRequested(QPoint)), this,
    //                SLOT(displayAffinityMenuTableWidget(QPoint)));

    //Disable Editing
    csvTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    csvTableViewTabWidget->addTab(csvTableView, "Actor Data from DB");
    csvTableViewTabWidget->addTab(affinityMatrix," Affinity Matrix ");
    stackWidget->addWidget(csvTableViewTabWidget);

    stackWidget->setCurrentIndex(0);
    disconnect(csvTableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayMenuTableView(QPoint)));

    turnSlider->hide();
    tableControlsFrame->show();

    scenarioComboBox->setEditable(false);
    turnSlider->setVisible(true);
    scenarioDescriptionLineEdit->setVisible(true);
    scenarioDescriptionLineEdit->setEnabled(false);

    actorsLineEdit->setEnabled(false);
    dimensionsLineEdit->setEnabled(false);
    actorsPushButton->setEnabled(false);
    dimensionsPushButton->setEnabled(false);
    donePushButton->setEnabled(false);

    modeltoDB = model;
    emit getDimensionCountfromDB();

    csvTableView->setModel(modeltoDB);
    csvTableView->showMaximized();

    csvTableView->resizeColumnsToContents();
    csvTableView->hideColumn(0);

    //Enable run button, which is disabled by default
    runButton->setEnabled(false);

    actorsLineEdit->setText(QString::number(model->rowCount()));
    dimensionsLineEdit->setText(QString::number(dimensions+1));

    // csv-db
    for(int col = 2 ; col < (5+(dimensionsLineEdit->text().toInt())*2); ++col)
        modeltoDB->insertColumn(col);

    //Headers Label
    modeltoDB->setHeaderData(1,Qt::Horizontal,("Turn"));
    modeltoDB->setHeaderData(2,Qt::Horizontal,("Actor"));
    modeltoDB->setHeaderData(3,Qt::Horizontal,("Description"));
    modeltoDB->setHeaderData(4,Qt::Horizontal,("Influence"));

    int k=0;
    for(int i=5 ; i <modeltoDB->columnCount(); i=i+2)
    {
        QString heading = dimensionList.at(k);
        heading = checkForHeaderString(heading);

        modeltoDB->setHeaderData(i,Qt::Horizontal ,( heading +" Position" ));
        modeltoDB->setHeaderData(i+1,Qt::Horizontal,( heading +" Salience" ));
        ++k;
    }
    //to create csv like view
    emit getActorsDesc();


    emit getInfluence(turnSlider->value());
    for(int i = 0 ; i < dimensionsLineEdit->text().toInt();++i)
    {
        emit getSalience(i,turnSlider->value());
        emit getPosition(i,turnSlider->value());
    }

    updateDBViewColumns();

    //Affinity Matrix
    for(int i =0 ; i < actorsLineEdit->text().toInt(); ++i)
    {
        QString actorHeader;
        actorHeader=modeltoDB->item(i,2)->text();
        affinityMatrix->insertColumn(affinityMatrix->columnCount());
        affinityMatrix->insertRow(affinityMatrix->rowCount());
        affinityMatrix->setHorizontalHeaderItem(affinityMatrix->columnCount()-1,new QTableWidgetItem(actorHeader));
        affinityMatrix->setVerticalHeaderItem(affinityMatrix->rowCount()-1,new QTableWidgetItem(actorHeader));

        //         initializeAffinityMatrixRowCol(affinityMatrix->rowCount()-1,"DatabaseEdit");
    }

    int index=0;
    for(int acti =0 ; acti < actorsName.length(); ++acti)
    {
        for ( int actj =0; actj < actorsName.length(); ++actj)
        {
            affinityMatrix->setItem(acti,actj,new QTableWidgetItem(actorAffinity.at(index++)));
        }
    }
}

void MainWindow::createNewSMPData(bool bl)
{
    Q_UNUSED(bl)

    tableType="NewSMPData";

    clearAllGraphs();

    setDefaultParameters();//Default Model Parameters

    if( stackWidget->count()>1 ) // 1 is csv_table view
    {
        stackWidget->removeWidget(smpDataTab);
        stackWidget->removeWidget(xmlTabWidget);
    }
    smpDataTab = new QTabWidget(stackWidget);

    csvTableWidget = new QTableWidget(central); // new CSV File
    connect(csvTableWidget,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(disableRunButton(QTableWidgetItem*)));
    //    csv_tableWidget->horizontalHeader()->viewport()->installEventFilter(this);
    //    csv_tableWidget->verticalHeader()->viewport()->installEventFilter(this);
    csvTableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(csvTableWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayMenuTableWidget(QPoint)));

    affinityMatrix= new QTableWidget;
    affinityMatrix->setContextMenuPolicy(Qt::CustomContextMenu);
    //    connect(affinityMatrix, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayAffinityMenuTableWidget(QPoint)));

    smpDataTab->addTab(csvTableWidget,"Actor Data ");
    smpDataTab->addTab(affinityMatrix,"Affinity Matrix ");

    stackWidget->addWidget(smpDataTab);
    stackWidget->setCurrentIndex(1);

    //removeAllGraphs();

    actorsLineEdit->setEnabled(true);
    dimensionsLineEdit->setEnabled(true);
    actorsPushButton->setEnabled(true);
    dimensionsPushButton->setEnabled(true);
    donePushButton->setEnabled(true);

    scenarioComboBox->setEditable(true);
    turnSlider->setVisible(false);
    scenarioDescriptionLineEdit->setVisible(true);
    scenarioDescriptionLineEdit->setEnabled(true);
    scenarioComboBox->lineEdit()->setPlaceholderText("Enter Scenario...");
    scenarioDescriptionLineEdit->setPlaceholderText("Enter Scenario Description here ... ");

    turnSlider->hide();
    tableControlsFrame->show();

    //    for (int i = 0 ; i < csv_tableWidget->columnCount(); ++i)
    //        csv_tableWidget->removeColumn(i);

    csvTableWidget->setShowGrid(true);

    csvTableWidget->resizeColumnsToContents();

    //csv_tableWidget->setHorizontalHeaderLabels(QString("HEADER;HEADER;HEADER;HEADER;HEADER").split(";"));

    actorsLineEdit->clear();
    dimensionsLineEdit->clear();
    mScenarioDesc.clear();
    mScenarioName.clear();
    mScenarioIds.clear();
    scenarioComboBox->clear();
    scenarioDescriptionLineEdit->clear();

    actorsLineEdit->setText(QString::number(3));
    dimensionsLineEdit->setText(QString::number(1));

    //   csv_tableWidget->horizontalHeader()->hide();

    insertNewRowCSV();

    csvTableWidget->insertColumn(csvTableWidget->columnCount());
    csvTableWidget->setHorizontalHeaderItem(0,new QTableWidgetItem("Actor"));
    csvTableWidget->insertColumn(csvTableWidget->columnCount());
    csvTableWidget->setHorizontalHeaderItem(1,new QTableWidgetItem("Description"));
    csvTableWidget->insertColumn(csvTableWidget->columnCount());
    csvTableWidget->setHorizontalHeaderItem(2,new QTableWidgetItem("Influence"));
    insertNewColumnCSV();

    /*   affinityMatrix->insertColumn(affinityMatrix->columnCount());
    affinityMatrix->setHorizontalHeaderItem(0,new QTableWidgetItem("Actor 0"));
    affinityMatrix->insertRow(affinityMatrix->rowCount());
    affinityMatrix->setVerticalHeaderItem(0,new QTableWidgetItem("Actor 0"));

    affinityMatrix->insertColumn(affinityMatrix->columnCount());
    affinityMatrix->setHorizontalHeaderItem(1,new QTableWidgetItem("Actor 1"));
    affinityMatrix->insertRow(affinityMatrix->rowCount());
    affinityMatrix->setVerticalHeaderItem(1,new QTableWidgetItem("Actor 1"));

    affinityMatrix->insertColumn(affinityMatrix->columnCount());
    affinityMatrix->setHorizontalHeaderItem(2,new QTableWidgetItem("Actor 2"));
    affinityMatrix->insertRow(affinityMatrix->rowCount());
    affinityMatrix->setVerticalHeaderItem(2,new QTableWidgetItem("Actor 2"));*/

    runButton->setEnabled(false);

}

void MainWindow::initializeCentralViewFrame()
{
    central = new QFrame;
    central->setFrameShape(QFrame::StyledPanel);

    gLayout = new QGridLayout;
    central->setLayout(gLayout);

    tableControlsFrame = new QFrame;
    tableControlsFrame->setFrameShape(QFrame::StyledPanel);

    QGridLayout *gCLayout = new QGridLayout(tableControlsFrame);

    stackWidget = new QStackedWidget(central);

    scenarioComboBox = new QComboBox(tableControlsFrame);
    scenarioComboBox->setMaximumWidth(200);
    scenarioComboBox->setFixedWidth(150);
    gCLayout->addWidget(scenarioComboBox,0,2);
    scenarioComboBox->setEditable(true);
    connect(scenarioComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(scenarioComboBoxValue(int)));

    turnSlider = new QSlider(Qt::Horizontal,central);
    turnSlider->setTickInterval(1);
    turnSlider->setTickPosition(QSlider::TicksBothSides);
    turnSlider->setPageStep(1);
    turnSlider->setSingleStep(1);

    connect(turnSlider,SIGNAL(valueChanged(int)),this,SLOT(sliderStateValueToQryDB(int)));

    actorsPushButton = new QPushButton("Actors",tableControlsFrame);
    actorsPushButton->setMaximumWidth(120);
    actorsPushButton->setFixedWidth(90);
    gCLayout->addWidget(actorsPushButton,0,0);

    actorsLineEdit = new QLineEdit ("3",tableControlsFrame);
    actorsLineEdit->setMaximumWidth(70);
    actorsLineEdit->setFixedWidth(60);
    actorsLineEdit->setValidator( new QIntValidator(1,1000,this));
    gCLayout->addWidget(actorsLineEdit,0,1);

    dimensionsPushButton = new QPushButton("Dimensions",tableControlsFrame);
    dimensionsPushButton->setMaximumWidth(120);
    dimensionsPushButton->setFixedWidth(90);
    gCLayout->addWidget(dimensionsPushButton,1,0);

    dimensionsLineEdit = new QLineEdit ("1",tableControlsFrame);
    dimensionsLineEdit->setMaximumWidth(70);
    dimensionsLineEdit->setFixedWidth(60);
    dimensionsLineEdit->setValidator( new QIntValidator(1,9,this));
    gCLayout->addWidget(dimensionsLineEdit,1,1);

    scenarioDescriptionLineEdit = new QLineEdit(tableControlsFrame);
    //    scenarioDescriptionLineEdit->setMaximumWidth(150);
    //    scenarioDescriptionLineEdit->setFixedWidth(130);
    gCLayout->addWidget(scenarioDescriptionLineEdit,0,3);

    donePushButton = new QPushButton("Done",tableControlsFrame);
    donePushButton->setMaximumWidth(150);
    donePushButton->setFixedWidth(130);
    gCLayout->addWidget(donePushButton,1,2);

    setCentralWidget(central);

    modeltoCSV = new QStandardItemModel;
    modeltoDB = new QStandardItemModel;

    csvTableViewTabWidget = new QTabWidget(central);
    //  csvTableView = new QTableView; // CSV and DB
    //    csvTableAffinityView = new QTableView;

    //    csvTableViewTabWidget->addTab(csvTableView,"Actor Data");
    //    csvTableView->setShowGrid(true);
    //    csv_tableView->horizontalHeader()->viewport()->installEventFilter(this);
    //    csv_tableView->verticalHeader()->viewport()->installEventFilter(this);

    smpDataTab = new QTabWidget(central);
    csvTableWidget = new QTableWidget(central);
    affinityMatrix = new QTableWidget(central);

    smpDataTab->addTab(csvTableWidget,"Actor Data ");
    smpDataTab->addTab(affinityMatrix," Affinity Matrix ");

    // stackWidget->addWidget(smpDataTab);
    //    csv_tableWidget->horizontalHeader()->viewport()->installEventFilter(this);
    //    csv_tableWidget->verticalHeader()->viewport()->installEventFilter(this);
    connect(csvTableWidget,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(disableRunButton(QTableWidgetItem*)));
    xmlTabWidget = new QTabWidget(central); //Xml and Affinity Matrix

    gLayout->addWidget(tableControlsFrame);
    gLayout->addWidget(turnSlider);
    gLayout->addWidget(stackWidget);

    stackWidget->addWidget(csvTableViewTabWidget);
    stackWidget->addWidget(smpDataTab);
    stackWidget->addWidget(xmlTabWidget);

    tableControlsFrame->hide();
    turnSlider->hide();

    connect(actorsPushButton,SIGNAL(pressed()),this, SLOT(insertNewRowCSV()));
    connect(dimensionsPushButton,SIGNAL(pressed()),this, SLOT(insertNewColumnCSV()));
    connect(donePushButton,SIGNAL(clicked(bool)),this, SLOT(donePushButtonClicked(bool)));

    dimension=0;
    in=0;
    generateColors();
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About SMP "),
                       tr("SMP !! "));
}

void MainWindow::createActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QToolBar *fileToolBar = addToolBar(tr("File"));
    QList <QKeySequence> seq;

    const QIcon NewSMPDataIcon = QIcon::fromTheme("Create New Actor Data", QIcon("://images/create.png"));
    QAction *NewSMPDataAct = new QAction(NewSMPDataIcon, tr("&Create New Actor Data"), this);
    NewSMPDataAct->setShortcuts(QKeySequence::New);
    NewSMPDataAct->setStatusTip(tr("Create New Actor Data"));
    connect(NewSMPDataAct, SIGNAL(triggered(bool)), this,SLOT(createNewSMPData(bool)));
    fileMenu->addAction(NewSMPDataAct);
    fileToolBar->addAction(NewSMPDataAct);

    const QIcon csvIcon = QIcon::fromTheme("View/Modify Exisiting CSV File", QIcon("://images/editcsv.png"));
    QAction *readCsvAct = new QAction(csvIcon, tr("&View/Modify Exisiting CSV File"), this);
    readCsvAct->setShortcuts(QKeySequence::Open);
    readCsvAct->setStatusTip(tr("View/Modify Exisiting CSV File"));
    connect(readCsvAct, SIGNAL(triggered(bool)), this,SLOT(csvGetFilePAth(bool)));
    fileMenu->addAction(readCsvAct);
    fileToolBar->addAction(readCsvAct);

    seq.clear();
    seq.append(Qt::Key_X | Qt::CTRL);
    const QIcon impXmlIcon = QIcon::fromTheme("View/Modify Exisiting Xml File ", QIcon("://images/xml.png"));
    QAction *importXmlAct = new QAction(impXmlIcon, tr("&View/Modify Exisiting Xml File"), this);
    importXmlAct->setShortcuts(seq);
    importXmlAct->setStatusTip(tr("View/Modify Exisiting Xml File"));
    connect(importXmlAct, SIGNAL(triggered(bool)), this,SLOT(importXmlGetFilePath(bool)));
    fileMenu->addAction(importXmlAct);
    fileToolBar->addAction(importXmlAct);

    const QIcon saveIcon = QIcon::fromTheme("document-save", QIcon("://images/save.png"));
    QAction *saveAct = new QAction(saveIcon, tr("&Save..."), this);
    connect(saveAct, SIGNAL(triggered(bool)), this, SLOT(donePushButtonClicked(bool)));
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save"));
    fileMenu->addAction(saveAct);
    fileToolBar->addAction(saveAct);

    fileToolBar->addSeparator();
    fileMenu->addSeparator();

    seq.clear();
    seq.append(Qt::Key_I | Qt::CTRL);
    const QIcon dbIcon = QIcon::fromTheme("Import Database ", QIcon("://images/import.png"));
    QAction *importDBAct = new QAction(dbIcon, tr("&Import Database"), this);
    importDBAct->setShortcuts(seq);
    importDBAct->setStatusTip(tr("Import Database"));
    connect(importDBAct, SIGNAL(triggered(bool)),this,SLOT(dbImported(bool)));
    connect(importDBAct, SIGNAL(triggered(bool)), this,SLOT(dbGetFilePAth(bool)));
    fileMenu->addAction(importDBAct);
    fileToolBar->addAction(importDBAct);

    seq.clear();
    seq.append(Qt::Key_E | Qt::CTRL);
    const QIcon modDBIcon = QIcon::fromTheme("Edit Database", QIcon("://images/editdb.png"));
    QAction *modifyDBAct = new QAction(modDBIcon, tr("&Edit DB, Save As"), this);
    modifyDBAct->setShortcuts(seq);
    modifyDBAct->setStatusTip(tr("&Edit DB, Save As"));
    connect(modifyDBAct, SIGNAL(triggered(bool)), this,SLOT(dbEditGetFilePAth(bool)));
    fileMenu->addAction(modifyDBAct);
    fileToolBar->addAction(modifyDBAct);

    fileToolBar->addSeparator();
    fileMenu->addSeparator();

    const QIcon exitIcon = QIcon::fromTheme("Quit ", QIcon("://images/exit.png"));
    QAction *quitAct = new QAction(exitIcon, tr("&Quit"), this);
    connect(quitAct, SIGNAL(triggered(bool)), this,SLOT(close()));
    fileMenu->addAction(quitAct);
    quitAct->setShortcuts(QKeySequence::Quit);
    quitAct->setStatusTip(tr("Quit the application"));

    viewMenu = menuBar()->addMenu(tr("&View"));

    menuBar()->addSeparator();

    QMenu *optionMenu = menuBar()->addMenu(tr("&Log Options"));

    logActions = new QActionGroup(this);
    logActions->setExclusive(true);

    logDefaultAct =new QAction(tr("&Default"), this);
    logDefaultAct->setCheckable(true);
    optionMenu->addAction(logDefaultAct);
    logDefaultAct->setStatusTip(tr("Log SMP Run Data to a Default File "));
    logActions->addAction(logDefaultAct);
    logDefaultAct->setChecked(true);

    logNewAct =new QAction(tr("&Custom"), this);
    logNewAct->setCheckable(true);
    optionMenu->addAction(logNewAct);
    logNewAct->setStatusTip(tr("Log SMP Run Data into a Custom File "));
    logActions->addAction(logNewAct);

    logNoneAct =new QAction(tr("&None"), this);
    logNoneAct->setCheckable(true);
    optionMenu->addAction(logNoneAct);
    logNoneAct->setStatusTip(tr("Log nothing"));
    logActions->addAction(logNoneAct);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction *aboutAct =new QAction(tr("&About"), this);
    helpMenu->addAction(aboutAct);
    connect(aboutAct, SIGNAL(triggered(bool)),this,SLOT(about()));
    aboutAct->setStatusTip(tr("About SMPQ "));

}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::createLinePlotsDockWindows()
{
    lineGraphDock = new QDockWidget(tr("Line Plots"), this);
    lineGraphMainFrame = new QFrame(lineGraphDock);
    lineGraphGridLayout = new QGridLayout(lineGraphMainFrame);

    //draw a graph
    initializeLineGraphDock();

    lineGraphDock->setWidget(lineGraphMainFrame);
    addDockWidget(Qt::BottomDockWidgetArea, lineGraphDock);
    viewMenu->addAction(lineGraphDock->toggleViewAction());

    connect(lineGraphDock,SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),this,SLOT(dockWindowChanged()));
    lineGraphDock->setVisible(true);
}

void MainWindow::createBarPlotsDockWindows()
{
    barGraphDock = new QDockWidget(tr("Bar Plots"), this);
    barGraphMainFrame = new QFrame(barGraphDock);
    barGraphGridLayout = new QGridLayout(barGraphMainFrame);

    //initialize BarGraph layout
    initializeBarGraphDock();

    barGraphDock->setWidget(barGraphMainFrame);
    addDockWidget(Qt::BottomDockWidgetArea, barGraphDock);
    viewMenu->addAction(barGraphDock->toggleViewAction());

    connect(barGraphDock,SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),this,SLOT(dockWindowChanged()));
    barGraphDock->setVisible(true);
}

void MainWindow::createQuadMapDockWindows()
{
    quadMapDock = new QDockWidget(tr("QuadMap"), this);
    quadMapMainFrame = new QFrame(quadMapDock);
    quadMapGridLayout = new QGridLayout(quadMapMainFrame);

    //initialize QuadMap layout
    initializeQuadMapDock();

    quadMapDock->setWidget(quadMapMainFrame);
    addDockWidget(Qt::BottomDockWidgetArea, quadMapDock);
    viewMenu->addAction(quadMapDock->toggleViewAction());

    connect(quadMapDock,SIGNAL(visibilityChanged(bool)),this,SLOT(dockWindowChanged()));

    quadMapDock->setVisible(false);

}

void MainWindow::createModelParametersDockWindow()
{
    modelParametersDock = new QDockWidget(tr("Model Parameters"),this);
    initializeModelParametersDock();
}

void MainWindow::saveTableViewToCSV()
{
    savedAsXml=false;
    if(0==validateControlButtons("csv_tableView"))
    {
        QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Save File to "), QDir::homePath(),tr("CSV File (*.csv)"));
        if( !saveFilePath.endsWith(".csv") )
            saveFilePath.append(".csv");

        // to pass csvfile path to smp
        csvPath = saveFilePath;

        QFile f(saveFilePath);

        if (f.open(QFile::WriteOnly | QFile::Truncate))
        {
            QTextStream data( &f );
            QStringList strList;

            strList <<scenarioComboBox->currentText();
            strList <<scenarioDescriptionLineEdit->text().trimmed();
            strList <<QString::number(modeltoCSV->rowCount());
            strList <<QString::number((modeltoCSV->columnCount()-3)/2);

            data << strList.join(",") << ","<< "\n";
            strList.clear();

            //Appending a header
            for( int col = 0; col < modeltoCSV->columnCount(); ++col )
            {
                strList <<modeltoCSV->horizontalHeaderItem(col)->data(Qt::DisplayRole).toString();
            }
            data << strList.join(",") << "," <<"\n";

            //appending data
            for (int row=0; row<modeltoCSV->rowCount(); row++)
            {
                strList.clear();

                for (int col=0; col<modeltoCSV->columnCount(); col++)
                {
                    strList << modeltoCSV->data(modeltoCSV->index(row,col)).toString();
                }
                data << strList.join(",") + "\n";
            }
            f.close();

            runButton->setEnabled(true);
        }
    }
}

void MainWindow::saveAsDialog()
{
    QMessageBox msgBox;
    msgBox.setText("SMP ACTOR DATA !");
    msgBox.setInformativeText("How do you want to save your data?");
    QAbstractButton *csvButton = msgBox.addButton(trUtf8("CSV"), QMessageBox::YesRole);
    QAbstractButton *xmlButton = msgBox.addButton(trUtf8("XML"), QMessageBox::YesRole);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.exec();

    if(msgBox.clickedButton() == csvButton)
    {
        savedAsXml=false;
        saveTableWidgetToCSV(true);
    }
    else if(msgBox.clickedButton() ==  xmlButton)
    {
        savedAsXml=true;
        saveTableWidgetToXML(true);
    }

}

void MainWindow::saveTableWidgetToCSV(bool bl)
{
    savedAsXml=false;
    if(0==validateControlButtons("csv_tableWidget"))
    {
        QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Save File As"), QDir::homePath(),tr("CSV files (*.csv)"));

        if( !saveFilePath.endsWith(".csv") )
            saveFilePath.append(".csv");

        // to pass csvfile path to smp
        csvPath = saveFilePath;

        QFile f(saveFilePath);

        if (f.open(QFile::WriteOnly | QFile::Truncate))
        {
            QTextStream data( &f );
            QStringList strList;

            strList <<scenarioComboBox->currentText();
            strList <<scenarioDescriptionLineEdit->text();
            strList <<QString::number(csvTableWidget->rowCount());
            strList <<QString::number((csvTableWidget->columnCount()-3)/2);

            data << strList.join(",") << ","<< "\n";
            strList.clear();

            //Appending a header
            for( int col = 0; col < csvTableWidget->columnCount(); ++col )
            {
                strList << csvTableWidget->horizontalHeaderItem(col)->data(Qt::DisplayRole).toString();
            }
            data << strList.join(",") << "," <<"\n";

            for( int row = 0; row < csvTableWidget->rowCount(); ++row )
            {
                strList.clear();
                for( int column = 0; column < csvTableWidget->columnCount(); ++column )
                {
                    strList <<csvTableWidget->item(row,column)->text();
                }
                data << strList.join( "," ) << ","  << "\n";
            }
            f.close();

            runButton->setEnabled(true);
        }

    }
}

void MainWindow::saveTableWidgetToXML(bool bl)
{
    savedAsXml=true;
    if(0==validateControlButtons("csv_tableWidget"))
    {
        QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Save File As"), QDir::homePath(),tr("XML files (*.xml)"));

        if( !saveFilePath.endsWith(".xml"))
            saveFilePath.append(".xml");

        // to pass xml path to smp
        xmlPath = saveFilePath;

        QStringList parameters;
        parameters.append(xmlPath);
        parameters.append(scenarioComboBox->currentText());
        parameters.append(scenarioDescriptionLineEdit->text());
        if(!seedRand->text().isEmpty())
            parameters.append(seedRand->text());
        else
            parameters.append("0");
        parameters.append(victProbModelComboBox->currentText());
        parameters.append(pCEModelComboBox->currentText());
        parameters.append(stateTransitionsComboBox->currentText());
        parameters.append(votingRuleComboBox->currentText());
        parameters.append(bigRAdjustComboBox->currentText());
        parameters.append(bigRRangeComboBox->currentText());
        parameters.append(thirdPartyCommitComboBox->currentText());
        parameters.append(interVecBrgnComboBox->currentText());
        parameters.append(bargnModelComboBox->currentText());

        for(int i =3; i < csvTableWidget->columnCount(); i += 2)
        {
            parameters.append(csvTableWidget->horizontalHeaderItem(i)
                              ->data(Qt::DisplayRole).toString());
        }
        emit saveNewSMPDataToXMLFile(parameters,csvTableWidget,affinityMatrix);

        runButton->setEnabled(true);

    }
}

int MainWindow::validateControlButtons(QString viewName)
{
    int ret=0;

    if("csv_tableWidget"==viewName)
    {
        if(true==actorsLineEdit->text().isEmpty()
                || csvTableWidget->rowCount()!=actorsLineEdit->text().toInt())
        {
            ++ret;
            displayMessage("Actors", "Enter a valid value");
        }
    }
    else if("csv_tableView"==viewName)
    {
        if(true==actorsLineEdit->text().isEmpty()
                || modeltoCSV->rowCount()!=actorsLineEdit->text().toInt())
        {
            ++ret;
            displayMessage("Actors", "Enter a valid value");
        }
    }
    else if("xml_tableView"==viewName)
    {
        if(true==actorsLineEdit->text().isEmpty()
                || xmlSmpDataModel->rowCount()!=actorsLineEdit->text().toInt())
        {
            ++ret;
            displayMessage("Actors", "Enter a valid value");
        }
    }
    if("csv_tableWidget"==viewName)
    {
        if(true==dimensionsLineEdit->text().isEmpty()
                || csvTableWidget->columnCount()!=(dimensionsLineEdit->text().toInt()*2)+3)
        {
            ++ret;
            displayMessage("Dimensions", "Enter a valid value");
        }
    }
    else if("csv_tableView"==viewName)
    {
        if(true==dimensionsLineEdit->text().isEmpty()
                || modeltoCSV->columnCount()!=(dimensionsLineEdit->text().toInt()*2)+3)
        {
            ++ret;
            displayMessage("Dimensions", "Enter a valid value");
        }
    }
    else if("xml_tableView"==viewName)
    {
        if(true==dimensionsLineEdit->text().isEmpty()
                || xmlSmpDataModel->columnCount()!=(dimensionsLineEdit->text().toInt()*2)+3)
        {
            ++ret;
            displayMessage("Dimensions", "Enter a valid value");
        }
    }

    if(0==scenarioComboBox->count() && true==scenarioComboBox->lineEdit()->text().isEmpty())
    {
        ++ret;
        displayMessage("Scenario", "Enter Scenario name");
    }

    if(true==scenarioDescriptionLineEdit->text().isEmpty())
    {
        ++ret;
        displayMessage("Scenario Description", "Enter Scenario Description");
    }

    if("csv_tableWidget"==viewName)
    {
        for(int rowIndex = 0; rowIndex < csvTableWidget->rowCount(); ++rowIndex)
        {
            for( int colIndex =0 ; colIndex < csvTableWidget->columnCount(); ++colIndex)
            {
                QTableWidgetItem* item = csvTableWidget->item(rowIndex,colIndex);

                if( !item  || item->text().isEmpty() )
                {
                    if(colIndex!=0)
                    {
                        QTableWidgetItem* actorName = csvTableWidget->item(rowIndex,0);// actor name
                        QTableWidgetItem* columnHeaderName = csvTableWidget->horizontalHeaderItem(colIndex); //column hdr
                        QMessageBox::about(0,"No Data Entered",
                                           "Please Enter \""  +columnHeaderName->text() +
                                           "\" Data for Actor \""+ actorName->text() +
                                           "\" at Row : " + QString::number(rowIndex+1));
                    }
                    else
                    {
                        QMessageBox::about(0,"No Data Entered",
                                           "please Enter Actor Name at Row : "+
                                           QString::number(rowIndex+1));
                    }
                    return ++ret;
                }
            }
        }
    }
    else if("csv_tableView"==viewName)
    {
        for(int rowIndex = 0; rowIndex < modeltoCSV->rowCount(); ++rowIndex)
        {
            for( int colIndex =0 ; colIndex < modeltoCSV->columnCount(); ++colIndex)
            {
                if(modeltoCSV->data(modeltoCSV->index(rowIndex,colIndex)).toString().isEmpty())
                {
                    if(colIndex!=0)
                    {
                        QString actorName = modeltoCSV->data(modeltoCSV->index(rowIndex,0)).toString();// actor name
                        QString columnHeaderName = modeltoCSV->headerData(colIndex,Qt::Horizontal).toString();
                        QMessageBox::about(0,"No Data Entered",
                                           "Please Enter \""  +columnHeaderName +
                                           "\" Data for Actor \""+ actorName +
                                           "\" at Row : " + QString::number(rowIndex+1));
                    }
                    else
                    {
                        QMessageBox::about(0,"No Data Entered",
                                           "please Enter Actor Name at Row : "+
                                           QString::number(rowIndex+1));
                    }
                    return ++ret;
                }
            }
        }
    }
    else if("xml_tableView"==viewName)
    {
        for(int rowIndex = 0; rowIndex < xmlSmpDataModel->rowCount(); ++rowIndex)
        {
            for( int colIndex =0 ; colIndex < xmlSmpDataModel->columnCount(); ++colIndex)
            {
                if(xmlSmpDataModel->data(xmlSmpDataModel->index(rowIndex,colIndex)).toString().isEmpty())
                {
                    if(colIndex!=0)
                    {
                        QString actorName = xmlSmpDataModel->data(xmlSmpDataModel->index(rowIndex,0)).toString();// actor name
                        QString columnHeaderName = xmlSmpDataModel->headerData(colIndex,Qt::Horizontal).toString();
                        QMessageBox::about(0,"No Data Entered",
                                           "Please Enter \""  +columnHeaderName +
                                           "\" Data for Actor \""+ actorName +
                                           "\" at Row : " + QString::number(rowIndex+1));
                    }
                    else
                    {
                        QMessageBox::about(0,"No Data Entered",
                                           "please Enter Actor Name at Row : "+
                                           QString::number(rowIndex+1));
                    }
                    return ++ret;
                }
            }
        }
    }
    return ret;
}

void MainWindow::updateDBViewColumns()
{
    //Updating values
    for(int row=0 ; row < actorsName.length(); ++row)
    {
        int col=2;

        if(!actorsInfl.isEmpty()) // To make Sure App doesnt crash when there is no data
        {
            modeltoDB->setItem(row,col,new QStandardItem(actorsName.at(row)));
            modeltoDB->setItem(row,++col,new QStandardItem(actorsDescription.at(row)));
            modeltoDB->setItem(row,++col,new QStandardItem(actorsInfl.at(row)));

            if(dimensionsLineEdit->text().toInt()>=1)
            {
                modeltoDB->setItem(row,++col,new QStandardItem(actorsPos[0].at(row)));
                modeltoDB->setItem(row,++col,new QStandardItem(actorsSal[0].at(row)));
            }
            if(dimensionsLineEdit->text().toInt()>=2)
            {
                modeltoDB->setItem(row,++col,new QStandardItem(actorsPos[1].at(row)));
                modeltoDB->setItem(row,++col,new QStandardItem(actorsSal[1].at(row)));
            }
            if(dimensionsLineEdit->text().toInt()==3)
            {
                modeltoDB->setItem(row,++col,new QStandardItem(actorsPos[2].at(row)));
                modeltoDB->setItem(row,++col,new QStandardItem(actorsSal[2].at(row)));
            }
        }
    }
}

QString MainWindow::checkForHeaderString(QString header)
{
    if(header.contains("Position"))
        header.replace("Position","");
    else if(header.contains("position"))
        header.replace("position","");

    if(header.contains("Salience"))
        header.replace("Salience","");
    else if(header.contains("salience"))
        header.replace("salience","");

    return header.trimmed();
}

void MainWindow::displayMenuTableWidget(QPoint pos)
{
    QMenu menu(this);
    QAction *posCol = menu.addAction("Insert Position Column");
    QAction *salCol = menu.addAction("Insert Salience Column");
    menu.addSeparator();
    QAction *newRow = menu.addAction("Insert Row");
    menu.addSeparator();
    QAction *col = menu.addAction("Remove Column");
    QAction *row = menu.addAction("Remove Row");
    menu.addSeparator();
    QAction *rename = menu.addAction("Rename Column Header");
    QAction *act = menu.exec(csvTableWidget->viewport()->mapToGlobal(pos));

    if (act == col)
    {
        if(csvTableWidget->currentColumn()>2)
            csvTableWidget->removeColumn(csvTableWidget->currentColumn());
        else
            statusBar()->showMessage("No Permission ! You cannot delete Actor, Description and Influence Columns");
    }
    if (act == row)
    {
        affinityMatrix->removeRow(csvTableWidget->currentRow());
        affinityMatrix->removeColumn(csvTableWidget->currentRow());
        csvTableWidget->removeRow(csvTableWidget->currentRow());

    }
    if (act == rename)
    {
        if(csvTableWidget->currentColumn()>2)
        {
            bool ok;
            QString text = QInputDialog::getText(this, tr("Plesase Enter the Header Name"),
                                                 tr("Header Name"), QLineEdit::Normal,
                                                 csvTableWidget->horizontalHeaderItem(csvTableWidget->currentColumn())->text(), &ok);

            if (ok && !text.isEmpty())
            {
                if(csvTableWidget->currentColumn()%2!=0)
                {
                    if(!(text.contains("Position") || text.contains("position")))
                        text = "Position";
                }
                else
                {
                    if(!(text.contains("Salience")|| text.contains("salience")))
                        text = "Salience";
                }
                csvTableWidget->setHorizontalHeaderItem(csvTableWidget->currentColumn(),new QTableWidgetItem(text));
                statusBar()->showMessage("Header changed");
            }
        }
        else
            statusBar()->showMessage("No Permission !  You cannot Edit Headers of"
                                     " Actor, Description and Influence Columns");
    }

    if (act == posCol)
    {
        if(csvTableWidget->currentColumn()>2)
        {
            csvTableWidget->insertColumn(csvTableWidget->currentColumn());
            csvTableWidget->setHorizontalHeaderItem(csvTableWidget->currentColumn()-1,new QTableWidgetItem("Position"));
            statusBar()->showMessage("Position Column Inserted");
        }
        else
            statusBar()->showMessage("No Permission !  You cannot change"
                                     " Actor, Description and Influence Columns");
    }

    if (act == salCol)
    {
        if(csvTableWidget->currentColumn()>2)
        {
            csvTableWidget->insertColumn(csvTableWidget->currentColumn());
            csvTableWidget->setHorizontalHeaderItem(csvTableWidget->currentColumn()-1,new QTableWidgetItem("Salience"));
            statusBar()->showMessage("Salience Column Inserted");
        }
        else
            statusBar()->showMessage("No Permission !  You cannot change"
                                     " Actor, Description and Influence Columns");
    }

    if(act == newRow)
    {
        affinityMatrix->insertRow(csvTableWidget->currentRow());
        affinityMatrix->insertColumn(csvTableWidget->currentRow());
        affinityMatrix->setVerticalHeaderItem(csvTableWidget->currentRow(), new QTableWidgetItem("Actor"));
        affinityMatrix->setHorizontalHeaderItem(csvTableWidget->currentRow(), new QTableWidgetItem("Actor"));


        for( int col = 0; col < affinityMatrix->columnCount(); ++col)
        {
            if(csvTableWidget->currentRow()==col)
                affinityMatrix->setItem(csvTableWidget->currentRow(),col,
                                        new QTableWidgetItem("1"));
            else
                affinityMatrix->setItem(csvTableWidget->currentRow(),col,
                                        new QTableWidgetItem("0"));
        }

        for(int row = 0; row < affinityMatrix->rowCount() ; ++row)
        {
            if(csvTableWidget->currentRow()==row)
                affinityMatrix->setItem(row,csvTableWidget->currentRow(),
                                        new QTableWidgetItem("1"));
            else
                affinityMatrix->setItem(row,csvTableWidget->currentRow(),
                                        new QTableWidgetItem("0"));
        }

        csvTableWidget->insertRow(csvTableWidget->currentRow());
    }

}

void MainWindow::displayMenuTableView(QPoint pos)
{
    QMenu menu(this);
    QAction *posCol = menu.addAction("Insert Position Column");
    QAction *salCol = menu.addAction("Insert Salience Column");
    menu.addSeparator();
    QAction *newRow = menu.addAction("Insert Row");
    menu.addSeparator();
    QAction *col = menu.addAction("Remove Column");
    QAction *row = menu.addAction("Remove Row");
    menu.addSeparator();
    QAction *rename = menu.addAction("Rename Column Header");

    QAction *act = menu.exec(csvTableView->viewport()->mapToGlobal(pos));

    if (act == col)
    {
        if(csvTableView->currentIndex().column()>2)
            modeltoCSV->removeColumn(csvTableView->currentIndex().column());
        else
            statusBar()->showMessage("No Permission ! You cannot delete Actor, Description and Influence Columns");
    }
    if (act == row)
    {
        csvAffinityModel->removeColumn(csvTableView->currentIndex().row());
        csvAffinityModel->removeRow(csvTableView->currentIndex().row());
        modeltoCSV->removeRow(csvTableView->currentIndex().row());
    }
    if (act == rename)
    {
        if(csvTableView->currentIndex().column()>2)
        {
            bool ok;
            QString text = QInputDialog::getText(this, tr("Plesase Enter the Header Name"),
                                                 tr("Header Name"), QLineEdit::Normal,
                                                 modeltoCSV->headerData(csvTableView->currentIndex().column(),Qt::Horizontal).toString(), &ok);

            if (ok && !text.isEmpty())
            {
                if(csvTableView->currentIndex().column()%2!=0)
                {
                    if(!(text.contains("Position") || text.contains("position")))
                        text = "Position";
                }
                else
                {
                    if(!(text.contains("Salience")|| text.contains("salience")))
                        text = "Salience";
                }
                modeltoCSV->setHeaderData(csvTableView->currentIndex().column(),Qt::Horizontal,text);
                statusBar()->showMessage("Header changed");
            }
        }
        else
            statusBar()->showMessage("No Permission !  You cannot Edit Headers of"
                                     " Actor, Description and Influence Columns");
    }
    if (act == posCol)
    {
        if(csvTableView->currentIndex().column()>2)
        {
            modeltoCSV->insertColumn(csvTableView->currentIndex().column());
            modeltoCSV->setHeaderData(csvTableView->currentIndex().column()-1,Qt::Horizontal,"Position");
            statusBar()->showMessage("Column Inserted, Header changed");
        }
        else
            statusBar()->showMessage("No Permission !  You cannot Edit Headers of"
                                     " Actor, Description and Influence Columns");
    }
    if (act == salCol)
    {
        if(csvTableView->currentIndex().column()>2)
        {
            modeltoCSV->insertColumn(csvTableView->currentIndex().column());
            modeltoCSV->setHeaderData(csvTableView->currentIndex().column()-1,Qt::Horizontal,"Salience");
            statusBar()->showMessage("Column Inserted, Header changed");
        }
        else
            statusBar()->showMessage("No Permission !  You cannot Edit Headers of"
                                     " Actor, Description and Influence Columns");
    }

    if(act == newRow)
    {
        csvAffinityModel->insertColumn(csvTableView->currentIndex().row());
        csvAffinityModel->insertRow(csvTableView->currentIndex().row());

        QString actorHeader;
        actorHeader.append(" Actor ")/*.append(QString::number(rows+1))*/;
        csvAffinityModel->setHorizontalHeaderItem(csvTableView->currentIndex().row(),
                                                  new QStandardItem(actorHeader));
        csvAffinityModel->setVerticalHeaderItem(csvTableView->currentIndex().row(),
                                                new QStandardItem(actorHeader));
        initializeAffinityMatrixRowCol(csvTableView->currentIndex().row(),"CSV");
        actorHeader.clear();

        modeltoCSV->insertRow(csvTableView->currentIndex().row());
    }
}

//void MainWindow::displayCsvAffinityMenuTableView(QPoint pos)
//{
//    QMenu menu(this);
//    QAction *newactor = menu.addAction("Insert New Actor w.r.t row");
//    QAction *delactor = menu.addAction("Delete Actor w.r.t row");
//    menu.addSeparator();
//    QAction *rename = menu.addAction("Rename Header w.r.t row");

//    QAction *act = menu.exec(csvTableAffinityView->viewport()->mapToGlobal(pos));

//    if (act == newactor)
//    {
//        bool ok;
//        QString text = QInputDialog::getText(this, tr("Plesase Enter the Header Name"),
//                                             tr("Header Name"), QLineEdit::Normal,"Actor ", &ok);
//        csvAffinityModel->insertColumn(csvTableAffinityView->currentIndex().row());
//        csvAffinityModel->insertRow(csvTableAffinityView->currentIndex().row());

//        if(ok && !text.isEmpty())
//        {
//            csvAffinityModel->setVerticalHeaderItem(csvTableAffinityView->currentIndex().row()-1,
//                                                    new QStandardItem(text));
//            csvAffinityModel->setHorizontalHeaderItem(csvTableAffinityView->currentIndex().row()-1,
//                                                      new QStandardItem(text));
//        }
//        for( int col = 0; col < csvAffinityModel->columnCount(); ++col)
//        {
//            if(csvTableAffinityView->currentIndex().row()-1==col)
//                csvAffinityModel->setItem(csvTableAffinityView->currentIndex().row()-1,col,
//                                          new QStandardItem("1"));
//            else
//                csvAffinityModel->setItem(csvTableAffinityView->currentIndex().row()-1,col,
//                                          new QStandardItem("0"));
//        }

//        for(int row = 0; row < csvAffinityModel->rowCount() ; ++row)
//        {
//            if(csvTableAffinityView->currentIndex().row()-1==row)
//                csvAffinityModel->setItem(row,csvTableAffinityView->currentIndex().row()-1,
//                                          new QStandardItem("1"));
//            else
//                csvAffinityModel->setItem(row,csvTableAffinityView->currentIndex().row()-1,
//                                          new QStandardItem("0"));
//        }
//    }
//    if (act == delactor)
//    {
//        csvAffinityModel->removeColumn(csvTableAffinityView->currentIndex().row());
//        csvAffinityModel->removeRow(csvTableAffinityView->currentIndex().row());
//    }
//    if (act == rename)
//    {
//        bool ok;
//        QString text = QInputDialog::getText(this, tr("Plesase Enter the Header Name"),
//                                             tr("Header Name"), QLineEdit::Normal,csvAffinityModel->headerData(
//                                                 csvTableAffinityView->currentIndex().row(),Qt::Horizontal).toString(),
//                                             &ok);
//        if (ok && !text.isEmpty())
//        {
//            csvAffinityModel->setVerticalHeaderItem(csvTableAffinityView->currentIndex().row(),
//                                                    new QStandardItem(text));
//            csvAffinityModel->setHorizontalHeaderItem(csvTableAffinityView->currentIndex().row(),
//                                                      new QStandardItem(text));
//            statusBar()->showMessage("Header changed");
//        }
//    }
//}

void MainWindow::actorsNameDesc(QList <QString> actorName,QList <QString> actorDescription)
{
    actorsName.clear();
    actorsDescription.clear();
    actorsName = actorName;
    actorsDescription = actorDescription;

    numAct= actorsName.length();
    actorsQueriedCount=numAct-1;
    if(!lineGraphActorsCheckBoxList.isEmpty())
    {
        for(int i=0; i < actorsName.length();++i)
        {
            if(lineGraphActorsCheckBoxList.at(i)->text()!=actorsName.at(i))
            {
                populateLineGraphActorsList();
                return;
            }
        }
    }
    else
        populateLineGraphActorsList();

    if(!barGraphActorsCheckBoxList.isEmpty())
    {
        for(int i=0; i < actorsName.length();++i)
        {
            if(barGraphActorsCheckBoxList.at(i)->text()!=actorsName.at(i))
            {
                populateBarGraphActorsList();
                return;
            }
        }
    }
    else
        populateBarGraphActorsList();


    if(!quadMapInitiatorsRadioButtonList.isEmpty())
    {
        for(int i=0; i < actorsName.length();++i)
        {
            if(quadMapInitiatorsRadioButtonList.at(i)->text()!=actorsName.at(i)
                    || quadMapInitiatorsRadioButtonList.length() != actorsName.length())
            {
                populateInitiatorsAndReceiversRadioButtonsAndCheckBoxes();
                return;
            }
            else if ((quadMapInitiatorsRadioButtonList.at(i)->text()!=actorsName.at(i)
                      || quadMapInitiatorsRadioButtonList.length() != actorsName.length())
                     && prevScenario != scenarioBox)
            {
                prevScenario=scenarioBox;
                populateInitiatorsAndReceiversRadioButtonsAndCheckBoxes();
                return;
            }
        }
    }
    else
        populateInitiatorsAndReceiversRadioButtonsAndCheckBoxes();
}

void MainWindow::actorsInfluence(QList<QString> actorInfluence)
{
    actorsInfl=actorInfluence;
}

void MainWindow::actorsPosition(QList<QString> actorPosition, int dim)
{
    actorsPos[dim]=actorPosition;
}

void MainWindow::actorsSalience(QList<QString> actorSalience,int dim)
{
    actorsSal[dim]=actorSalience;
}

void MainWindow::actAffinity(QList<QString> actorAff, QList<int> actorI, QList<int> actorJ)
{
    actorAffinity=actorAff;
    actI = actorI;
    actJ = actorJ;
}

void MainWindow::clearAllGraphs()
{
    lineCustomGraph->clearGraphs();
    lineGraphTitle->setText(" ");
    barGraphTitle->setText(" ");

    deleteBars();

    lineCustomGraph->replot();
    barCustomGraph->replot();

    for(int index=0; index < lineActorCBList.length();++ index)
    {
        disconnect(lineActorCBList.at(index),SIGNAL(toggled(bool)),this,SLOT(lineGraphActorsCheckboxClicked(bool)));
        disconnect(barActorCBList.at(index),SIGNAL(toggled(bool)),this,SLOT(barGraphActorsCheckboxClicked(bool)));
    }

    disconnect(lineGraphDimensionComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(lineGraphDimensionChanged(int)));
    disconnect(barGraphDimensionComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(barGraphDimensionChanged(int)));
    lineGraphDimensionComboBox->clear();
    barGraphDimensionComboBox->clear();

    disconnect(lineGraphSelectAllCheckBox,SIGNAL(clicked(bool)),this,SLOT(lineGraphSelectAllActorsCheckBoxClicked(bool)));
    disconnect(barGraphSelectAllCheckBox,SIGNAL(clicked(bool)),this,SLOT(barGraphSelectAllActorsCheckBoxClicked(bool)));
    disconnect(barGraphBinWidthButton,SIGNAL(clicked(bool)),this, SLOT(barGraphBinWidthButtonClicked(bool)));

}

void MainWindow :: reconnectPlotWidgetSignals()
{
    connect(lineGraphDimensionComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(lineGraphDimensionChanged(int)));
    connect(barGraphDimensionComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(barGraphDimensionChanged(int)));
    connect(lineGraphSelectAllCheckBox,SIGNAL(clicked(bool)),this,SLOT(lineGraphSelectAllActorsCheckBoxClicked(bool)));
    connect(barGraphSelectAllCheckBox,SIGNAL(clicked(bool)),this,SLOT(barGraphSelectAllActorsCheckBoxClicked(bool)));
    connect(barGraphBinWidthButton,SIGNAL(clicked(bool)),this, SLOT(barGraphBinWidthButtonClicked(bool)));

}

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
