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
#include "cmath"

//lineGraph - Graph 1 / Dock 1

void MainWindow::initializeLineGraphDock()
{
  graphTypeStackedWidget = new QStackedWidget(lineGraphDock);

  lineCustomGraph = new QCustomPlot;
  graphTypeStackedWidget->addWidget(lineCustomGraph);

  lineGraphGridLayout->addWidget(graphTypeStackedWidget,1,0);

  initializeLineGraphPlot();

  lineGraphControlsFrame = new QFrame;
  lineGraphControlsFrame->setFrameShape(QFrame::StyledPanel);

  QVBoxLayout *lineControlsVerticalLayout = new QVBoxLayout(lineGraphControlsFrame);

  QFont  labelFont;
  labelFont.setBold(true);

  QLabel * actorsLabel = new QLabel("Actors");
  actorsLabel->setAlignment(Qt::AlignHCenter);
  actorsLabel->setFont(labelFont);
  actorsLabel->setFrameStyle(QFrame::Panel | QFrame::StyledPanel);
  lineControlsVerticalLayout->addWidget(actorsLabel);

  lineGraphActorsScrollArea = new QScrollArea(lineGraphControlsFrame);

  lineControlsVerticalLayout->addWidget(lineGraphActorsScrollArea);

  lineGraphSelectAllCheckBox = new QCheckBox("Select All Actors");
  lineGraphSelectAllCheckBox->setChecked(true);
  lineControlsVerticalLayout->addWidget(lineGraphSelectAllCheckBox);

  connect(lineGraphSelectAllCheckBox,SIGNAL(clicked(bool)),this,SLOT(lineGraphSelectAllActorsCheckBoxClicked(bool)));

  QLabel * lineGraphTypeLabel = new QLabel("Graph Type");
  lineGraphTypeLabel->setAlignment(Qt::AlignHCenter);
  lineGraphTypeLabel->setFont(labelFont);
  lineGraphTypeLabel->setFrameStyle(QFrame::Panel | QFrame::StyledPanel);

  lineControlsVerticalLayout->addWidget(lineGraphTypeLabel);

  lineGraphRadioButton = new QRadioButton("Line Graph");
  sankeyGraphRadioButton = new QRadioButton("Sankey Diagram");
  lineGraphRadioButton->setChecked(true);
  connect(lineGraphRadioButton,SIGNAL(clicked(bool)),this,SLOT(toggleLabels()));

  QVBoxLayout * layout = new QVBoxLayout;
  layout->addWidget(lineGraphRadioButton);
  layout->addWidget(sankeyGraphRadioButton);
  lineControlsVerticalLayout->addLayout(layout);
  sankeyGraphRadioButton->setHidden(true);

  QLabel * lineGraphDimensionsLabel = new QLabel("Dimensions");
  lineGraphDimensionsLabel->setAlignment(Qt::AlignHCenter);
  lineGraphDimensionsLabel->setFont(labelFont);
  lineGraphDimensionsLabel->setFrameStyle(QFrame::Panel | QFrame::StyledPanel);
  lineControlsVerticalLayout->addWidget(lineGraphDimensionsLabel);

  lineGraphDimensionComboBox= new QComboBox;
  lineControlsVerticalLayout->addWidget(lineGraphDimensionComboBox);
  connect(lineGraphDimensionComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(lineGraphDimensionChanged(int)));

  lineGraphTurnSlider = new QSlider(Qt::Horizontal);
  lineGraphTurnSlider->setTickInterval(1);
  lineGraphTurnSlider->setTickPosition(QSlider::TicksBothSides);
  lineGraphTurnSlider->setPageStep(1);
  lineGraphTurnSlider->setSingleStep(1);
  lineGraphTurnSlider->setRange(0,1);
  lineGraphTurnSlider->setVisible(false);
  connect(lineGraphTurnSlider,SIGNAL(valueChanged(int)),this,SLOT(lineGraphTurnSliderChanged(int)));

  //positioning widgets in the grid
  lineGraphGridLayout->addWidget(lineGraphTurnSlider,0,0,1,1,Qt::AlignTop);
  lineGraphGridLayout->addWidget(lineGraphControlsFrame,1,1,Qt::AlignRight);
}

void MainWindow::initializeLineGraphPlot()
{

  QFont font("Helvetica[Adobe]",15);
  lineGraphTitle = new QCPPlotTitle(lineCustomGraph," ");
  lineGraphTitle->setFont(font);
  lineGraphTitle->setTextColor(QColor(255,51,51));

  lineCustomGraph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes |
                                   QCP::iSelectLegend | QCP::iSelectPlottables);

  lineCustomGraph->axisRect()->setupFullAxesBox();

  lineCustomGraph->plotLayout()->insertRow(0);
  lineCustomGraph->plotLayout()->addElement(0, 0,lineGraphTitle);

  lineCustomGraph->xAxis->setLabel("Time");
  lineCustomGraph->yAxis->setLabel(" ");
  lineCustomGraph->legend->setVisible(false);


  lineCustomGraph->xAxis->setAutoTicks(false);
  lineCustomGraph->xAxis->setAutoTickLabels(false);

  QVector<double> xAxisTicks;
  QVector<QString> xAxisLabels;

  for(int i =0 ; i < 100 ; ++i)
    {
      xAxisTicks <<i;
      xAxisLabels<<QString::number(i);
    }
  lineCustomGraph->xAxis->setTickVector(xAxisTicks);
  lineCustomGraph->xAxis->setTickVectorLabels(xAxisLabels);
  lineCustomGraph->xAxis->setRange(-1, 10);

  QVector<double> yAxisTicks;
  QVector<QString> yAxisLabels;

  for(int i =0 ; i <= 100 ;i+=10)
    {
      yAxisTicks <<i;
      yAxisLabels<<QString::number(i);
    }
  lineCustomGraph->yAxis->setAutoTicks(false);
  lineCustomGraph->yAxis->setAutoTickLabels(false);
  lineCustomGraph->yAxis->setTickStep(10.0);
  lineCustomGraph->yAxis->setTickVector(yAxisTicks);
  lineCustomGraph->yAxis->setTickVectorLabels(yAxisLabels);
  lineCustomGraph->yAxis->setRange(0,100);

  lineCustomGraph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

  //    QFont legendFont = font();
  //    legendFont.setPointSize(10);
  //    lineCustomGraph->legend->setFont(legendFont);
  //    lineCustomGraph->legend->setSelectedFont(legendFont);
  //    lineCustomGraph->legend->setSelectableParts(QCPLegend::spItems); // legend box shall not be selectable, only legend items

  //    connect(lineCustomGraph, SIGNAL(legendClick(QCPLegend*,QCPAbstractLegendItem*,QMouseEvent*)), this, SLOT(selectionChanged()));
  // connect slot that ties some axis selections together (especially opposite axes):
  //connect(lineCustomGraph, SIGNAL(selectionChangedByUser()), this, SLOT(selectionChanged()));
  // connect slots that takes care that when an axis is selected, only that direction can be dragged and zoomed:
  connect(lineCustomGraph, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePress()));
  connect(lineCustomGraph, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(mouseWheel()));

  // make bottom and left axes transfer their ranges to top and right axes:
  connect(lineCustomGraph->xAxis, SIGNAL(rangeChanged(QCPRange)), lineCustomGraph->xAxis2, SLOT(setRange(QCPRange)));
  connect(lineCustomGraph->yAxis, SIGNAL(rangeChanged(QCPRange)), lineCustomGraph->yAxis2, SLOT(setRange(QCPRange)));

  //    // connect some interaction slots:
  //    connect(lineCustomGraph, SIGNAL(titleDoubleClick(QMouseEvent*,QCPPlotTitle*)), this, SLOT(titleDoubleClick(QMouseEvent*,QCPPlotTitle*)));
  //    connect(lineCustomGraph, SIGNAL(axisDoubleClick(QCPAxis*,QCPAxis::SelectablePart,QMouseEvent*)), this, SLOT(axisLabelDoubleClick(QCPAxis*,QCPAxis::SelectablePart)));
  //    connect(lineCustomGraph, SIGNAL(legendDoubleClick(QCPLegend*,QCPAbstractLegendItem*,QMouseEvent*)), this, SLOT(legendDoubleClick(QCPLegend*,QCPAbstractLegendItem*)));

  // connect slot that shows a message in the status bar when a graph is clicked:
  connect(lineCustomGraph, SIGNAL(plottableClick(QCPAbstractPlottable*,QMouseEvent*)), this, SLOT(graphClicked(QCPAbstractPlottable*)));

  // setup policy and connect slot for context menu popup:
  lineCustomGraph->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(lineCustomGraph, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequest(QPoint)));
}

void MainWindow::populateLineGraphActorsList()
{
  QCheckBox * actor;
  lineGraphActorsCheckBoxList.clear();
  lineGraphCheckedActorsIdList.clear();
  lineActorCBList.clear();

  QWidget* widget = new QWidget;
  QVBoxLayout *layout = new QVBoxLayout(widget);

  for(int actorsCount = 0; actorsCount < actorsName.count(); ++actorsCount)
    {
      actor = new QCheckBox(actorsName.at(actorsCount));
      actor->setChecked(true);

      QColor mycolor = colorsList.at(actorsCount);

      QString style = "background: rgb(%1, %2, %3);";
      style = style.arg(mycolor.red()).arg(mycolor.green()).arg(mycolor.blue());
      style += "color:white; font-size:15px;";
      style += "font-weight:bold;";

      actor->setStyleSheet(style);

      actor->setObjectName(QString::number(actorsCount));

      layout->addWidget(actor);
      layout->stretch(0);

      lineGraphActorsCheckBoxList.append(actor);

      //setting all checkboxes as checked as initial condition
      lineGraphCheckedActorsIdList.append(true);
      connect(actor,SIGNAL(toggled(bool)),this,SLOT(lineGraphActorsCheckboxClicked(bool)));
      lineActorCBList.append(actor);
    }

  lineGraphActorsScrollArea->setWidget(widget);
}

void MainWindow::populateLineGraphDimensions(int dim)
{
  Q_UNUSED(dim)

  disconnect(lineGraphDimensionComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(lineGraphDimensionChanged(int)));
  lineGraphDimensionComboBox->clear();
  lineGraphDimensionComboBox->addItem(" ");
  for(int dims = 0; dims < dimensionList.length(); ++ dims )
    {
      lineGraphDimensionComboBox->addItem(dimensionList.at(dims));
    }
  connect(lineGraphDimensionComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(lineGraphDimensionChanged(int)));
  lineGraphDimensionComboBox->removeItem(0);
}


void MainWindow::updateLineDimension(QStringList *dims)
{
  QStringList  dimenList;
  for(int index=0;index<dims->length();++index)
    {
      dimenList.append(dims->at(index));
    }
  //    int lineDimIndex = lineGraphDimensionComboBox->currentIndex();

  disconnect(lineGraphDimensionComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(lineGraphDimensionChanged(int)));
  lineGraphDimensionComboBox->clear();
  lineGraphDimensionComboBox->addItem(" ");
  for(int dims = 0; dims < dimenList.length(); ++ dims )
    {
      lineGraphDimensionComboBox->addItem(dimenList.at(dims));
    }
  connect(lineGraphDimensionComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(lineGraphDimensionChanged(int)));
  lineGraphDimensionComboBox->removeItem(0);
  //lineGraphDimensionComboBox->currentIndexChanged(lineDimIndex);
}

void MainWindow::toggleLabels()
{
  if(lineLabelToggleList.contains(true))
    {
      for(int i=0; i <lineLabelList.length();++i)
        {
          lineLabelList.at(i)->setVisible(false);
          lineLabelToggleList[i]=false;
        }
    }
  else
    {
      for(int i=0; i <lineLabelList.length();++i)
        {
          lineLabelList.at(i)->setVisible(true);
          lineLabelToggleList[i]=true;
        }
    }
  lineCustomGraph->replot();
}

void MainWindow::splineValues(const QVector<double> &x, const QVector<double> &y)
{
  lineCustomGraph->graph()->setData(x, y);
}

void MainWindow::populateLineGraphStateRange(int states)
{
  lineCustomGraph->xAxis->setRange(-1, states+1);
  numStates = states;
  lineGraphTurnSlider->setRange(0,states);
  connect(turnSlider,SIGNAL(valueChanged(int)),lineGraphTurnSlider,SLOT(setValue(int)));
}

void MainWindow::clearAllLabels()
{
  for(int i=0; i <lineLabelList.length();++i)
    lineLabelList.at(i)->setVisible(false);

  lineLabelList.clear();
  lineLabelToggleList.clear();

  lineCustomGraph->replot();
}

void MainWindow::lineGraphSelectAllActorsCheckBoxClicked(bool click)
{
  for(int index=0; index < lineActorCBList.length();++ index)
    disconnect(lineActorCBList.at(index),SIGNAL(toggled(bool)),this,SLOT(lineGraphActorsCheckboxClicked(bool)));

  for(int actors = 0 ; actors < lineGraphCheckedActorsIdList.length(); ++actors)
    {
      lineGraphCheckedActorsIdList[actors]=click;
      lineGraphActorsCheckBoxList[actors]->setChecked(click);
    }
  for(int index=0; index < lineActorCBList.length();++ index)
    connect(lineActorCBList.at(index),SIGNAL(toggled(bool)),this,SLOT(lineGraphActorsCheckboxClicked(bool)));

  lineCustomGraph->clearGraphs();
  emit getScenarioRunValues(lineGraphTurnSlider->value(),scenarioBox,dimension);
  lineCustomGraph->replot();

}

void MainWindow::lineGraphDimensionChanged(int value)
{
  clearAllLabels();
  dimension=value;
  barGraphDimensionComboBox->setCurrentIndex(value);

  lineGraphTitle->setText(QString(lineGraphDimensionComboBox->currentText()
                                  + " vs Time, Iteration " +
                                  QString::number(lineGraphTurnSlider->value())));
  lineCustomGraph->yAxis->setLabel(lineGraphDimensionComboBox->currentText());

  lineCustomGraph->clearGraphs();
  emit getScenarioRunValues(lineGraphTurnSlider->value(),scenarioBox,dimension);
  lineCustomGraph->replot();
}

void MainWindow::lineGraphTurnSliderChanged(int value)
{
  lineGraphTitle->setText(QString(lineGraphDimensionComboBox->currentText()
                                  + " vs Time, Iteration " +
                                  QString::number(value)));
  lineCustomGraph->yAxis->setLabel(lineGraphDimensionComboBox->currentText());

  lineCustomGraph->clearGraphs();
  emit getScenarioRunValues(lineGraphTurnSlider->value(),scenarioBox,dimension);
  lineCustomGraph->replot();

}

void MainWindow::lineGraphActorsCheckboxClicked(bool click)
{
  QCheckBox * actorCheckBox = qobject_cast<QCheckBox *>(sender());
  quint8 actorId = actorCheckBox->objectName().toInt();

  lineGraphCheckedActorsIdList[actorId]=click;

  //    lineGraphTurnSliderChanged(lineGraphTurnSlider->value());

  lineCustomGraph->clearGraphs();
  emit getScenarioRunValues(lineGraphTurnSlider->value(),scenarioBox,dimension);
  lineCustomGraph->replot();

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
      lineCustomGraph->replot();
    }
}

void MainWindow::mousePress()
{
  // if an axis is selected, only allow the direction of that axis to be dragged
  // if no axis is selected, both directions may be dragged

  if (lineCustomGraph->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
    lineCustomGraph->axisRect()->setRangeDrag(lineCustomGraph->xAxis->orientation());
  else if (lineCustomGraph->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
    lineCustomGraph->axisRect()->setRangeDrag(lineCustomGraph->yAxis->orientation());
  else
    lineCustomGraph->axisRect()->setRangeDrag(Qt::Horizontal|Qt::Vertical);
}

void MainWindow::mouseWheel()
{
  // if an axis is selected, only allow the direction of that axis to be zoomed
  // if no axis is selected, both directions may be zoomed

  if (lineCustomGraph->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
    lineCustomGraph->axisRect()->setRangeZoom(lineCustomGraph->xAxis->orientation());
  else if (lineCustomGraph->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
    lineCustomGraph->axisRect()->setRangeZoom(lineCustomGraph->yAxis->orientation());
  else
    lineCustomGraph->axisRect()->setRangeZoom(Qt::Horizontal|Qt::Vertical);
}

void MainWindow::createSpline(const QVector<double> &x, const QVector<double> &y,QString Actor, int turn)
{
  if(turn==0)
    {
      QVector<double> X(x), Y(y);
      X.prepend(-0.1);
      Y.prepend(y.at(0));
      X.resize(2);
      Y.resize(2);

      lineCustomGraph->addGraph();
      lineCustomGraph->graph()->setData(X,Y);
    }
  else if(turn>0)
    {
      QVector<double> X(x), Y(y);

      QList< double > deltaX;
      QList< double > deltaY;

      QList< double > absDeltaX;
      QList< double > absDeltaY;

      QList< qint8 > sgnDeltaX;
      QList< qint8 > sgnDeltaY;

      double xDiff15 = 0.05;
      double xDiff24 = 0.05;
      double xDiff3  = 0.80;

      double yDiff15 = 0.01;
      double yDiff24 = 0.03;
      double yDiff3  = 0.92;

      double xVal = 0.0;
      double yVal = Y.at(0);

      // turn 0 initialization
      deltaX.append(0);
      deltaY.append(0);
      absDeltaX.append(0);
      absDeltaY.append(0);
      sgnDeltaX.append(0);
      sgnDeltaY.append(0);

      for(int i = 1 ; i <= turn ; ++i)
        {
          deltaX.append(X.at(i)-X.at(i-1));
          deltaY.append(Y.at(i)-Y.at(i-1));
          absDeltaX.append(abs(X.at(i)-X.at(i-1)));
          absDeltaY.append(abs(Y.at(i)-Y.at(i-1)));
          sgnDeltaX.append((deltaX.at(i) > 0) ? 1 : (deltaX.at(i) < 0) ? -1 : 0);
          sgnDeltaY.append((deltaY.at(i) > 0) ? 1 : (deltaY.at(i) < 0) ? -1 : 0);
        }

      QVector <double> xNew, yNew;
      for(int i = 1 ; i <= turn; ++i)
        {
          xNew.append(xVal                     + sgnDeltaX.at(i) * xDiff15 * absDeltaX.at(i)); //xstp1
          xNew.append(xNew.at(xNew.length()-1) + sgnDeltaX.at(i) * xDiff24 * absDeltaX.at(i)); //xstp2
          xNew.append(xNew.at(xNew.length()-1) + sgnDeltaX.at(i) * xDiff3  * absDeltaX.at(i)); //xstp3
          xNew.append(xNew.at(xNew.length()-1) + sgnDeltaX.at(i) * xDiff24 * absDeltaX.at(i)); //xstp4
          xNew.append(xNew.at(xNew.length()-1) + sgnDeltaX.at(i) * xDiff15 * absDeltaX.at(i)); //xstp5
          xVal=xNew.at(xNew.length()-1);

          yNew.append(yVal                     + sgnDeltaY.at(i) * yDiff15 * absDeltaY.at(i)); //ystp1
          yNew.append(yNew.at(yNew.length()-1) + sgnDeltaY.at(i) * yDiff24 * absDeltaY.at(i)); //ystp2
          yNew.append(yNew.at(yNew.length()-1) + sgnDeltaY.at(i) * yDiff3  * absDeltaY.at(i)); //ystp3
          yNew.append(yNew.at(yNew.length()-1) + sgnDeltaY.at(i) * yDiff24 * absDeltaY.at(i)); //ystp4
          yNew.append(yNew.at(yNew.length()-1) + sgnDeltaY.at(i) * yDiff15 * absDeltaY.at(i)); //ystp5
          yVal=yNew.at(yNew.length()-1);

        }

      xNew.prepend(-0.1); // Horizontal bar at 0
      yNew.prepend(yNew.at(0));

      lineCustomGraph->addGraph();
      lineCustomGraph->graph()->setData(xNew,yNew);
      lineCustomGraph->graph()->setScatterStyle(QCPScatterStyle((QCPScatterStyle::ScatterShape)(0)));


    }

  lineCustomGraph->graph()->setName(Actor);

  QString actorDetails;
  actorDetails.append("Name: " +Actor + "\n");
  actorDetails.append("Description: " +actorsDescription.at(actorsName.indexOf(Actor)) + "\n");
  actorDetails.append("Influence: " +actorsInfl.at(actorsName.indexOf(Actor)));

  lineCustomGraph->graph()->setTooltip(actorDetails);

  QPen graphPen;
  graphPen.setColor(colorsList.at(actorsName.indexOf(Actor)));
  graphPen.setWidthF(2.0);

  lineCustomGraph->graph()->setPen(graphPen);

}

void MainWindow::addGraphOnModule1(const QVector<double> &x, const QVector<double> &y,QString Actor,int turn)
{
  //  && actorsName.length()==numAct && Actor.isEmpty()==false
  if(lineLabelList.length() <= actorsName.length())
    {
      textLabel = new QCPItemText(lineCustomGraph);
      textLabel->setText(Actor);
      textLabel->setColor(colorsList.at(actorsName.indexOf(Actor)));
      textLabel->position->setCoords(x.at(0)-0.7,y.at(0)+0.3);
      textLabel->setVisible(false);
      lineCustomGraph->addItem(textLabel);
      lineLabelList.append(textLabel);
      lineLabelToggleList.append(false);
    }
  if(lineGraphCheckedActorsIdList.at(actorsName.indexOf(Actor))==true)
    {
      createSpline(x,y,Actor,turn);
    }
}
void MainWindow::removeAllGraphs()
{
  lineCustomGraph->clearGraphs();
}

void MainWindow::contextMenuRequest(QPoint pos)
{
  QMenu *menu = new QMenu(this);
  menu->setAttribute(Qt::WA_DeleteOnClose);

  if (lineCustomGraph->legend->selectTest(pos, false) >= 0) // context menu on legend requested
    {
      //      menu->addAction("Move to top left", this, SLOT(moveLegend()))->setData((int)(Qt::AlignTop|Qt::AlignLeft));
      //      menu->addAction("Move to top center", this, SLOT(moveLegend()))->setData((int)(Qt::AlignTop|Qt::AlignHCenter));
      //      menu->addAction("Move to top right", this, SLOT(moveLegend()))->setData((int)(Qt::AlignTop|Qt::AlignRight));
      //      menu->addAction("Move to bottom right", this, SLOT(moveLegend()))->setData((int)(Qt::AlignBottom|Qt::AlignRight));
      //      menu->addAction("Move to bottom left", this, SLOT(moveLegend()))->setData((int)(Qt::AlignBottom|Qt::AlignLeft));
    }
  else  // general context menu on graphs requested
    {
      //      menu->addAction("Add random graph", this, SLOT(addGraphOnModule1()));
      //      if (lineCustomGraph->selectedGraphs().size() > 0)
      //        menu->addAction("Remove selected graph", this, SLOT(removeSelectedGraph()));
      //      if (lineCustomGraph->graphCount() > 0)
      //        menu->addAction("Remove all graphs", this, SLOT(removeAllGraphs()));
    }

  menu->popup(lineCustomGraph->mapToGlobal(pos));
}

void MainWindow::moveLegend()
{
  if (QAction * contextAction = qobject_cast<QAction*>(sender())) // make sure this slot is really called by a context menu action, so it carries the data we need
    {
      bool ok;
      int dataInt = contextAction->data().toInt(&ok);
      if (ok)
        {
          lineCustomGraph->axisRect()->insetLayout()->setInsetAlignment(0, (Qt::Alignment)dataInt);
          lineCustomGraph->replot();
        }
    }
}

void MainWindow::graphClicked(QCPAbstractPlottable *plottable)
{
  statusBar()->showMessage(QString("Clicked on Line '%1'.").arg(plottable->name()), 1000);

  if(lineLabelToggleList.at(actorsName.indexOf(plottable->name()))==true)
    {
      lineLabelList.at(actorsName.indexOf(plottable->name()))->setVisible(false);
      lineLabelToggleList[actorsName.indexOf(plottable->name())]=false;
    }
  else
    {
      lineLabelList.at(actorsName.indexOf(plottable->name()))->setVisible(true);
      lineLabelToggleList[actorsName.indexOf(plottable->name())]=true;
    }

  lineCustomGraph->replot();
}



// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
