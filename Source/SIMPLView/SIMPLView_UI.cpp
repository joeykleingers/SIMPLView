/* ============================================================================
 * Copyright (c) 2009-2016 BlueQuartz Software, LLC
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of BlueQuartz Software, the US Air Force, nor the names of its
 * contributors may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The code contained herein was partially funded by the followig contracts:
 *    United States Air Force Prime Contract FA8650-07-D-5800
 *    United States Air Force Prime Contract FA8650-10-D-5210
 *    United States Prime Contract Navy N00173-07-C-2068
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "SIMPLView_UI.h"

//-- Qt Includes
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QFileInfoList>
#include <QtCore/QMimeData>
#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QUrl>
#include <QtGui/QCloseEvent>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QShortcut>
#include <QtWidgets/QToolButton>

//-- SIMPLView Includes
#include "SIMPLib/Common/Constants.h"
#include "SIMPLib/Common/DocRequestManager.h"
#include "SIMPLib/FilterParameters/JsonFilterParametersReader.h"
#include "SIMPLib/Filtering/FilterManager.h"
#include "SIMPLib/Plugin/PluginManager.h"
#include "SIMPLib/Utilities/SIMPLDataPathValidator.h"

#include "SVWidgetsLib/Animations/PipelineItemBorderSizeAnimation.h"
#include "SVWidgetsLib/Core/FilterWidgetManager.h"
#include "SVWidgetsLib/Dialogs/AboutPlugins.h"
#include "SVWidgetsLib/QtSupport/QtSMacros.h"
#include "SVWidgetsLib/QtSupport/QtSPluginFrame.h"
#include "SVWidgetsLib/QtSupport/QtSRecentFileList.h"
#include "SVWidgetsLib/QtSupport/QtSFileUtils.h"
#include "SVWidgetsLib/Widgets/BookmarksModel.h"
#include "SVWidgetsLib/Widgets/BookmarksToolboxWidget.h"
#include "SVWidgetsLib/Widgets/BookmarksTreeView.h"
#include "SVWidgetsLib/Widgets/FilterLibraryToolboxWidget.h"
#include "SVWidgetsLib/Widgets/PipelineListWidget.h"
#include "SVWidgetsLib/Widgets/PipelineModel.h"
#include "SVWidgetsLib/Widgets/PipelineViewController.h"
#include "SVWidgetsLib/Widgets/SVStyle.h"
#include "SVWidgetsLib/Widgets/StatusBarWidget.h"
#include "SVWidgetsLib/Widgets/SVPipelineTreeView.h"
#ifdef SIMPL_USE_QtWebEngine
#include "SVWidgetsLib/Widgets/SVUserManualDialog.h"
#else
#include <QtGui/QDesktopServices>
#include <QtWidgets/QMessageBox>
#endif

#ifdef SIMPL_USE_MKDOCS
#define URL_GENERATOR QtSDocServer
#include "SVWidgetsLib/QtSupport/QtSDocServer.h"
#endif

#ifdef SIMPL_USE_DISCOUNT
#define URL_GENERATOR QtSHelpUrlGenerator
#include "SVWidgetsLib/QtSupport/QtSHelpUrlGenerator.h"
#endif

#include "SIMPLView/AboutSIMPLView.h"
#include "SIMPLView/SIMPLView.h"
#include "SIMPLView/SIMPLViewApplication.h"
#include "SIMPLView/SIMPLViewConstants.h"
#include "SIMPLView/SIMPLViewVersion.h"

#include "BrandedStrings.h"

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
SIMPLView_UI::SIMPLView_UI(QWidget* parent)
: QMainWindow(parent)
, m_Ui(new Ui::SIMPLView_UI)
, m_FilterManager(nullptr)
, m_FilterWidgetManager(nullptr)
{
  // Register all of the Filters we know about - the rest will be loaded through plugins
  //  which all should have been loaded by now.
  m_FilterManager = FilterManager::Instance();
  // m_FilterManager->RegisterKnownFilters(m_FilterManager);

  // Register all the known filterWidgets
  m_FilterWidgetManager = FilterWidgetManager::Instance();
  m_FilterWidgetManager->RegisterKnownFilterWidgets();

  // Calls the Parent Class to do all the Widget Initialization that were created
  // using the QDesigner program
  m_Ui->setupUi(this);

  dream3dApp->registerSIMPLViewWindow(this);

  // Do our own widget initializations
  setupGui();

  this->setAcceptDrops(true);

  // Read various settings
  readSettings();
  if(SIMPLView::DockWidgetSettings::HideDockSetting::OnError == IssuesWidget::GetHideDockSetting())
  {
    m_Ui->issuesDockWidget->setHidden(true);
  }
  if(SIMPLView::DockWidgetSettings::HideDockSetting::OnError == StandardOutputWidget::GetHideDockSetting())
  {
    m_Ui->stdOutDockWidget->setHidden(true);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
SIMPLView_UI::~SIMPLView_UI()
{
  writeSettings();

  dream3dApp->unregisterSIMPLViewWindow(this);

  if(dream3dApp->activeWindow() == this)
  {
    dream3dApp->setActiveWindow(nullptr);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::resizeEvent(QResizeEvent* event)
{
  QMainWindow::resizeEvent(event);

  emit parentResized();

  // We need to write the window settings so that any new windows will open with these window settings
  writeWindowSettings();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::addPipeline(FilterPipeline::Pointer pipeline, int insertIndex)
{
  PipelineView* pipelineView = getPipelineView();
  if(pipelineView)
  {
    pipelineView->addPipeline(pipeline, insertIndex);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::listenSavePipelineTriggered()
{
  savePipeline();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool SIMPLView_UI::savePipeline()
{
  PipelineView* pipelineView = getPipelineView();
  return pipelineView->savePipeline();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::listenSavePipelineAsTriggered()
{
  savePipelineAs();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool SIMPLView_UI::savePipelineAs()
{
  PipelineView* pipelineView = getPipelineView();
  return pipelineView->savePipelineAs();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::activateBookmark(const QString& filePath, bool execute)
{
  SIMPLView_UI* instance = dream3dApp->getActiveInstance();

  PipelineView* pipelineView = instance->getPipelineView();
  PipelineModel* pipelineModel = pipelineView->getPipelineModel();
  if (pipelineModel && pipelineModel->rowCount() == pipelineModel->getMaxPipelineCount())
  {
    instance = dream3dApp->getNewSIMPLViewInstance();
    instance->show();
  }

  instance->openPipeline(filePath);

  QtSRecentFileList* list = QtSRecentFileList::Instance();
  list->addFile(filePath);

  if(execute)
  {
//    pipelineView->executePipeline();
  }

  instance->raise();
  QApplication::setActiveWindow(instance);
}

// -----------------------------------------------------------------------------
//  Called when the main window is closed.
// -----------------------------------------------------------------------------
void SIMPLView_UI::closeEvent(QCloseEvent* event)
{
  PipelineView* pipelineView = getPipelineView();
  if(pipelineView && pipelineView->arePipelinesRunning() == true)
  {
    QMessageBox runningPipelineBox;
    runningPipelineBox.setWindowTitle("Pipelines Are Running");
    runningPipelineBox.setText("There are pipelines currently running.\nPlease cancel the running pipelines and try again.");
    runningPipelineBox.setStandardButtons(QMessageBox::Ok);
    runningPipelineBox.setIcon(QMessageBox::Warning);
    runningPipelineBox.exec();
    event->ignore();
    return;
  }

  QMessageBox::StandardButton choice = checkDirtyDocument();
  if(choice == QMessageBox::Cancel)
  {
    event->ignore();
    return;
  }

  // Status Bar Widget needs to write out its settings BEFORE the main window is closed
  //  m_StatusBar->writeSettings();

  event->accept();
}

// -----------------------------------------------------------------------------
//  Read our settings from a file
// -----------------------------------------------------------------------------
void SIMPLView_UI::readSettings()
{
  QSharedPointer<QtSSettings> prefs = QSharedPointer<QtSSettings>(new QtSSettings());

  // Have the pipeline builder read its settings from the prefs file
  readWindowSettings();
  readVersionCheckSettings();

  // Read dock widget settings
  prefs->beginGroup(SIMPLView::DockWidgetSettings::GroupName);

  prefs->beginGroup(SIMPLView::DockWidgetSettings::IssuesDockGroupName);
  readDockWidgetSettings(prefs.data(), m_Ui->issuesDockWidget);
  prefs->endGroup();

  prefs->beginGroup(SIMPLView::DockWidgetSettings::StandardOutputGroupName);
  readDockWidgetSettings(prefs.data(), m_Ui->stdOutDockWidget);
  prefs->endGroup();

  prefs->endGroup();

  prefs->beginGroup("ToolboxSettings");

  // Read dock widget settings
  prefs->beginGroup("Bookmarks Widget");
  m_Ui->bookmarksWidget->readSettings(prefs.data());
  prefs->endGroup();

  prefs->beginGroup("Filter List Widget");
  m_Ui->filterListWidget->readSettings(prefs.data());
  prefs->endGroup();

  prefs->beginGroup("Filter Library Widget");
  m_Ui->filterLibraryWidget->readSettings(prefs.data());
  prefs->endGroup();

  prefs->endGroup();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::readWindowSettings()
{
  QSharedPointer<QtSSettings> prefs = QSharedPointer<QtSSettings>(new QtSSettings());

  bool ok = false;
  prefs->beginGroup("WindowSettings");
  if(prefs->contains(QString("MainWindowGeometry")))
  {
    QByteArray geo_data = prefs->value("MainWindowGeometry", QByteArray());
    ok = restoreGeometry(geo_data);
    if(!ok)
    {
      qDebug() << "Error Restoring the Window Geometry"
               << "\n";
    }
  }

  if(prefs->contains(QString("MainWindowState")))
  {
    QByteArray layout_data = prefs->value("MainWindowState", QByteArray());
    restoreState(layout_data);
  }

  prefs->endGroup();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::readDockWidgetSettings(QtSSettings* prefs, QDockWidget* dw)
{
  restoreDockWidget(dw);

  QString name = dw->objectName();
  bool b = prefs->value(dw->objectName(), QVariant(false)).toBool();
  dw->setHidden(b);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::readVersionCheckSettings()
{
}

// -----------------------------------------------------------------------------
//  Write our Prefs to file
// -----------------------------------------------------------------------------
void SIMPLView_UI::writeSettings()
{
  // Have the pipeline builder write its settings to the prefs file
  writeWindowSettings();

  // Have the version check widet write its preferences.
  writeVersionCheckSettings();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::writeVersionCheckSettings()
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::writeWindowSettings()
{
  QSharedPointer<QtSSettings> prefs = QSharedPointer<QtSSettings>(new QtSSettings());

  prefs->beginGroup("WindowSettings");
  QByteArray geo_data = saveGeometry();
  QByteArray layout_data = saveState();
  prefs->setValue(QString("MainWindowGeometry"), geo_data);
  prefs->setValue(QString("MainWindowState"), layout_data);

  prefs->endGroup();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::writeDockWidgetSettings(QtSSettings* prefs, QDockWidget* dw)
{
  prefs->setValue(dw->objectName(), dw->isHidden());
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::setupGui()
{
  setTabPosition(Qt::DockWidgetArea::TopDockWidgetArea, QTabWidget::TabPosition::North);
  setTabPosition(Qt::DockWidgetArea::RightDockWidgetArea, QTabWidget::TabPosition::North); 
  setTabPosition(Qt::DockWidgetArea::BottomDockWidgetArea, QTabWidget::TabPosition::North); 
  setTabPosition(Qt::DockWidgetArea::LeftDockWidgetArea, QTabWidget::TabPosition::North);

#ifdef SIMPLView_USE_TREEVIEW
  setupPipelineTreeView();
#else
  setupPipelineListView();
#endif

  // Set the IssuesWidget as a PipelineMessageObserver Object.
  PipelineView* pipelineView = getPipelineView();
  pipelineView->addPipelineMessageObserver(m_Ui->issuesWidget);
  pipelineView->addPipelineMessageObserver(this);

  QAbstractItemView* abstractPipelineView = getAbstractPipelineView();
  abstractPipelineView->installEventFilter(this);

  createSIMPLViewMenuSystem();

  // Hook up the signals from the various docks to the PipelineViewWidget that will either add a filter
  // or load an entire pipeline into the view
  connectSignalsSlots();

  // This will set the initial list of filters in the FilterListToolboxWidget
  // Tell the Filter Library that we have more Filters (potentially)
  m_Ui->filterLibraryWidget->refreshFilterGroups();

  // Read the toolbox settings and update the filter list
  m_Ui->filterListWidget->loadFilterList();

  tabifyDockWidget(m_Ui->filterListDockWidget, m_Ui->filterLibraryDockWidget);
  tabifyDockWidget(m_Ui->filterLibraryDockWidget, m_Ui->bookmarksDockWidget);

  m_Ui->filterListDockWidget->raise();

  // Shortcut to close the window
  new QShortcut(QKeySequence(QKeySequence::Close), this, SLOT(close()));

  //  m_StatusBar = new StatusBarWidget();
  //  this->statusBar()->insertPermanentWidget(0, m_StatusBar, 0);

  //  m_StatusBar->setButtonAction(m_Ui->filterListDockWidget, StatusBarWidget::Button::FilterList);
  //  m_StatusBar->setButtonAction(m_Ui->filterLibraryDockWidget, StatusBarWidget::Button::FilterLibrary);
  //  m_StatusBar->setButtonAction(m_Ui->bookmarksDockWidget, StatusBarWidget::Button::Bookmarks);
  //  m_StatusBar->setButtonAction(m_Ui->pipelineDockWidget, StatusBarWidget::Button::Pipeline);
  //  m_StatusBar->setButtonAction(m_Ui->issuesDockWidget, StatusBarWidget::Button::Issues);
  //  m_StatusBar->setButtonAction(m_Ui->stdOutDockWidget, StatusBarWidget::Button::Console);
  //  m_StatusBar->setButtonAction(m_Ui->dataBrowserDockWidget, StatusBarWidget::Button::DataStructure);

  //  m_StatusBar->readSettings();

  //  connect(m_Ui->issuesWidget, SIGNAL(tableHasErrors(bool, int, int)), m_StatusBar, SLOT(issuesTableHasErrors(bool, int, int)));
  connect(m_Ui->issuesWidget, SIGNAL(tableHasErrors(bool, int, int)), this, SLOT(issuesTableHasErrors(bool, int, int)));
  connect(m_Ui->issuesWidget, SIGNAL(showTable(bool)), m_Ui->issuesDockWidget, SLOT(setVisible(bool)));

  connectDockWidgetSignalsSlots(m_Ui->bookmarksDockWidget);
  connectDockWidgetSignalsSlots(m_Ui->dataBrowserDockWidget);
  connectDockWidgetSignalsSlots(m_Ui->filterLibraryDockWidget);
  connectDockWidgetSignalsSlots(m_Ui->filterListDockWidget);
  connectDockWidgetSignalsSlots(m_Ui->issuesDockWidget);
  connectDockWidgetSignalsSlots(m_Ui->pipelineDockWidget);
  connectDockWidgetSignalsSlots(m_Ui->stdOutDockWidget);

  m_Ui->bookmarksDockWidget->installEventFilter(this);
  m_Ui->dataBrowserDockWidget->installEventFilter(this);
  m_Ui->filterLibraryDockWidget->installEventFilter(this);
  m_Ui->filterListDockWidget->installEventFilter(this);
  m_Ui->issuesDockWidget->installEventFilter(this);
  m_Ui->pipelineDockWidget->installEventFilter(this);
  m_Ui->stdOutDockWidget->installEventFilter(this);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::setupPipelineListView()
{
  PipelineListWidget* pipelineListWidget = new PipelineListWidget(m_Ui->pipelineInteralWidget);
  pipelineListWidget->setObjectName(QStringLiteral("pipelineListWidget"));
  m_Ui->gridLayout_3->addWidget(pipelineListWidget, 0, 0, 1, 1);
  setWindowTitle(tr("[*]%1 - %2").arg("Untitled").arg(BrandedStrings::ApplicationName));

  m_PipelineView = dynamic_cast<PipelineView*>(pipelineListWidget->getPipelineView());
  m_AbstractPipelineView = dynamic_cast<QAbstractItemView*>(pipelineListWidget->getPipelineView());

  connect(m_PipelineView->getPipelineModel(), &PipelineModel::pipelineAdded, [=](FilterPipeline::Pointer pipeline, const QModelIndex &pipelineRootIndex) {
    Q_UNUSED(pipelineRootIndex)
    setWindowTitle(tr("[*]%1 - %2").arg(pipeline->getName()).arg(BrandedStrings::ApplicationName));
  });

  connect(m_PipelineView->getPipelineModel(), &PipelineModel::pipelineSaved, [=](FilterPipeline::Pointer pipeline, const QModelIndex &pipelineRootIndex) {
    Q_UNUSED(pipelineRootIndex)
    setWindowTitle(tr("[*]%1 - %2").arg(pipeline->getName()).arg(BrandedStrings::ApplicationName));
    setWindowModified(false);
  });

  connect(m_PipelineView->getPipelineModel(), &PipelineModel::pipelineModified, [=](FilterPipeline::Pointer pipeline, const QModelIndex &pipelineRootIndex, bool modified) {
    Q_UNUSED(pipeline)
    Q_UNUSED(pipelineRootIndex)
    setWindowModified(modified);
  });
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::setupPipelineTreeView()
{
  SVPipelineTreeView* pipelineTreeView = new SVPipelineTreeView(m_Ui->pipelineInteralWidget);
  pipelineTreeView->setObjectName(QStringLiteral("pipelineTreeView"));
  pipelineTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  pipelineTreeView->setDragEnabled(true);
  pipelineTreeView->setDragDropMode(QAbstractItemView::DragDrop);
  pipelineTreeView->setDefaultDropAction(Qt::MoveAction);
  pipelineTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  pipelineTreeView->setAnimated(true);
  pipelineTreeView->header()->setVisible(false);
  m_Ui->gridLayout_3->addWidget(pipelineTreeView, 0, 0, 1, 1);

  m_PipelineView = dynamic_cast<PipelineView*>(pipelineTreeView);
  m_AbstractPipelineView = dynamic_cast<QAbstractItemView*>(pipelineTreeView);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool SIMPLView_UI::eventFilter(QObject* watched, QEvent* event)
{
  if (watched == getAbstractPipelineView() && event->type() == QEvent::KeyPress)
  {
    QAbstractItemView* abstractItemView = getAbstractPipelineView();
    PipelineView* pipelineView = getPipelineView();
    PipelineModel* pipelineModel = pipelineView->getPipelineModel();

    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    if(keyEvent->key() == Qt::Key_Backspace || keyEvent->key() == Qt::Key_Delete)
    {
      QModelIndexList selectedIndexes = abstractItemView->selectionModel()->selectedRows();
      if(selectedIndexes.size() <= 0)
      {
        return false;
      }

      std::vector<AbstractFilter::Pointer> filters;
      for (int i = 0; i < selectedIndexes.size(); i++)
      {
        QModelIndex selectedIndex = selectedIndexes[i];
        PipelineItem::ItemType itemType = static_cast<PipelineItem::ItemType>(pipelineModel->data(selectedIndex, PipelineModel::ItemTypeRole).toInt());
        if(itemType == PipelineItem::ItemType::PipelineRoot)
        {
          PipelineItem::PipelineState pipelineState = static_cast<PipelineItem::PipelineState>(pipelineModel->data(selectedIndex, PipelineModel::PipelineStateRole).toInt());
          if (pipelineState != PipelineItem::PipelineState::Running)
          {
            pipelineView->removePipeline(selectedIndex);
          }
        }
        else if (itemType == PipelineItem::ItemType::Filter && selectedIndex.parent().isValid())
        {
          PipelineItem::PipelineState pipelineState = static_cast<PipelineItem::PipelineState>(pipelineModel->data(selectedIndex.parent(), PipelineModel::PipelineStateRole).toInt());
          if (pipelineState != PipelineItem::PipelineState::Running)
          {
            AbstractFilter::Pointer filter = pipelineModel->filter(selectedIndex);
            filters.push_back(filter);
          }
        }
      }

      if(filters.empty() == false)
      {
        pipelineView->removeFilters(filters);
      }
    }
    else if(keyEvent->key() == Qt::Key_A && qApp->queryKeyboardModifiers() == Qt::ControlModifier)
    {
      abstractItemView->selectAll();
    }
  }
  else if(static_cast<QDockWidget*>(watched) != nullptr)
  {
    // Writes the window settings when dock widgets are resized or when the tabs are rearranged.  ChildRemoved and ChildAdded
    // are the only signals emitted when changing the order of the tabs, and there doesn't seem to be a better way to detect that.
    if(event->type() == QEvent::Resize || event->type() == QEvent::ChildRemoved || event->type() == QEvent::ChildAdded)
    {
      writeWindowSettings();
    }
  }

  return QMainWindow::eventFilter(watched, event);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::createSIMPLViewMenuSystem()
{
  m_SIMPLViewMenu = new QMenuBar(this);

  m_MenuFile = new QMenu("File", this);
  m_MenuEdit = new QMenu("Edit", this);
  m_MenuView = new QMenu("View", this);
  m_MenuBookmarks = new QMenu("Bookmarks", this);
  m_MenuPipeline = new QMenu("Pipeline", this);
  m_MenuHelp = new QMenu("Help", this);
  m_MenuAdvanced = new QMenu("Advanced", this);
  m_MenuThemes = new QMenu("Themes", this);
  QMenu* menuRecentFiles = dream3dApp->getRecentFilesMenu();

  m_ActionNew = new QAction("New...", this);
  m_ActionOpen = new QAction("Open...", this);
  m_ActionSave = new QAction("Save", this);
  m_ActionSaveAs = new QAction("Save As...", this);
  m_ActionLoadTheme = new QAction("Load Theme", this);
  m_ActionSaveTheme = new QAction("Save Theme", this);
  m_ActionClearRecentFiles = new QAction("Clear Recent Files", this);
  m_ActionExit = new QAction("Exit " + QApplication::applicationName(), this);
  m_ActionShowSIMPLViewHelp = new QAction(QApplication::applicationName() + " Help", this);
  m_ActionAboutSIMPLView = new QAction("About " + QApplication::applicationName(), this);
  m_ActionCheckForUpdates = new QAction("Check For Updates", this);
  m_ActionPluginInformation = new QAction("Plugin Information", this);
  m_ActionClearCache = new QAction("Reset Preferences", this);

  // SIMPLView_UI Actions
  connect(m_ActionNew, &QAction::triggered, dream3dApp, &SIMPLViewApplication::listenNewInstanceTriggered);
  connect(m_ActionOpen, &QAction::triggered, dream3dApp, &SIMPLViewApplication::listenOpenPipelineTriggered);
  connect(m_ActionSave, &QAction::triggered, this, &SIMPLView_UI::listenSavePipelineTriggered);
  connect(m_ActionSaveAs, &QAction::triggered, this, &SIMPLView_UI::listenSavePipelineAsTriggered);
  connect(m_ActionExit, &QAction::triggered, dream3dApp, &SIMPLViewApplication::listenExitApplicationTriggered);
  connect(m_ActionClearRecentFiles, &QAction::triggered, dream3dApp, &SIMPLViewApplication::listenClearRecentFilesTriggered);
  connect(m_ActionAboutSIMPLView, &QAction::triggered, dream3dApp, &SIMPLViewApplication::listenDisplayAboutSIMPLViewDialogTriggered);
  connect(m_ActionCheckForUpdates, &QAction::triggered, dream3dApp, &SIMPLViewApplication::listenCheckForUpdatesTriggered);
  connect(m_ActionShowSIMPLViewHelp, &QAction::triggered, dream3dApp, &SIMPLViewApplication::listenShowSIMPLViewHelpTriggered);
  connect(m_ActionPluginInformation, &QAction::triggered, dream3dApp, &SIMPLViewApplication::listenDisplayPluginInfoDialogTriggered);
  connect(m_ActionClearCache, &QAction::triggered, dream3dApp, &SIMPLViewApplication::listenClearSIMPLViewCacheTriggered);

  m_ActionNew->setShortcut(QKeySequence::New);
  m_ActionOpen->setShortcut(QKeySequence::Open);
  m_ActionSave->setShortcut(QKeySequence::Save);
  m_ActionSaveAs->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_S));
  m_ActionExit->setShortcut(QKeySequence::Quit);
  m_ActionCheckForUpdates->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));
  m_ActionShowSIMPLViewHelp->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H));
  m_ActionPluginInformation->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_I));

  // Pipeline View Actions
  PipelineView* viewWidget = getPipelineView();
  QAction* actionCut = viewWidget->getPipelineViewController()->getActionCut();
  QAction* actionCopy = viewWidget->getPipelineViewController()->getActionCopy();
  QAction* actionPaste = viewWidget->getPipelineViewController()->getActionPaste();
  QAction* actionClearPipeline = viewWidget->getPipelineViewController()->getActionClearPipeline();
  QAction* actionUndo = viewWidget->getActionUndo();
  QAction* actionRedo = viewWidget->getActionRedo();

  // Bookmarks Actions
  BookmarksTreeView* bookmarksView = m_Ui->bookmarksWidget->getBookmarksTreeView();
  QAction* actionAddBookmark = bookmarksView->getActionAddBookmark();
  QAction* actionNewFolder = bookmarksView->getActionAddBookmarkFolder();
  QAction* actionClearBookmarks = bookmarksView->getActionClearBookmarks();

  // Create File Menu
  m_SIMPLViewMenu->addMenu(m_MenuFile);
  m_MenuFile->addAction(m_ActionNew);
  m_MenuFile->addAction(m_ActionOpen);
  m_MenuFile->addSeparator();
  m_MenuFile->addAction(m_ActionSave);
  m_MenuFile->addAction(m_ActionSaveAs);
  m_MenuFile->addSeparator();
  m_MenuFile->addAction(menuRecentFiles->menuAction());
  m_MenuFile->addSeparator();
  m_MenuFile->addAction(m_ActionExit);

  // Create Edit Menu
  m_SIMPLViewMenu->addMenu(m_MenuEdit);
  m_MenuEdit->addAction(actionUndo);
  m_MenuEdit->addAction(actionRedo);
  m_MenuEdit->addSeparator();
  m_MenuEdit->addAction(actionCut);
  m_MenuEdit->addAction(actionCopy);
  m_MenuEdit->addAction(actionPaste);

  // Create View Menu
  m_SIMPLViewMenu->addMenu(m_MenuView);

  QStringList themeNames = BrandedStrings::LoadedThemeNames;
  if (themeNames.size() > 0)  // We are not counting the Default theme when deciding whether or not to add the theme menu
  {
    m_ThemeActionGroup = new QActionGroup(this);
    m_MenuThemes = dream3dApp->createThemeMenu(m_ThemeActionGroup, m_SIMPLViewMenu);

    m_MenuView->addMenu(m_MenuThemes);

    m_MenuView->addSeparator();
  }

  m_MenuView->addAction(m_Ui->filterListDockWidget->toggleViewAction());
  m_MenuView->addAction(m_Ui->filterLibraryDockWidget->toggleViewAction());
  m_MenuView->addAction(m_Ui->bookmarksDockWidget->toggleViewAction());
  m_MenuView->addAction(m_Ui->pipelineDockWidget->toggleViewAction());
  m_MenuView->addAction(m_Ui->issuesDockWidget->toggleViewAction());
  m_MenuView->addAction(m_Ui->stdOutDockWidget->toggleViewAction());
  m_MenuView->addAction(m_Ui->dataBrowserDockWidget->toggleViewAction());

  // Create Bookmarks Menu
  m_SIMPLViewMenu->addMenu(m_MenuBookmarks);
  m_MenuBookmarks->addAction(actionAddBookmark);
  m_MenuBookmarks->addSeparator();
  m_MenuBookmarks->addAction(actionNewFolder);

  // Create Pipeline Menu
  m_SIMPLViewMenu->addMenu(m_MenuPipeline);
  m_MenuPipeline->addAction(actionClearPipeline);

  // Create Help Menu
  m_SIMPLViewMenu->addMenu(m_MenuHelp);
  m_MenuHelp->addAction(m_ActionShowSIMPLViewHelp);
  m_MenuHelp->addSeparator();

  m_MenuHelp->addAction(m_ActionCheckForUpdates);
  m_MenuHelp->addSeparator();

  m_MenuHelp->addMenu(m_MenuAdvanced);
  m_MenuAdvanced->addAction(m_ActionClearCache);
  m_MenuAdvanced->addSeparator();
  m_MenuAdvanced->addAction(actionClearBookmarks);

  #if defined SIMPL_RELATIVE_PATH_CHECK

  m_MenuDataDirectory = new QMenu("Data Directory", this);
  m_ActionSetDataFolder = new QAction("Set Location...", this);
  m_ActionShowDataFolder = new QAction("Show Location", this);

  connect(m_ActionSetDataFolder, &QAction::triggered, dream3dApp, &SIMPLViewApplication::listenSetDataFolderTriggered);
  connect(m_ActionShowDataFolder, &QAction::triggered, dream3dApp, &SIMPLViewApplication::listenShowDataFolderTriggered);

  m_MenuHelp->addSeparator();
  m_MenuHelp->addMenu(m_MenuDataDirectory);
  m_MenuDataDirectory->addAction(m_ActionSetDataFolder);
  m_MenuDataDirectory->addAction(m_ActionShowDataFolder);
  #endif

  m_MenuHelp->addSeparator();
  m_MenuHelp->addAction(m_ActionAboutSIMPLView);
  m_MenuHelp->addAction(m_ActionPluginInformation);

  setMenuBar(m_SIMPLViewMenu);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::connectSignalsSlots()
{
  QAbstractItemView* abstractItemView = getAbstractPipelineView();
  PipelineView* pipelineView = getPipelineView();
  PipelineModel* pipelineModel = pipelineView->getPipelineModel();

  /* Documentation Requester connections */
  DocRequestManager* docRequester = DocRequestManager::Instance();

  connect(abstractItemView, &QAbstractItemView::customContextMenuRequested, this, &SIMPLView_UI::requestContextMenu);

  connect(docRequester, SIGNAL(showFilterDocs(const QString&)), this, SLOT(showFilterHelp(const QString&)));
  connect(docRequester, SIGNAL(showFilterDocUrl(const QUrl&)), this, SLOT(showFilterHelpUrl(const QUrl&)));

  /* Filter Library Widget Connections */
  connect(m_Ui->filterLibraryWidget, &FilterLibraryToolboxWidget::filterItemDoubleClicked, [=] (const QString &filterName, int insertIndex) {
    pipelineView->addFilterFromClassName(filterName, pipelineModel->getActivePipeline(), insertIndex);
  });

  /* Filter List Widget Connections */
  connect(m_Ui->filterListWidget, &FilterListToolboxWidget::filterItemDoubleClicked, [=] (const QString &filterName, int insertIndex) {
    pipelineView->addFilterFromClassName(filterName, pipelineModel->getActivePipeline(), insertIndex);
  });

  /* Bookmarks Widget Connections */
  connect(m_Ui->bookmarksWidget, &BookmarksToolboxWidget::bookmarkActivated, this, &SIMPLView_UI::activateBookmark);

  connect(m_Ui->bookmarksWidget, SIGNAL(updateStatusBar(const QString&)), this, SLOT(setStatusBarMessage(const QString&)));

  connect(m_Ui->bookmarksWidget, &BookmarksToolboxWidget::raiseBookmarksDockWidget, [=] { showDockWidget(m_Ui->bookmarksDockWidget); });

  /* Pipeline View Connections */
  connect(abstractItemView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SIMPLView_UI::filterSelectionChanged);

  connect(pipelineModel, &PipelineModel::filterParametersChanged, m_Ui->dataBrowserWidget, &DataStructureWidget::filterActivated);
  connect(pipelineModel, &PipelineModel::statusMessage, [=](const QString& msg) { statusBar()->showMessage(msg); });
  connect(pipelineModel, &PipelineModel::stdOutMessage, [=](const QString& msg) { addStdOutputMessage(msg); });
  connect(pipelineModel, &PipelineModel::clearIssuesTriggered, m_Ui->issuesWidget, &IssuesWidget::clearIssues);

  connect(pipelineModel, &PipelineModel::activePipelineUpdated, this, &SIMPLView_UI::handleActivePipelineUpdated);

  connect(pipelineView->getPipelineViewController(), &PipelineViewController::clearIssuesTriggered, m_Ui->issuesWidget, &IssuesWidget::clearIssues);

  connect(pipelineView->getPipelineViewController(), &PipelineViewController::statusMessage, [=](const QString& msg) { statusBar()->showMessage(msg); });
  connect(pipelineView->getPipelineViewController(), &PipelineViewController::stdOutMessage, [=](const QString& msg) { addStdOutputMessage(msg); });
  connect(pipelineView->getPipelineViewController(), &PipelineViewController::errorMessage, [=](const QString& msg) { addStdOutputMessage(msg, QColor(255, 191, 193)); });

  connect(pipelineView->getPipelineViewController(), &PipelineViewController::pipelineSavedAs, this, &SIMPLView_UI::handlePipelineSaved);

  connect(pipelineView->getPipelineViewController(), &PipelineViewController::pipelineChanged, this, &SIMPLView_UI::handlePipelineChanges);

  // Connection that displays issues in the Issue Table when the preflight is finished
  connect(pipelineView->getPipelineViewController(), &PipelineViewController::preflightFinished, [=](FilterPipeline::Pointer pipeline, int err) {
    m_Ui->dataBrowserWidget->refreshData();
    m_Ui->issuesWidget->displayCachedMessages();
  });

//  connect(pipelineView, &SVPipelineListView::filterInputWidgetNeedsCleared, this, &SIMPLView_UI::clearFilterInputWidget);

//  connect(pipelineView, &SVPipelineListView::filePathOpened, [=](const QString& filePath) { m_LastOpenedFilePath = filePath; });
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::connectDockWidgetSignalsSlots(QDockWidget* dockWidget)
{
  connect(dockWidget, &QDockWidget::dockLocationChanged, [=] { writeWindowSettings(); });
  connect(dockWidget, &QDockWidget::topLevelChanged, [=] { writeWindowSettings(); });
  connect(dockWidget, &QDockWidget::visibilityChanged, [=] { writeWindowSettings(); });
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::handleActivePipelineUpdated(const QModelIndex &pipelineRootIndex)
{
  PipelineView* pipelineView = getPipelineView();
  if (pipelineView)
  {
    PipelineModel* pipelineModel = pipelineView->getPipelineModel();
    if (pipelineModel)
    {
      QTextEdit* stdOutTextEdit = pipelineModel->standardOutputTextEdit(pipelineRootIndex);
    }
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::requestContextMenu(const QPoint& pos)
{
  activateWindow();

  PipelineView* pipelineView = getPipelineView();
  QAbstractItemView* abstractItemView = getAbstractPipelineView();
  PipelineModel* pipelineModel = pipelineView->getPipelineModel();

  QModelIndex index = abstractItemView->indexAt(pos);
  QPoint mapped;

  PipelineItem::ItemType itemType = static_cast<PipelineItem::ItemType>(pipelineModel->data(index, PipelineModel::ItemTypeRole).toInt());
  if(itemType == PipelineItem::ItemType::Filter || itemType == PipelineItem::ItemType::PipelineRoot)
  {
    mapped = abstractItemView->viewport()->mapToGlobal(pos);
  }
  else
  {
    mapped = mapToGlobal(pos);
  }

  PipelineViewController* viewController = pipelineView->getPipelineViewController();

  if(viewController)
  {
    QMenu menu;
    if (index.isValid() == false)
    {
      viewController->getDefaultContextMenu(menu);
    }
    else if(itemType == PipelineItem::ItemType::Filter)
    {
      viewController->getFilterItemContextMenu(menu, index);
    }
    else if(itemType == PipelineItem::ItemType::PipelineRoot)
    {
      viewController->getPipelineItemContextMenu(menu, index);
    }
    else
    {
      viewController->getDefaultContextMenu(menu);
    }

    menu.exec(mapped);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::showDockWidget(QDockWidget* dockWidget)
{
  if (tabifiedDockWidgets(dockWidget).isEmpty() && dockWidget->toggleViewAction()->isChecked() == false)
  {
    dockWidget->toggleViewAction()->trigger();
  }

  dockWidget->raise();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool SIMPLView_UI::openPipeline(const QString& filePath)
{
  PipelineView* pipelineView = getPipelineView();
  return pipelineView->openPipeline(filePath);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::handlePipelineChanges(FilterPipeline::Pointer pipeline)
{
  QAbstractItemView* abstractItemView = getAbstractPipelineView();

  QModelIndexList selectedIndexes = abstractItemView->selectionModel()->selectedRows();
  qSort(selectedIndexes);

  if (selectedIndexes.size() != 1)
  {
    m_Ui->dataBrowserWidget->filterActivated(AbstractFilter::NullPointer());
  }
  else
  {
    QModelIndex selectedIndex = selectedIndexes[0];
    PipelineView* pipelineView = getPipelineView();
    PipelineModel* pipelineModel = pipelineView->getPipelineModel();

    AbstractFilter::Pointer filter = pipelineModel->filter(selectedIndex);

    if (pipeline->getFilterContainer().contains(filter))
    {
      m_Ui->dataBrowserWidget->filterActivated(filter);
    }
  }

  pipeline->preflightPipeline();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::handlePipelineSaved(const QModelIndex &pipelineRootIndex, const QString &filePath)
{
  Q_UNUSED(pipelineRootIndex)

  QMessageBox bookmarkMsgBox(this);
  bookmarkMsgBox.setWindowTitle("Pipeline Saved");
  bookmarkMsgBox.setText("The pipeline has been saved.");
  bookmarkMsgBox.setInformativeText("Would you also like to bookmark this pipeline?");
  bookmarkMsgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
  bookmarkMsgBox.setDefaultButton(QMessageBox::Yes);
  int ret = bookmarkMsgBox.exec();

  if(ret == QMessageBox::Yes)
  {
    m_Ui->bookmarksWidget->getBookmarksTreeView()->addBookmark(filePath, QModelIndex());
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::setLoadedPlugins(QVector<ISIMPLibPlugin*> plugins)
{
  m_LoadedPlugins = plugins;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QMessageBox::StandardButton SIMPLView_UI::checkDirtyDocument()
{

  if(this->isWindowModified() == true)
  {
    int r = QMessageBox::warning(this, BrandedStrings::ApplicationName, tr("The Pipeline has been modified.\nDo you want to save your changes?"), QMessageBox::Save | QMessageBox::Default,
                                 QMessageBox::Discard, QMessageBox::Cancel | QMessageBox::Escape);
    if(r == QMessageBox::Save)
    {
      if(savePipeline() == true)
      {
        return QMessageBox::Save;
      }
      else
      {
        return QMessageBox::Cancel;
      }
    }
    else if(r == QMessageBox::Discard)
    {
      return QMessageBox::Discard;
    }
    else if(r == QMessageBox::Cancel)
    {
      return QMessageBox::Cancel;
    }
  }

  return QMessageBox::Ignore;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::populateMenus(QObject* plugin)
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::processPipelineMessage(const PipelineMessage& msg)
{
  if(msg.getType() == PipelineMessage::MessageType::StatusMessageAndProgressValue && nullptr != this->statusBar())
  {
    this->statusBar()->showMessage(msg.generateStatusString());
  }
  else if(msg.getType() == PipelineMessage::MessageType::StandardOutputMessage || msg.getType() == PipelineMessage::MessageType::StatusMessage)
  {
    if(msg.getType() == PipelineMessage::MessageType::StatusMessage)
    {
      if(nullptr != this->statusBar())
      {
        this->statusBar()->showMessage(msg.generateStatusString());
      }
    }

    // Allow status messages to open the standard output widget
    if(SIMPLView::DockWidgetSettings::HideDockSetting::OnStatusAndError == StandardOutputWidget::GetHideDockSetting())
    {
      m_Ui->stdOutDockWidget->setVisible(true);
    }

    // Allow status messages to open the issuesDockWidget as well
    if(SIMPLView::DockWidgetSettings::HideDockSetting::OnStatusAndError == IssuesWidget::GetHideDockSetting())
    {
      m_Ui->issuesDockWidget->setVisible(true);
    }

    QString text = "<span style=\" color:#000000;\" >";
    text.append(msg.getText());
    text.append("</span>");
    m_Ui->stdOutWidget->appendText(text);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::showFilterHelp(const QString& className)
{
// Launch the dialog
#ifdef SIMPL_USE_QtWebEngine
  SVUserManualDialog::LaunchHelpDialog(className);
#else
  QUrl helpURL = URL_GENERATOR::GenerateHTMLUrl(className);
  bool didOpen = QDesktopServices::openUrl(helpURL);
  if(false == didOpen)
  {
    QMessageBox msgBox;
    msgBox.setText(QString("Error Opening Help File"));
    msgBox.setInformativeText(QString::fromLatin1("SIMPLView could not open the help file path ") + helpURL.path());
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();
  }
#endif
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::showFilterHelpUrl(const QUrl& helpURL)
{
#ifdef SIMPL_USE_QtWebEngine
  SVUserManualDialog::LaunchHelpDialog(helpURL);
#else
  bool didOpen = QDesktopServices::openUrl(helpURL);
  if(false == didOpen)
  {
    QMessageBox msgBox;
    msgBox.setText(QString("Error Opening Help File"));
    msgBox.setInformativeText(QString::fromLatin1("SIMPLView could not open the help file path ") + helpURL.path());
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();
  }
#endif
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
DataStructureWidget* SIMPLView_UI::getDataStructureWidget()
{
  return m_Ui->dataBrowserWidget;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
PipelineView* SIMPLView_UI::getPipelineView()
{
  return m_PipelineView;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QAbstractItemView* SIMPLView_UI::getAbstractPipelineView()
{
  return m_AbstractPipelineView;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::filterSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  PipelineView* pipelineView = getPipelineView();
  QAbstractItemView* abstractView = getAbstractPipelineView();
  PipelineModel* pipelineModel = pipelineView->getPipelineModel();

  QModelIndexList selectedIndexes = abstractView->selectionModel()->selectedRows();
  qSort(selectedIndexes);

  if(selectedIndexes.size() == 1)
  {
    QModelIndex selectedIndex = selectedIndexes[0];

    FilterInputWidget* fiw = pipelineModel->filterInputWidget(selectedIndex);
    setFilterInputWidget(fiw);

    AbstractFilter::Pointer filter = pipelineModel->filter(selectedIndex);
    m_Ui->dataBrowserWidget->filterActivated(filter);
  }
  else
  {
    clearFilterInputWidget();
    m_Ui->dataBrowserWidget->filterActivated(AbstractFilter::NullPointer());
  }

  pipelineView->getPipelineViewController()->setCutCopyEnabled(abstractView->selectionModel()->selectedRows().size() > 0);

  QModelIndex index = abstractView->currentIndex();
  PipelineItem::ItemType itemType = static_cast<PipelineItem::ItemType>(pipelineModel->data(index, PipelineModel::ItemTypeRole).toInt());
  if(itemType == PipelineItem::ItemType::PipelineRoot && index != pipelineModel->getActivePipeline())
  {
    pipelineModel->updateActivePipeline(index);
  }
  else if (index.parent().isValid() && index.parent() != pipelineModel->getActivePipeline())
  {
    pipelineModel->updateActivePipeline(index.parent());
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::setFilterInputWidget(FilterInputWidget* widget)
{
  if(m_FilterInputWidget)
  {
    emit m_FilterInputWidget->endPathFiltering();
    emit m_FilterInputWidget->endViewPaths();
    emit m_FilterInputWidget->endDataStructureFiltering();
  }

  // Clear the filter input widget
  clearFilterInputWidget();

  if(widget == nullptr)
  {
    return;
  }

  // Alert to DataArrayPath requirements
  connect(widget, SIGNAL(viewPathsMatchingReqs(DataContainerSelectionFilterParameter::RequirementType)), getDataStructureWidget(),
          SLOT(setViewReqs(DataContainerSelectionFilterParameter::RequirementType)), Qt::ConnectionType::UniqueConnection);
  connect(widget, SIGNAL(viewPathsMatchingReqs(AttributeMatrixSelectionFilterParameter::RequirementType)), getDataStructureWidget(),
          SLOT(setViewReqs(AttributeMatrixSelectionFilterParameter::RequirementType)), Qt::ConnectionType::UniqueConnection);
  connect(widget, SIGNAL(viewPathsMatchingReqs(DataArraySelectionFilterParameter::RequirementType)), getDataStructureWidget(), SLOT(setViewReqs(DataArraySelectionFilterParameter::RequirementType)),
          Qt::ConnectionType::UniqueConnection);
  connect(widget, SIGNAL(endViewPaths()), getDataStructureWidget(), SLOT(clearViewRequirements()), Qt::ConnectionType::UniqueConnection);
  connect(getDataStructureWidget(), SIGNAL(filterPath(DataArrayPath)), widget, SIGNAL(filterPath(DataArrayPath)), Qt::ConnectionType::UniqueConnection);
  connect(getDataStructureWidget(), SIGNAL(endDataStructureFiltering()), widget, SIGNAL(endDataStructureFiltering()), Qt::ConnectionType::UniqueConnection);
  connect(getDataStructureWidget(), SIGNAL(applyPathToFilteringParameter(DataArrayPath)), widget, SIGNAL(applyPathToFilteringParameter(DataArrayPath)));

  emit widget->endPathFiltering();

  // Set the widget into the frame
  m_Ui->fiwFrameVLayout->addWidget(widget);
  m_FilterInputWidget = widget;
  widget->show();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::clearFilterInputWidget()
{
  QLayoutItem* item = m_Ui->fiwFrameVLayout->takeAt(0);
  if(item)
  {
    QWidget* w = item->widget();
    if(w)
    {
      w->hide();
      w->setParent(nullptr);
    }
  }

  m_FilterInputWidget = nullptr;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::markDocumentAsDirty()
{
  setWindowModified(true);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::issuesTableHasErrors(bool hasErrors, int errCount, int warnCount)
{
  Q_UNUSED(errCount)
  Q_UNUSED(warnCount)

  SIMPLView::DockWidgetSettings::HideDockSetting errorTableSetting = IssuesWidget::GetHideDockSetting();
  if(SIMPLView::DockWidgetSettings::HideDockSetting::OnError == errorTableSetting 
     || SIMPLView::DockWidgetSettings::HideDockSetting::OnStatusAndError == errorTableSetting)
  {
    m_Ui->issuesDockWidget->setVisible(hasErrors);
  }

  SIMPLView::DockWidgetSettings::HideDockSetting stdOutSetting = StandardOutputWidget::GetHideDockSetting();
  if(SIMPLView::DockWidgetSettings::HideDockSetting::OnError == stdOutSetting 
     || SIMPLView::DockWidgetSettings::HideDockSetting::OnStatusAndError == stdOutSetting)
  {
    m_Ui->stdOutDockWidget->setVisible(hasErrors);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::setStatusBarMessage(const QString& msg)
{
  m_Ui->statusbar->showMessage(msg);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::addStdOutputMessage(const QString& msg, QColor textColor)
{
  QString text = tr("<span style=\" color:%1;\" >").arg(textColor.name());
  text.append(msg);
  text.append("</span>");
  m_Ui->stdOutWidget->appendText(text);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::changeEvent(QEvent* event)
{
  if(event->type() == QEvent::ActivationChange)
  {
    emit dream3dWindowChangedState(this);
  }
}
