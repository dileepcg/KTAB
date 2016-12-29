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
    Q_UNUSED(in)
    runButton->setEnabled(false);
}

void MainWindow::insertNewRowCSV()
{
    if(tableType=="NewCSV" || tableType=="DatabaseEdit")
    {
        while(csvTableWidget->rowCount()!=actorsLineEdit->text().toInt() && actorsLineEdit->text().toInt()>=csvTableWidget->rowCount())
            csvTableWidget->insertRow(csvTableWidget->rowCount());

    }
    else if(tableType=="CSV")
    {
        if(modeltoCSV->rowCount() < actorsLineEdit->text().toInt())
            modeltoCSV->insertRows(modeltoCSV->rowCount(),(actorsLineEdit->text().toInt() - modeltoCSV->rowCount()));
    }
}

void MainWindow::insertNewColumnCSV()
{
    if(tableType=="NewCSV" || tableType=="DatabaseEdit")
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
    else if (tableType=="NewCSV"|| tableType=="DatabaseEdit")
        saveTableWidgetToCSV();
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

    //    QWidget *TitleWidgetRec=new QWidget(this);
    //    if(graph2Dock->isFloating())
    //        graph2Dock->setTitleBarWidget(TitleWidgetRec);
    //    else
    //        graph2Dock->setTitleBarWidget(0);
}

void MainWindow::setCSVItemModel(QStandardItemModel *model, QStringList scenarioName)
{
    tableType="CSV";

    stackWidget->setCurrentIndex(0);
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
}

void MainWindow::setDBItemModelEdit(/*QSqlTableModel *modelEdit*/)
{
    if(tableType=="Database")
    {
        if(stackWidget->count()>1) // 1 is csv_table view
        {
            stackWidget->removeWidget(csvTableWidget);
        }
        tableType="DatabaseEdit";

        csvTableWidget = new QTableWidget(central);
        //        csv_tableWidget->horizontalHeader()->viewport()->installEventFilter(this);
        //        csv_tableWidget->verticalHeader()->viewport()->installEventFilter(this);

        csvTableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(csvTableWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayMenuTableWidget(QPoint)));
        connect(csvTableWidget,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(disableRunButton(QTableWidgetItem*)));

        stackWidget->addWidget(csvTableWidget);
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
                csvTableWidget->setItem(row,++col,new QTableWidgetItem(actorsPos[0].at(row)));
                csvTableWidget->setItem(row,++col,new QTableWidgetItem(actorsSal[0].at(row)));
            }
            if(dimensionsLineEdit->text().toInt()>=2)
            {
                csvTableWidget->setItem(row,++col,new QTableWidgetItem(actorsPos[1].at(row)));
                csvTableWidget->setItem(row,++col,new QTableWidgetItem(actorsSal[1].at(row)));
            }
            if(dimensionsLineEdit->text().toInt()==3)
            {
                csvTableWidget->setItem(row,++col,new QTableWidgetItem(actorsPos[2].at(row)));
                csvTableWidget->setItem(row,++col,new QTableWidgetItem(actorsSal[2].at(row)));
            }
        }
        runButton->setEnabled(false);
    }
    else
    {
        displayMessage("Database File", "Import a Database first, then Click on \n - Edit Database to Save as CSV");
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

    //Disable Editing
    csvTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

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
}

void MainWindow::createNewCSV(bool bl)
{
    Q_UNUSED(bl)
    tableType="NewCSV";

    clearAllGraphs();

    if( stackWidget->count()>1 ) // 1 is csv_table view
    {
        stackWidget->removeWidget(csvTableWidget);
    }

    csvTableWidget = new QTableWidget(central); // new CSV File
    //    csv_tableWidget->horizontalHeader()->viewport()->installEventFilter(this);
    //    csv_tableWidget->verticalHeader()->viewport()->installEventFilter(this);
    csvTableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(csvTableWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayMenuTableWidget(QPoint)));


    stackWidget->addWidget(csvTableWidget);
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
    //    scenarioComboBox->setMinimumWidth(40);
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

    csvTableView = new QTableView(central); // CSV and DB
    csvTableView->setShowGrid(true);
    //    csv_tableView->horizontalHeader()->viewport()->installEventFilter(this);
    //    csv_tableView->verticalHeader()->viewport()->installEventFilter(this);

    csvTableWidget = new QTableWidget(central); // new CSV File
    //    csv_tableWidget->horizontalHeader()->viewport()->installEventFilter(this);
    //    csv_tableWidget->verticalHeader()->viewport()->installEventFilter(this);

    gLayout->addWidget(tableControlsFrame);
    gLayout->addWidget(turnSlider);
    gLayout->addWidget(stackWidget);

    stackWidget->addWidget(csvTableView);
    stackWidget->addWidget(csvTableWidget);

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

    const QIcon saveIcon = QIcon::fromTheme("document-save", QIcon("://images/save.png"));
    QAction *saveAct = new QAction(saveIcon, tr("&Save..."), this);
    connect(saveAct, SIGNAL(triggered(bool)), this, SLOT(donePushButtonClicked(bool)));
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save"));
    fileMenu->addAction(saveAct);
    fileToolBar->addAction(saveAct);

    fileMenu->addSeparator();

    const QIcon newCSVIcon = QIcon::fromTheme("Create New CSV File", QIcon("://images/csv.png"));
    QAction *newCsvAct = new QAction(newCSVIcon, tr("&Create New CSV File"), this);
    newCsvAct->setShortcuts(QKeySequence::New);
    newCsvAct->setStatusTip(tr("Create New CSV File"));
    connect(newCsvAct, SIGNAL(triggered(bool)), this,SLOT(createNewCSV(bool)));
    fileMenu->addAction(newCsvAct);
    fileToolBar->addAction(newCsvAct);

    const QIcon csvIcon = QIcon::fromTheme("View/Modify Exisiting CSV File", QIcon("://images/editcsv.png"));
    QAction *readCsvAct = new QAction(csvIcon, tr("&View/Modify Exisiting CSV File"), this);
    readCsvAct->setShortcuts(QKeySequence::Open);
    readCsvAct->setStatusTip(tr("View/Modify Exisiting CSV File"));
    connect(readCsvAct, SIGNAL(triggered(bool)), this,SLOT(csvGetFilePAth(bool)));
    fileMenu->addAction(readCsvAct);
    fileToolBar->addAction(readCsvAct);

    QList <QKeySequence> seq;
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
    QAction *modifyDBAct = new QAction(modDBIcon, tr("&Edit DB, Save as CSV"), this);
    modifyDBAct->setShortcuts(seq);
    modifyDBAct->setStatusTip(tr("&Edit DB, Save as CSV"));
    connect(modifyDBAct, SIGNAL(triggered(bool)), this,SLOT(dbEditGetFilePAth(bool)));
    fileMenu->addAction(modifyDBAct);
    fileToolBar->addAction(modifyDBAct);

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

void MainWindow::saveTableWidgetToCSV()
{
    if(0==validateControlButtons("csv_tableWidget"))
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

int MainWindow::validateControlButtons(QString viewName)
{
    int ret=0;

    if("csv_tableWidget"==viewName)
    {
        if(true==actorsLineEdit->text().isEmpty() || csvTableWidget->rowCount()!=actorsLineEdit->text().toInt())
        {
            ++ret;
            displayMessage("Actors", "Enter a valid value");
        }
    }
    else if("csv_tableView"==viewName)
    {
        if(true==actorsLineEdit->text().isEmpty() || modeltoCSV->rowCount()!=actorsLineEdit->text().toInt())
        {
            ++ret;
            displayMessage("Actors", "Enter a valid value");
        }
    }

    if("csv_tableWidget"==viewName)
    {
        if(true==dimensionsLineEdit->text().isEmpty()|| csvTableWidget->columnCount()!=(dimensionsLineEdit->text().toInt()*2)+3)
        {
            ++ret;
            displayMessage("Dimensions", "Enter a valid value");
        }
    }
    else if("csv_tableView"==viewName)
    {
        if(true==dimensionsLineEdit->text().isEmpty()|| modeltoCSV->columnCount()!=(dimensionsLineEdit->text().toInt()*2)+3)
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
        modeltoCSV->insertRow(csvTableView->currentIndex().row());
    }
}

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

    //NOTE: Changes for MultiScenario DB under progress
    //    if(!quadMapInitiatorsRadioButtonList.isEmpty())
    //    {
    //        for(int i=0; i < actorsName.length();++i)
    //        {
    //            if(quadMapInitiatorsRadioButtonList.at(i)->text()!= actorsName.at(i)
    //                    && quadMapInitiatorsRadioButtonList.length() != actorsName.length())
    //            {
    //                populateInitiatorsAndReceiversRadioButtonsAndCheckBoxes();
    //                return;
    //            }
    //            else if (prevScenario != scenarioBox)
    //            {
    //                prevScenario=scenarioBox;
    //                populateInitiatorsAndReceiversRadioButtonsAndCheckBoxes();
    //                quadMapTurnSliderChanged(quadMapTurnSlider->value());
    //                return;
    //            }
    //            else
    //            {
    //            }
    //        }
    //    }
    //    else
    //        populateInitiatorsAndReceiversRadioButtonsAndCheckBoxes();

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
