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

    createModuleParametersDockWindow();
    createGraph1DockWindows();
    createGraph2DockWindows();

    setWindowTitle(tr("SMPQ_Dock_GUI"));

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
    connect(this,SIGNAL(dbFilePath(QString)),dbObj, SLOT(openDB(QString)));
    //To open database by passing the path
    connect(this,SIGNAL(dbEditFilePath(QString)),dbObj,SLOT(openDBEdit(QString)));
    //To get Database item Model to show on GUI
    connect(dbObj,SIGNAL(dbModel(QStandardItemModel*)),this,SLOT(setDBItemModel(QStandardItemModel*)));
    //To get Database item Model to show on Edit
    //    connect(dbObj,&Database::dbModelEdit,this,&MainWindow::setDBItemModelEdit);
    //To display any message to user
    connect(dbObj,SIGNAL(Message(QString,QString)),this,SLOT(displayMessage(QString,QString)));
    //To get vector positions of actors to plot a graph on GUI
    connect(dbObj,SIGNAL(vectorPosition(QVector<double>,QVector<double>,QString))
            ,this,SLOT(addGraphOnModule1(QVector<double>,QVector<double>,QString)));
    //To get number of states to set the max values of slider
    connect(dbObj,SIGNAL(statesCount(int)),this,SLOT(updateStateCount_SliderRange(int)));
    //To get all scenarios from Database to update ScenarioComboBox
    connect(dbObj,SIGNAL(scenarios(QStringList*)),this,SLOT(updateScenarioList_ComboBox(QStringList*)));
    //To get initial scenario vector positions and Item model to update graph and Table view
    connect(this, SIGNAL(getScenarioRunValues(int,QString)),dbObj,SLOT(getScenarioData(int,QString)));
    //To store scenario value, for edit database to save as csv reference
    connect(this, SIGNAL(getScenarioRunValuesEdit(QString)),dbObj,SLOT(getScenarioDataEdit(QString)));
    //To get state count
    connect(this, SIGNAL(getStateCountfromDB()),dbObj,SLOT(getStateCount()));
    //To get dimensions count
    connect(this,SIGNAL(getDimensionCountfromDB()),dbObj,SLOT(getDimensionCount()));
    //received dimension Values
    connect(dbObj,SIGNAL(dimensionsCount(int)),this,SLOT(updateDimensionCount(int)));

    //DB to CSV
    connect(this, SIGNAL(getActorsDesc()),dbObj,SLOT(getActors_DescriptionDB()));
    connect(dbObj,SIGNAL(actorsNameDesc(QList <QString> ,QList <QString>)),this,SLOT(actorsName_Description(QList  <QString> ,QList  <QString>)));
    connect(this, SIGNAL(getInfluence(int)),dbObj,SLOT(getInfluenceDB(int)));
    connect(dbObj,SIGNAL(actorsInflu(QList<QString>)),this,SLOT(actors_Influence(QList  <QString>)));
    connect(this, SIGNAL(getPosition(int,int)),dbObj,SLOT(getPositionDB(int,int)));
    connect(dbObj,SIGNAL(actors_Pos(QList<QString>,int)),this,SLOT(actors_Position(QList<QString>,int)));
    connect(this, SIGNAL(getSalience(int,int)),dbObj,SLOT(getSalienceDB(int,int)));
    connect(dbObj,SIGNAL(actors_Sal(QList<QString>,int)),this,SLOT(actors_Salience(QList<QString>,int)));

    //editable headers of TableWidget and TableView
    header_editor = 0;

}

MainWindow::~MainWindow()
{
}

void MainWindow::csvGetFilePAth(bool bl)
{
    Q_UNUSED(bl)
    statusBar()->showMessage(tr("Looking for CSV file"));
    //Get  *.csv file path
    QString csvPath;
    csvPath = QFileDialog::getOpenFileName(this,tr("Open CSV File"), QDir::homePath() , tr("CSV File (*.csv)"));


    //emit path to csv class for processing
    if(!csvPath.isEmpty())
    {
        modeltoCSV->clear();
        emit csvFilePath(csvPath);
    }
    statusBar()->showMessage(tr(" "));

}

void MainWindow::dbGetFilePAth(bool bl)
{
    Q_UNUSED(bl)
    statusBar()->showMessage(tr("Looking for Database file ..."));
    //Get  *.db file path
    dbPath = QFileDialog::getOpenFileName(this,tr("Database File"), QDir::homePath() , tr("Database File (*.db)"));

    //emit path to db class for processing
    if(!dbPath.isEmpty())
    {
        disconnect(scenarioComboBox,SIGNAL(currentIndexChanged(QString)),this,SLOT(scenarioComboBoxValue(QString)));
        disconnect(turnSlider,SIGNAL(valueChanged(int)),this,SLOT(sliderStateValueToQryDB(int)));
        scenarioComboBox->clear();
        connect(scenarioComboBox,SIGNAL(currentIndexChanged(QString)),this,SLOT(scenarioComboBoxValue(QString)));
        connect(turnSlider,SIGNAL(valueChanged(int)),this,SLOT(sliderStateValueToQryDB(int)));

        modeltoDB->clear();
        emit dbFilePath(dbPath);

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
}

void MainWindow::updateStateCount_SliderRange(int states)
{
    turnSlider->setRange(0,states);
}

void MainWindow::updateScenarioList_ComboBox(QStringList * scenarios)
{
    for(int index=0;index<scenarios->length();++index)
        scenarioComboBox->addItem(scenarios->at(index));

    scenario_box = scenarioComboBox->currentText();
    qDebug() <<scenario_box;
    //    sliderStateValueToQryDB(0);//when new database is opened, start from zero
}

void MainWindow::updateDimensionCount(int dim)
{
    dimensions = dim;
}

void MainWindow::sliderStateValueToQryDB(int value)
{

    scenario_box = scenarioComboBox->currentText();
    removeAllGraphs();
    emit getScenarioRunValues(value,scenario_box);

    //csv- DB
    emit getInfluence(value);
    for(int i = 0 ; i < dimensionsLineEdit->text().toInt();++i)
    {
        emit getSalience(i,value);
        emit getPosition(i,value);
    }

    updateDBViewColumns();

    emit getScenarioRunValuesEdit(scenario_box);
    // to update current scenario value for editing database to csv reference
}

void MainWindow::scenarioComboBoxValue(QString scenario)
{
    removeAllGraphs();
    scenario_box = scenario;

    if(tableType=="Database")
        emit getScenarioRunValues(turnSlider->value(),scenario_box); // to keep the same state when scenario changes
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
        while(csv_tableWidget->rowCount()!=actorsLineEdit->text().toInt() && actorsLineEdit->text().toInt()>=csv_tableWidget->rowCount())
            csv_tableWidget->insertRow(csv_tableWidget->rowCount());

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
        while(((csv_tableWidget->columnCount()-3)/2)!=dimensionsLineEdit->text().toInt()
              && ((csv_tableWidget->columnCount()-3)/2) <= dimensionsLineEdit->text().toInt())
        {
            if(csv_tableWidget->columnCount()%2!=0)
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
    csv_tableWidget->insertColumn(csv_tableWidget->columnCount());
    csv_tableWidget->setHorizontalHeaderItem(csv_tableWidget->columnCount()-1,hdr);
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

void MainWindow::vectorPositionsFromDB()
{

}

void MainWindow::dockWindowChanged()
{
    QWidget *TitleWidgetRec=new QWidget(this);
    if(graph2Dock->isFloating())
        graph2Dock->setTitleBarWidget(TitleWidgetRec);
    else
        graph2Dock->setTitleBarWidget(0);
}

void MainWindow::setCSVItemModel(QStandardItemModel *model, QStringList scenarioName)
{
    tableType="CSV";

    stackWidget->setCurrentIndex(0);
    //    csv_tableView->horizontalHeader()->viewport()->installEventFilter(this);
    //    csv_tableView->verticalHeader()->viewport()->installEventFilter(this);
    csv_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(csv_tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayMenu_tableView(QPoint)));

    //Enable Editing
    csv_tableView->setEditTriggers(QAbstractItemView::AllEditTriggers);

    turnSlider->hide();
    tableControlsFrame->show();

    scenarioComboBox->setEditable(true);
    turnSlider->setVisible(false);
    scenarioDescriptionLineEdit->setVisible(true);

    actorsLineEdit->setEnabled(true);
    dimensionsLineEdit->setEnabled(true);
    actorsPushButton->setEnabled(true);
    dimensionsPushButton->setEnabled(true);
    donePushButton->setEnabled(true);

    modeltoCSV = model;

    //update: received model to widget
    csv_tableView->setModel(modeltoCSV);
    csv_tableView->showMaximized();
    csv_tableView->resizeColumnsToContents();
    csv_tableView->resizeColumnToContents(1);

    //    csv_tableView->setAlternatingRowColors(true);
    //    csv_tableView->resizeRowsToContents();
    //    for (int c = 0; c < csv_tableView->horizontalHeader()->count(); ++c)
    //        csv_tableView->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);

    connect(modeltoCSV, SIGNAL(itemChanged(QStandardItem*)),this, SLOT(cellSelected(QStandardItem*))); // disable run button

    //Enable run button, which is disabled by default
    runButton->setEnabled(true);

    //update scenario name
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
            stackWidget->removeWidget(csv_tableWidget);
        }
        tableType="DatabaseEdit";

        csv_tableWidget = new QTableWidget(central);
        //        csv_tableWidget->horizontalHeader()->viewport()->installEventFilter(this);
        //        csv_tableWidget->verticalHeader()->viewport()->installEventFilter(this);

        csv_tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(csv_tableWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayMenu_tableWidget(QPoint)));

        stackWidget->addWidget(csv_tableWidget);
        stackWidget->setCurrentIndex(1);

        removeAllGraphs();

        actorsLineEdit->setEnabled(true);
        dimensionsLineEdit->setEnabled(true);
        actorsPushButton->setEnabled(true);
        dimensionsPushButton->setEnabled(true);
        donePushButton->setEnabled(true);

        QString currentScenario = scenarioComboBox->currentText();
        scenarioComboBox->clear();
        scenarioComboBox->addItem(currentScenario);
        scenarioDescriptionLineEdit->clear();

        scenarioComboBox->setEditable(true);
        turnSlider->setVisible(false);
        scenarioDescriptionLineEdit->setVisible(true);
        scenarioComboBox->lineEdit()->setPlaceholderText("Enter Scenario...");
        scenarioDescriptionLineEdit->setPlaceholderText("Enter Scenario Description here ... ");

        csv_tableWidget->resizeColumnsToContents();

        turnSlider->hide();
        tableControlsFrame->show();
        csv_tableWidget->setShowGrid(true);

        for(int row = 0 ; row < actorsName.length();++row)
            csv_tableWidget->insertRow(row);
        for(int col =0; col < 3+(dimensionsLineEdit->text().toInt())*2; ++col)
            csv_tableWidget->insertColumn(col);

        //Headers Label
        csv_tableWidget->setHorizontalHeaderItem(0,new QTableWidgetItem("Actor"));
        csv_tableWidget->setHorizontalHeaderItem(1,new QTableWidgetItem("Description"));
        csv_tableWidget->setHorizontalHeaderItem(2,new QTableWidgetItem("Influence"));

        int k=0;
        for(int i=3 ; i <csv_tableWidget->columnCount(); i=i+2)
        {
            csv_tableWidget->setHorizontalHeaderItem(i ,new QTableWidgetItem("Position "+QString::number(k)));
            csv_tableWidget->setHorizontalHeaderItem(i+1,new QTableWidgetItem("Salience "+QString::number(k)));
            ++k;
        }
        //Updating values
        for(int row=0 ; row < actorsName.length(); ++row)
        {
            int col=0;
            csv_tableWidget->setItem(row,col,new QTableWidgetItem(actorsName.at(row)));
            csv_tableWidget->setItem(row,++col,new QTableWidgetItem(actorsDescription.at(row)));
            csv_tableWidget->setItem(row,++col,new QTableWidgetItem(actorsInfluence.at(row)));

            if(dimensionsLineEdit->text().toInt()>=1)
            {
                csv_tableWidget->setItem(row,++col,new QTableWidgetItem(actorsPosition[0].at(row)));
                csv_tableWidget->setItem(row,++col,new QTableWidgetItem(actorsSalience[0].at(row)));
            }
            if(dimensionsLineEdit->text().toInt()>=2)
            {
                csv_tableWidget->setItem(row,++col,new QTableWidgetItem(actorsPosition[1].at(row)));
                csv_tableWidget->setItem(row,++col,new QTableWidgetItem(actorsSalience[1].at(row)));
            }
            if(dimensionsLineEdit->text().toInt()==3)
            {
                csv_tableWidget->setItem(row,++col,new QTableWidgetItem(actorsPosition[2].at(row)));
                csv_tableWidget->setItem(row,++col,new QTableWidgetItem(actorsSalience[2].at(row)));
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
    csv_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    stackWidget->setCurrentIndex(0);
    disconnect(csv_tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayMenu_tableView(QPoint)));

    turnSlider->hide();
    tableControlsFrame->show();

    scenarioComboBox->setEditable(false);
    turnSlider->setVisible(true);
    scenarioDescriptionLineEdit->setVisible(false);

    actorsLineEdit->setEnabled(false);
    dimensionsLineEdit->setEnabled(false);
    actorsPushButton->setEnabled(false);
    dimensionsPushButton->setEnabled(false);
    donePushButton->setEnabled(false);

    modeltoDB = model;

    csv_tableView->setModel(modeltoDB);
    csv_tableView->showMaximized();

    csv_tableView->resizeColumnsToContents();
    csv_tableView->hideColumn(0);

    //Enable run button, which is disabled by default
    runButton->setEnabled(false);

    emit getDimensionCountfromDB();

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
        modeltoDB->setHeaderData(i,Qt::Horizontal ,("Position "+QString::number(k)));
        modeltoDB->setHeaderData(i+1,Qt::Horizontal,("Salience "+QString::number(k)));
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

    if( stackWidget->count()>1 ) // 1 is csv_table view
    {
        stackWidget->removeWidget(csv_tableWidget);
    }

    csv_tableWidget = new QTableWidget(central); // new CSV File
    //    csv_tableWidget->horizontalHeader()->viewport()->installEventFilter(this);
    //    csv_tableWidget->verticalHeader()->viewport()->installEventFilter(this);
    csv_tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(csv_tableWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayMenu_tableWidget(QPoint)));


    stackWidget->addWidget(csv_tableWidget);
    stackWidget->setCurrentIndex(1);

    removeAllGraphs();

    actorsLineEdit->setEnabled(true);
    dimensionsLineEdit->setEnabled(true);
    actorsPushButton->setEnabled(true);
    dimensionsPushButton->setEnabled(true);
    donePushButton->setEnabled(true);

    scenarioComboBox->setEditable(true);
    turnSlider->setVisible(false);
    scenarioDescriptionLineEdit->setVisible(true);
    scenarioComboBox->lineEdit()->setPlaceholderText("Enter Scenario...");
    scenarioDescriptionLineEdit->setPlaceholderText("Enter Scenario Description here ... ");

    turnSlider->hide();
    tableControlsFrame->show();

    //    for (int i = 0 ; i < csv_tableWidget->columnCount(); ++i)
    //        csv_tableWidget->removeColumn(i);

    csv_tableWidget->setShowGrid(true);

    csv_tableWidget->resizeColumnsToContents();

    //csv_tableWidget->setHorizontalHeaderLabels(QString("HEADER;HEADER;HEADER;HEADER;HEADER").split(";"));

    actorsLineEdit->clear();
    dimensionsLineEdit->clear();
    scenarioComboBox->clear();
    scenarioDescriptionLineEdit->clear();

    actorsLineEdit->setText(QString::number(3));
    dimensionsLineEdit->setText(QString::number(1));

    //   csv_tableWidget->horizontalHeader()->hide();

    insertNewRowCSV();

    csv_tableWidget->insertColumn(csv_tableWidget->columnCount());
    csv_tableWidget->setHorizontalHeaderItem(0,new QTableWidgetItem("Actor"));
    csv_tableWidget->insertColumn(csv_tableWidget->columnCount());
    csv_tableWidget->setHorizontalHeaderItem(1,new QTableWidgetItem("Description"));
    csv_tableWidget->insertColumn(csv_tableWidget->columnCount());
    csv_tableWidget->setHorizontalHeaderItem(2,new QTableWidgetItem("Influence"));
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
    connect(scenarioComboBox,SIGNAL(currentIndexChanged(QString)),this,SLOT(scenarioComboBoxValue(QString)));

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

    csv_tableView = new QTableView(central); // CSV and DB
    csv_tableView->setShowGrid(true);
    //    csv_tableView->horizontalHeader()->viewport()->installEventFilter(this);
    //    csv_tableView->verticalHeader()->viewport()->installEventFilter(this);

    csv_tableWidget = new QTableWidget(central); // new CSV File
    //    csv_tableWidget->horizontalHeader()->viewport()->installEventFilter(this);
    //    csv_tableWidget->verticalHeader()->viewport()->installEventFilter(this);

    gLayout->addWidget(tableControlsFrame);
    gLayout->addWidget(turnSlider);
    gLayout->addWidget(stackWidget);

    stackWidget->addWidget(csv_tableView);
    stackWidget->addWidget(csv_tableWidget);

    tableControlsFrame->hide();
    turnSlider->hide();

    connect(actorsPushButton,SIGNAL(pressed()),this, SLOT(insertNewRowCSV()));
    connect(dimensionsPushButton,SIGNAL(pressed()),this, SLOT(insertNewColumnCSV()));
    connect(donePushButton,SIGNAL(clicked(bool)),this, SLOT(donePushButtonClicked(bool)));
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

    const QIcon saveIcon = QIcon::fromTheme("document-save", QIcon(":/images/save.png"));
    QAction *saveAct = new QAction(saveIcon, tr("&Save..."), this);
    connect(saveAct, SIGNAL(triggered(bool)), this, SLOT(donePushButtonClicked(bool)));
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save"));
    fileMenu->addAction(saveAct);
    fileToolBar->addAction(saveAct);

    fileMenu->addSeparator();

    const QIcon newCSVIcon = QIcon::fromTheme("Create New CSV File", QIcon(":/images/csv.png"));
    QAction *newCsvAct = new QAction(newCSVIcon, tr("&Create New CSV File"), this);
    newCsvAct->setShortcuts(QKeySequence::New);
    newCsvAct->setStatusTip(tr("Create New CSV File"));
    connect(newCsvAct, SIGNAL(triggered(bool)), this,SLOT(createNewCSV(bool)));
    fileMenu->addAction(newCsvAct);
    fileToolBar->addAction(newCsvAct);

    const QIcon csvIcon = QIcon::fromTheme("View/Modify Exisiting CSV File", QIcon(":/images/csv.png"));
    QAction *readCsvAct = new QAction(csvIcon, tr("&View/Modify Exisiting CSV File"), this);
    readCsvAct->setShortcuts(QKeySequence::Open);
    readCsvAct->setStatusTip(tr("View/Modify Exisiting CSV File"));
    connect(readCsvAct, SIGNAL(triggered(bool)), this,SLOT(csvGetFilePAth(bool)));
    fileMenu->addAction(readCsvAct);
    fileToolBar->addAction(readCsvAct);

    const QIcon dbIcon = QIcon::fromTheme("Import Database ", QIcon(":/images/csv.png"));
    QAction *importDBAct = new QAction(dbIcon, tr("&Import Database"), this);
    importDBAct->setStatusTip(tr("Import Database"));
    connect(importDBAct, SIGNAL(triggered(bool)), this,SLOT(dbGetFilePAth(bool)));
    fileMenu->addAction(importDBAct);
    fileToolBar->addAction(importDBAct);

    const QIcon modDBIcon = QIcon::fromTheme("Edit Database", QIcon(":/images/csv.png"));
    QAction *modifyDBAct = new QAction(modDBIcon, tr("&Edit DB, Save as CSV"), this);
    modifyDBAct->setStatusTip(tr("&Edit DB, Save as CSV"));
    connect(modifyDBAct, SIGNAL(triggered(bool)), this,SLOT(dbEditGetFilePAth(bool)));
    fileMenu->addAction(modifyDBAct);
    fileToolBar->addAction(modifyDBAct);

    fileMenu->addSeparator();

    QAction *quitAct = new QAction(modDBIcon, tr("&Quit"), this);
    connect(quitAct, SIGNAL(triggered(bool)), this,SLOT(close()));
    fileMenu->addAction(quitAct);
    quitAct->setShortcuts(QKeySequence::Quit);
    quitAct->setStatusTip(tr("Quit the application"));

    viewMenu = menuBar()->addMenu(tr("&View"));

    menuBar()->addSeparator();

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

void MainWindow::createGraph1DockWindows()
{
    graph1Dock = new QDockWidget(tr("Graph 1"), this);
    //   graph1Dock->setAllowedAreas(Qt::LeftDockWidgetArea |Qt::RightDockWidgetArea |Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    graphWidget = new QFrame(graph1Dock);
    gridLayout = new QGridLayout(graphWidget);
    customGraph = new QCustomPlot(graphWidget);
    gridLayout->addWidget(customGraph);

    //draw a graph
    initializeGraphPlot1();
    plotGraph();

    graph1Dock->setWidget(graphWidget);
    addDockWidget(Qt::RightDockWidgetArea, graph1Dock);
    viewMenu->addAction(graph1Dock->toggleViewAction());

    connect(graph1Dock,SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),this,SLOT(dockWindowChanged()));
}

void MainWindow::createGraph2DockWindows()
{
    graph2Dock = new QDockWidget(tr("Module2"), this);
    //  graph2Dock->setAllowedAreas(Qt::LeftDockWidgetArea |Qt::RightDockWidgetArea |Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    listwidget2 = new QListWidget(graph2Dock);
    graph2Dock->setWidget(listwidget2);
    addDockWidget(Qt::BottomDockWidgetArea, graph2Dock);
    viewMenu->addAction(graph2Dock->toggleViewAction());

    connect(graph2Dock,SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),this,SLOT(dockWindowChanged()));
}

void MainWindow::createModuleParametersDockWindow()
{
    moduleParametersDock = new QDockWidget(tr(" Parameters"),this);

    moduleFrame = new QFrame(moduleParametersDock);
    VLayout  = new QGridLayout(moduleFrame);

    for (int i = 0; i < N; ++ i)
    {
        frames[i] = new QFrame(moduleParametersDock);
        frames[i]->setFrameShape(QFrame::StyledPanel);//change to flat if required
        VLayout->addWidget(frames[i]);
    }

    //RUN Button
    runButton = new QPushButton;
    runButton->setText("RUN");
    runButton->setEnabled(false);
    VLayout->addWidget(runButton);

    //Radio buttons //Section1
    QGridLayout *radioButtonLayout = new QGridLayout(frames[0]);
    rparam1 = new QRadioButton("Parameter 1");
    rparam2 = new QRadioButton("Parameter 2");
    rparam3 = new QRadioButton("Parameter 3");
    radioButtonLayout->addWidget(rparam1);
    radioButtonLayout->addWidget(rparam2);
    radioButtonLayout->addWidget(rparam3);
    frames[0]->setLayout(radioButtonLayout);

    //CheckBoxes //Section2
    QGridLayout * checkBoxLayout = new QGridLayout(frames[1]);
    cparam1 = new QCheckBox("Parameter 1");
    cparam2 = new QCheckBox("Parameter 2");
    cparam3 = new QCheckBox("Parameter 3");
    checkBoxLayout->addWidget(cparam1);
    checkBoxLayout->addWidget(cparam2);
    checkBoxLayout->addWidget(cparam3);
    frames[1]->setLayout(checkBoxLayout);

    moduleParametersDock->setWidget(moduleFrame);
    // moduleParametersDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea | Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea );
    addDockWidget(Qt::LeftDockWidgetArea, moduleParametersDock);
    viewMenu->addAction(moduleParametersDock->toggleViewAction());

}

void MainWindow::saveTableViewToCSV()
{
    if(0==validateControlButtons("csv_tableView"))
    {
        QString saveFilePath = QFileDialog::getSaveFileName(this,tr("Save File to "), QDir::homePath(),tr("CSV File (*.csv)"));
        if( !saveFilePath.endsWith(".csv") )
            saveFilePath.append(".csv");

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

        QFile f(saveFilePath);

        if (f.open(QFile::WriteOnly | QFile::Truncate))
        {
            QTextStream data( &f );
            QStringList strList;

            strList <<scenarioComboBox->currentText();
            strList <<scenarioDescriptionLineEdit->text();
            strList <<QString::number(csv_tableWidget->rowCount());
            strList <<QString::number((csv_tableWidget->columnCount()-3)/2);

            data << strList.join(",") << ","<< "\n";
            strList.clear();

            //Appending a header
            for( int col = 0; col < csv_tableWidget->columnCount(); ++col )
            {
                strList << csv_tableWidget->horizontalHeaderItem(col)->data(Qt::DisplayRole).toString();
            }
            data << strList.join(",") << "," <<"\n";

            for( int row = 0; row < csv_tableWidget->rowCount(); ++row )
            {
                strList.clear();
                for( int column = 0; column < csv_tableWidget->columnCount(); ++column )
                {
                    strList <<csv_tableWidget->item(row,column)->text();
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
        if(true==actorsLineEdit->text().isEmpty() || csv_tableWidget->rowCount()!=actorsLineEdit->text().toInt())
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

        if(true==dimensionsLineEdit->text().isEmpty()|| csv_tableWidget->columnCount()!=(dimensionsLineEdit->text().toInt()*2)+3)
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
        for(int rowIndex = 0; rowIndex < csv_tableWidget->rowCount(); ++rowIndex)
        {
            for( int colIndex =0 ; colIndex < csv_tableWidget->columnCount(); ++colIndex)
            {
                QTableWidgetItem* item = csv_tableWidget->item(rowIndex,colIndex);

                if( !item  || item->text().isEmpty() )
                {
                    if(colIndex!=0)
                    {
                        QTableWidgetItem* actorName = csv_tableWidget->item(rowIndex,0);// actor name
                        QTableWidgetItem* columnHeaderName = csv_tableWidget->horizontalHeaderItem(colIndex); //column hdr
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

        modeltoDB->setItem(row,col,new QStandardItem(actorsName.at(row)));
        modeltoDB->setItem(row,++col,new QStandardItem(actorsDescription.at(row)));
        modeltoDB->setItem(row,++col,new QStandardItem(actorsInfluence.at(row)));

        if(dimensionsLineEdit->text().toInt()>=1)
        {
            modeltoDB->setItem(row,++col,new QStandardItem(actorsPosition[0].at(row)));
            modeltoDB->setItem(row,++col,new QStandardItem(actorsSalience[0].at(row)));
        }
        if(dimensionsLineEdit->text().toInt()>=2)
        {
            modeltoDB->setItem(row,++col,new QStandardItem(actorsPosition[1].at(row)));
            modeltoDB->setItem(row,++col,new QStandardItem(actorsSalience[1].at(row)));
        }
        if(dimensionsLineEdit->text().toInt()==3)
        {
            modeltoDB->setItem(row,++col,new QStandardItem(actorsPosition[2].at(row)));
            modeltoDB->setItem(row,++col,new QStandardItem(actorsSalience[2].at(row)));
        }
    }
}

void MainWindow::initializeGraphPlot1()
{
    customGraph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes |
                                 QCP::iSelectLegend | QCP::iSelectPlottables);
    customGraph->xAxis->setRange(0, 10);
    customGraph->yAxis->setRange(0,100);
    customGraph->axisRect()->setupFullAxesBox();

    customGraph->plotLayout()->insertRow(0);
    customGraph->plotLayout()->addElement(0, 0, new QCPPlotTitle(customGraph, "Competitive vs Time "));

    customGraph->xAxis->setLabel("Time");
    customGraph->yAxis->setLabel("Competitive");
    customGraph->legend->setVisible(true);

    QFont legendFont = font();
    legendFont.setPointSize(10);
    customGraph->legend->setFont(legendFont);
    customGraph->legend->setSelectedFont(legendFont);
    customGraph->legend->setSelectableParts(QCPLegend::spItems); // legend box shall not be selectable, only legend items

    connect(customGraph, SIGNAL(legendClick(QCPLegend*,QCPAbstractLegendItem*,QMouseEvent*)), this, SLOT(selectionChanged()));
    // connect slot that ties some axis selections together (especially opposite axes):
    connect(customGraph, SIGNAL(selectionChangedByUser()), this, SLOT(selectionChanged()));
    // connect slots that takes care that when an axis is selected, only that direction can be dragged and zoomed:
    connect(customGraph, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePress()));
    connect(customGraph, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(mouseWheel()));

    // make bottom and left axes transfer their ranges to top and right axes:
    connect(customGraph->xAxis, SIGNAL(rangeChanged(QCPRange)), customGraph->xAxis2, SLOT(setRange(QCPRange)));
    connect(customGraph->yAxis, SIGNAL(rangeChanged(QCPRange)), customGraph->yAxis2, SLOT(setRange(QCPRange)));

    // connect some interaction slots:
    connect(customGraph, SIGNAL(titleDoubleClick(QMouseEvent*,QCPPlotTitle*)), this, SLOT(titleDoubleClick(QMouseEvent*,QCPPlotTitle*)));
    connect(customGraph, SIGNAL(axisDoubleClick(QCPAxis*,QCPAxis::SelectablePart,QMouseEvent*)), this, SLOT(axisLabelDoubleClick(QCPAxis*,QCPAxis::SelectablePart)));
    connect(customGraph, SIGNAL(legendDoubleClick(QCPLegend*,QCPAbstractLegendItem*,QMouseEvent*)), this, SLOT(legendDoubleClick(QCPLegend*,QCPAbstractLegendItem*)));

    // connect slot that shows a message in the status bar when a graph is clicked:
    connect(customGraph, SIGNAL(plottableClick(QCPAbstractPlottable*,QMouseEvent*)), this, SLOT(graphClicked(QCPAbstractPlottable*)));

    // setup policy and connect slot for context menu popup:
    customGraph->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(customGraph, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequest(QPoint)));
}

void MainWindow::plotGraph()
{
    //    addGraphOnModule1(0,0);
    //    addGraphOnModule1(1,0);
    //    addGraphOnModule1(2,0);
    //    addGraphOnModule1(3,0);
    //    addGraphOnModule1(4,0);
    //    addGraphOnModule1(5,0);
    //    addGraphOnModule1(6,0);
    //    addGraphOnModule1(7,0);
}

//bool MainWindow::eventFilter(QObject* object, QEvent* event)
//{
//    if ((object == csv_tableView->horizontalHeader()->viewport() ||
//         object == csv_tableView->verticalHeader()->viewport()) &&
//            event->type() == QEvent::MouseButtonDblClick)
//    {
//        if (header_editor)
//        { //delete previous editor just in case
//            header_editor->deleteLater();
//            header_editor = 0;
//        }
//        QMouseEvent* e = static_cast<QMouseEvent*>(event);
//        QHeaderView* header = static_cast<QHeaderView*>(object->parent());
//        int mouse_pos = header->orientation() == Qt::Horizontal ? e->x() : e->y();
//        int logical_index = header->logicalIndex(header->visualIndexAt(mouse_pos));

//        if (logical_index >= 3)
//        {
//            // if mouse is over an item
//            QRect rect; // line edit rect in header's viewport's coordinates

//            if (header->orientation() == Qt::Horizontal)
//            {
//                rect.setLeft(header->sectionPosition(logical_index));
//                rect.setWidth(header->sectionSize(logical_index));
//                rect.setTop(0);
//                rect.setHeight(header->height());
//            }
//            else
//            {
//                rect.setTop(header->sectionPosition(logical_index));
//                rect.setHeight(header->sectionSize(logical_index));
//                rect.setLeft(0);
//                rect.setWidth(header->width());
//            }
//            rect.adjust(1, 1, -1, -1);

//            header_editor = new QLineEdit(header->viewport());
//            header_editor->move(rect.topLeft());
//            header_editor->resize(rect.size());
//            header_editor->setFrame(false);//get current item text

//            QString text = header->model()->headerData(logical_index, header->orientation()).toString();

//            header_editor->setText(text);
//            header_editor->setFocus();
//            editor_index = logical_index; //save for future use
//            header_editor->installEventFilter(this); //catch focus out event

//            //if user presses Enter it should close editor
//            connect(header_editor, SIGNAL(returnPressed()),csv_tableView, SLOT(setFocus()));
//            header_editor->show();
//        }
//        return true; // filter out event
//    }
//    else if ((object == csv_tableWidget->horizontalHeader()->viewport() ||
//              object == csv_tableWidget->verticalHeader()->viewport()) &&
//             event->type() == QEvent::MouseButtonDblClick)
//    {
//        if (header_editor)
//        { //delete previous editor just in case
//            header_editor->deleteLater();
//            header_editor = 0;
//        }
//        QMouseEvent* e = static_cast<QMouseEvent*>(event);
//        QHeaderView* header = static_cast<QHeaderView*>(object->parent());
//        int mouse_pos = header->orientation() == Qt::Horizontal ? e->x() : e->y();
//        int logical_index = header->logicalIndex(header->visualIndexAt(mouse_pos));

//        if (logical_index >= 3)
//        {
//            // if mouse is over an item
//            QRect rect; // line edit rect in header's viewport's coordinates

//            if (header->orientation() == Qt::Horizontal)
//            {
//                rect.setLeft(header->sectionPosition(logical_index));
//                rect.setWidth(header->sectionSize(logical_index));
//                rect.setTop(0);
//                rect.setHeight(header->height());
//            }
//            else
//            {
//                rect.setTop(header->sectionPosition(logical_index));
//                rect.setHeight(header->sectionSize(logical_index));
//                rect.setLeft(0);
//                rect.setWidth(header->width());
//            }
//            rect.adjust(1, 1, -1, -1);

//            header_editor = new QLineEdit(header->viewport());
//            header_editor->move(rect.topLeft());
//            header_editor->resize(rect.size());
//            header_editor->setFrame(false);//get current item text

//            QString text = header->model()->headerData(logical_index, header->orientation()).toString();

//            header_editor->setText(text);
//            header_editor->setFocus();
//            editor_index = logical_index; //save for future use
//            header_editor->installEventFilter(this); //catch focus out event

//            //if user presses Enter it should close editor
//            connect(header_editor, SIGNAL(returnPressed()),csv_tableWidget, SLOT(setFocus()));
//            header_editor->show();
//        }
//        return true; // filter out event
//    }
//    else if (object == header_editor && event->type() == QEvent::FocusOut)
//    {
//        QHeaderView* header = static_cast<QHeaderView*>(header_editor->parentWidget()->parentWidget());

//        //save item text
//        header->model()->setHeaderData(editor_index, header->orientation(),header_editor->text());
//        header_editor->deleteLater(); //safely delete editor
//        header_editor = 0;
//    }
//    return false;
//}

void MainWindow::displayMenu_tableWidget(QPoint pos)
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
    QAction *act = menu.exec(csv_tableWidget->viewport()->mapToGlobal(pos));

    if (act == col)
    {
        if(csv_tableWidget->currentColumn()>2)
            csv_tableWidget->removeColumn(csv_tableWidget->currentColumn());
        else
            statusBar()->showMessage("No Permission ! You cannot delete Actor, Description and Influence Columns");
    }
    if (act == row)
    {
        csv_tableWidget->removeRow(csv_tableWidget->currentRow());
    }
    if (act == rename)
    {
        if(csv_tableWidget->currentColumn()>2)
        {
            bool ok;
            QString text = QInputDialog::getText(this, tr("Plesase Enter the Header Name"),
                                                 tr("Header Name"), QLineEdit::Normal,
                                                 csv_tableWidget->horizontalHeaderItem(csv_tableWidget->currentColumn())->text(), &ok);

            if (ok && !text.isEmpty())
            {
                if(csv_tableWidget->currentColumn()%2!=0)
                {
                    if(!(text.contains("Position") || text.contains("position")))
                        text = "Position";
                }
                else
                {
                    if(!(text.contains("Salience")|| text.contains("salience")))
                        text = "Salience";
                }
                csv_tableWidget->setHorizontalHeaderItem(csv_tableWidget->currentColumn(),new QTableWidgetItem(text));
                statusBar()->showMessage("Header changed");
            }
        }
        else
            statusBar()->showMessage("No Permission !  You cannot Edit Headers of"
                                     " Actor, Description and Influence Columns");
    }

    if (act == posCol)
    {
        if(csv_tableWidget->currentColumn()>2)
        {
            csv_tableWidget->insertColumn(csv_tableWidget->currentColumn());
            csv_tableWidget->setHorizontalHeaderItem(csv_tableWidget->currentColumn()-1,new QTableWidgetItem("Position"));
            statusBar()->showMessage("Position Column Inserted");
        }
        else
            statusBar()->showMessage("No Permission !  You cannot change"
                                     " Actor, Description and Influence Columns");
    }

    if (act == salCol)
    {
        if(csv_tableWidget->currentColumn()>2)
        {
            csv_tableWidget->insertColumn(csv_tableWidget->currentColumn());
            csv_tableWidget->setHorizontalHeaderItem(csv_tableWidget->currentColumn()-1,new QTableWidgetItem("Salience"));
            statusBar()->showMessage("Salience Column Inserted");
        }
        else
            statusBar()->showMessage("No Permission !  You cannot change"
                                     " Actor, Description and Influence Columns");
    }

    if(act == newRow)
    {
        csv_tableWidget->insertRow(csv_tableWidget->currentRow());
    }

}

void MainWindow::displayMenu_tableView(QPoint pos)
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

    QAction *act = menu.exec(csv_tableView->viewport()->mapToGlobal(pos));

    if (act == col)
    {
        if(csv_tableView->currentIndex().column()>2)
            modeltoCSV->removeColumn(csv_tableView->currentIndex().column());
        else
            statusBar()->showMessage("No Permission ! You cannot delete Actor, Description and Influence Columns");
    }
    if (act == row)
    {
        modeltoCSV->removeRow(csv_tableView->currentIndex().row());
    }
    if (act == rename)
    {
        if(csv_tableView->currentIndex().column()>2)
        {
            bool ok;
            QString text = QInputDialog::getText(this, tr("Plesase Enter the Header Name"),
                                                 tr("Header Name"), QLineEdit::Normal,
                                                 modeltoCSV->headerData(csv_tableView->currentIndex().column(),Qt::Horizontal).toString(), &ok);

            if (ok && !text.isEmpty())
            {
                if(csv_tableView->currentIndex().column()%2!=0)
                {
                    if(!(text.contains("Position") || text.contains("position")))
                        text = "Position";
                }
                else
                {
                    if(!(text.contains("Salience")|| text.contains("salience")))
                        text = "Salience";
                }
                modeltoCSV->setHeaderData(csv_tableView->currentIndex().column(),Qt::Horizontal,text);
                statusBar()->showMessage("Header changed");
            }
        }
        else
            statusBar()->showMessage("No Permission !  You cannot Edit Headers of"
                                     " Actor, Description and Influence Columns");
    }
    if (act == posCol)
    {
        if(csv_tableView->currentIndex().column()>2)
        {
            modeltoCSV->insertColumn(csv_tableView->currentIndex().column());
            modeltoCSV->setHeaderData(csv_tableView->currentIndex().column()-1,Qt::Horizontal,"Position");
            statusBar()->showMessage("Column Inserted, Header changed");
        }
        else
            statusBar()->showMessage("No Permission !  You cannot Edit Headers of"
                                     " Actor, Description and Influence Columns");
    }

    if (act == salCol)
    {
        if(csv_tableView->currentIndex().column()>2)
        {

            modeltoCSV->insertColumn(csv_tableView->currentIndex().column());
            modeltoCSV->setHeaderData(csv_tableView->currentIndex().column()-1,Qt::Horizontal,"Salience");
            statusBar()->showMessage("Column Inserted, Header changed");
        }
        else
            statusBar()->showMessage("No Permission !  You cannot Edit Headers of"
                                     " Actor, Description and Influence Columns");
    }

    if(act == newRow)
    {
        modeltoCSV->insertRow(csv_tableView->currentIndex().row());
    }

}

void MainWindow::actorsName_Description(QList <QString> actorName,QList <QString> actorDescription)
{
    actorsName.clear();
    actorsDescription.clear();
    actorsName = actorName;
    actorsDescription = actorDescription;
    //    qDebug()<<actorsName.at(0) <<actorsName.count();
}

void MainWindow::actors_Influence(QList<QString> actorInfluence)
{
    actorsInfluence=actorInfluence;
    qDebug()<<actorsInfluence.count() <<"actorsInfluence";
}

void MainWindow::actors_Position(QList<QString> actorPosition, int dim)
{
    actorsPosition[dim]=actorPosition;
    //    qDebug()<<actorsPosition[dim].at(1);
}

void MainWindow::actors_Salience(QList<QString> actorSalience,int dim)
{
    actorsSalience[dim]=actorSalience;
    //    qDebug()<<actorsSalience[dim].at(0);
}

void MainWindow::titleDoubleClick(QMouseEvent* event, QCPPlotTitle* title)
{
    Q_UNUSED(event)
    // Set the plot title by double clicking on it
    bool ok;
    QString newTitle = QInputDialog::getText(this, "Title", "New plot title:", QLineEdit::Normal, title->text(), &ok);
    if (ok)
    {
        title->setText(newTitle);
        customGraph->replot();
    }
}

void MainWindow::axisLabelDoubleClick(QCPAxis *axis, QCPAxis::SelectablePart part)
{
    // Set an axis label by double clicking on it
    if (part == QCPAxis::spAxisLabel) // only react when the actual axis label is clicked, not tick label or axis backbone
    {
        bool ok;
        QString newLabel = QInputDialog::getText(this, "Title", "New axis label:", QLineEdit::Normal, axis->label(), &ok);
        if (ok)
        {
            axis->setLabel(newLabel);
            customGraph->replot();
        }
    }
}

void MainWindow::legendDoubleClick(QCPLegend *legend, QCPAbstractLegendItem *item)
{
    // Rename a graph by double clicking on its legend item

    Q_UNUSED(legend)
    if (item) // only react if item was clicked (user could have clicked on border padding of legend where there is no item, then item is 0)
    {
        QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(item);
        bool ok;
        QString newName = QInputDialog::getText(this, "Title","New graph name:", QLineEdit::Normal, plItem->plottable()->name(), &ok);
        if (ok)
        {
            plItem->plottable()->setName(newName);
            customGraph->replot();
        }
    }
}

void MainWindow::selectionChanged()
{
    // make top and bottom axes be selected synchronously, and handle axis and tick labels as one selectable object:
    if (customGraph->xAxis->selectedParts().testFlag(QCPAxis::spAxis) || customGraph->xAxis->selectedParts().testFlag(QCPAxis::spTickLabels) ||
            customGraph->xAxis2->selectedParts().testFlag(QCPAxis::spAxis) || customGraph->xAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
    {
        customGraph->xAxis2->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
        customGraph->xAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
    }
    // make left and right axes be selected synchronously, and handle axis and tick labels as one selectable object:
    if (customGraph->yAxis->selectedParts().testFlag(QCPAxis::spAxis) || customGraph->yAxis->selectedParts().testFlag(QCPAxis::spTickLabels) ||
            customGraph->yAxis2->selectedParts().testFlag(QCPAxis::spAxis) || customGraph->yAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
    {
        customGraph->yAxis2->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
        customGraph->yAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
    }

    // synchronize selection of graphs with selection of corresponding legend items:
    for (int i=0; i<customGraph->graphCount(); ++i)
    {
        QCPGraph *graph = customGraph->graph(i);
        QCPPlottableLegendItem *item = customGraph->legend->itemWithPlottable(graph);
        if (item->selected() || graph->selected())
        {
            item->setSelected(true);
            graph->setSelected(true);
        }
    }
}

void MainWindow::mousePress()
{
    // if an axis is selected, only allow the direction of that axis to be dragged
    // if no axis is selected, both directions may be dragged

    if (customGraph->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
        customGraph->axisRect()->setRangeDrag(customGraph->xAxis->orientation());
    else if (customGraph->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
        customGraph->axisRect()->setRangeDrag(customGraph->yAxis->orientation());
    else
        customGraph->axisRect()->setRangeDrag(Qt::Horizontal|Qt::Vertical);
}

void MainWindow::mouseWheel()
{
    // if an axis is selected, only allow the direction of that axis to be zoomed
    // if no axis is selected, both directions may be zoomed

    if (customGraph->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
        customGraph->axisRect()->setRangeZoom(customGraph->xAxis->orientation());
    else if (customGraph->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
        customGraph->axisRect()->setRangeZoom(customGraph->yAxis->orientation());
    else
        customGraph->axisRect()->setRangeZoom(Qt::Horizontal|Qt::Vertical);
}

void MainWindow::addGraphOnModule1(const QVector<double> &x, const QVector<double> &y,QString Actor)
{
    {//    QSqlQuery qry;
        //    QString query= QString("select * from VectorPosition where Act_i='%1' and Dim_k='%2' ").arg(act).arg(dim);

        //    int n=0;
        //    qry.exec(query);

        //    while(qry.next())
        //    {
        //        n++;
        //        qDebug()<<"n" << n;
        //    }

        //    QVector<double> x(n), y(n);
        //    int actornumber=0;
        //    int i =0;
        //    query= QString("select * from VectorPosition where Act_i='%1' and Dim_k='%2' ").arg(act).arg(dim);
        //    qry.exec(query);
        //    while(qry.next())
        //    {
        //        qDebug()<<qry.value(1).toDouble() << "    " <<qry.value(4).toDouble();
        //        x[i]=qry.value(1).toDouble();
        //        y[i]=qry.value(4).toDouble()*100;
        //        actornumber=qry.value(2).toInt();
        //        ++i;
        //    }

        //    customGraph->addGraph();
        //    customGraph->graph()->setPen(QPen(Qt::blue));
        //    customGraph->graph()->setBrush(QBrush(QColor(0, 0, 255, 20)));
        //    customGraph->addGraph();
        //    customGraph->graph()->setPen(QPen(Qt::red));
        //    QVector<double> x(500), y0(500), y1(500);

        //    for (int i=0; i<n; ++i)
        //    {
        //        //     x[i] = (i/n-0.5)*10;
        //        //     y[i] = qExp(-x[i]*x[i]*0.25)*qSin(x[i]*5)*5;
        //        //      y[i] = qExp(-x[i]*x[i]*0.25)*5;
        //    }
        //    customGraph->graph(0)->setData(x, y);
        //    //  customGraph->graph(1)->setData(x, y1);
        //    customGraph->axisRect()->setupFullAxesBox(true);
        //    customGraph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

        //    QVector<double> x(500), y0(500), y1(500);


        //    for (int i=0; i<n; ++i)
        //    {
        //      x[i] = (i/n-1-0.5)*10;
        //      y[i] = qExp(-x[i]*x[i]*0.25)*qSin(x[i]*5)*5;
        //    }
        //    customGraph->graph()->setData(x, y);
        //   // customGraph->graph(1)->setData(x, y);
        //    customGraph->axisRect()->setupFullAxesBox(true);
        //    customGraph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

        //        int n = 50; // number of points in graph
        //    double xScale = (rand()/(double)RAND_MAX + 0.5)*2;
        //    double yScale = (rand()/(double)RAND_MAX + 0.5)*2;
        //    double xOffset = (rand()/(double)RAND_MAX - 0.5)*4;
        //    double yOffset = (rand()/(double)RAND_MAX - 0.5)*5;
        //    double r1 = (rand()/(double)RAND_MAX - 0.5)*2;
        //    double r2 = (rand()/(double)RAND_MAX - 0.5)*2;
        //    double r3 = (rand()/(double)RAND_MAX - 0.5)*2;
        //    double r4 = (rand()/(double)RAND_MAX - 0.5)*2;
        //        QVector<double> x(n), y(n);

        //        for (int i=0; i<n; i++)
        //        {
        //            x[i] = (i/(double)n-0.5)*10.0*xScale + xOffset;
        //            y[i] = (qSin(x[i]*r1*5)*qSin(qCos(x[i]*r2)*r4*3)+r3*qCos(qSin(x[i])*r4*2))*yScale + yOffset;
        //        }
    }

    customGraph->addGraph();
    customGraph->graph()->setName(Actor);
    customGraph->graph()->setData(x, y);
    customGraph->graph()->setLineStyle(((QCPGraph::LineStyle)(1)));//upto 5

    //  if (rand()%100 > 50)
    customGraph->graph()->setScatterStyle(QCPScatterStyle((QCPScatterStyle::ScatterShape)(9)));

    QPen graphPen;
    graphPen.setColor(QColor(rand()%245+10, rand()%245+10, rand()%245+10));
    graphPen.setWidthF(rand()/(double)RAND_MAX*2+1);

    customGraph->graph()->setPen(graphPen);
    customGraph->replot();

}

void MainWindow::removeSelectedGraph()
{
    if (customGraph->selectedGraphs().size() > 0)
    {
        customGraph->removeGraph(customGraph->selectedGraphs().first());
        customGraph->replot();
    }
}

void MainWindow::removeAllGraphs()
{
    customGraph->clearGraphs();
    customGraph->replot();
}

void MainWindow::contextMenuRequest(QPoint pos)
{
    QMenu *menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    if (customGraph->legend->selectTest(pos, false) >= 0) // context menu on legend requested
    {
        menu->addAction("Move to top left", this, SLOT(moveLegend()))->setData((int)(Qt::AlignTop|Qt::AlignLeft));
        menu->addAction("Move to top center", this, SLOT(moveLegend()))->setData((int)(Qt::AlignTop|Qt::AlignHCenter));
        menu->addAction("Move to top right", this, SLOT(moveLegend()))->setData((int)(Qt::AlignTop|Qt::AlignRight));
        menu->addAction("Move to bottom right", this, SLOT(moveLegend()))->setData((int)(Qt::AlignBottom|Qt::AlignRight));
        menu->addAction("Move to bottom left", this, SLOT(moveLegend()))->setData((int)(Qt::AlignBottom|Qt::AlignLeft));
    }
    else  // general context menu on graphs requested
    {
        menu->addAction("Add random graph", this, SLOT(addGraphOnModule1()));
        if (customGraph->selectedGraphs().size() > 0)
            menu->addAction("Remove selected graph", this, SLOT(removeSelectedGraph()));
        if (customGraph->graphCount() > 0)
            menu->addAction("Remove all graphs", this, SLOT(removeAllGraphs()));
    }

    menu->popup(customGraph->mapToGlobal(pos));
}

void MainWindow::moveLegend()
{
    if (QAction * contextAction = qobject_cast<QAction*>(sender())) // make sure this slot is really called by a context menu action, so it carries the data we need
    {
        bool ok;
        int dataInt = contextAction->data().toInt(&ok);
        if (ok)
        {
            customGraph->axisRect()->insetLayout()->setInsetAlignment(0, (Qt::Alignment)dataInt);
            customGraph->replot();
        }
    }
}

void MainWindow::graphClicked(QCPAbstractPlottable *plottable)
{
    //statusBar->showMessage(QString("Clicked on graph '%1'.").arg(plottable->name()), 1000);
}

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
