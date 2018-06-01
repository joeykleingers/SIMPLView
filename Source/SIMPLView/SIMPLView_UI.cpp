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
#include <QtGui/QClipboard>
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

#include "SVWidgetsLib/Animations/PipelineItemBorderSizeAnimation.h"
#include "SVWidgetsLib/Core/FilterWidgetManager.h"
#include "SVWidgetsLib/Dialogs/AboutPlugins.h"
#include "SVWidgetsLib/QtSupport/QtSMacros.h"
#include "SVWidgetsLib/QtSupport/QtSPluginFrame.h"
#include "SVWidgetsLib/QtSupport/QtSRecentFileList.h"
#include "SVWidgetsLib/Widgets/SVStyle.h"
#include "SVWidgetsLib/Animations/PipelineItemBorderSizeAnimation.h"
#include "SVWidgetsLib/Core/FilterWidgetManager.h"
#include "SVWidgetsLib/Dialogs/AboutPlugins.h"
#include "SVWidgetsLib/Widgets/util/AddFilterCommand.h"
#include "SVWidgetsLib/Widgets/BookmarksModel.h"
#include "SVWidgetsLib/Widgets/BookmarksToolboxWidget.h"
#include "SVWidgetsLib/Widgets/BookmarksTreeView.h"
#include "SVWidgetsLib/Widgets/FilterLibraryToolboxWidget.h"
#include "SVWidgetsLib/Widgets/PipelineItemDelegate.h"
#include "SVWidgetsLib/Widgets/PipelineListWidget.h"
#include "SVWidgetsLib/Widgets/PipelineModel.h"
#include "SVWidgetsLib/Widgets/StatusBarWidget.h"
#include "SVWidgetsLib/Widgets/util/AddFilterCommand.h"
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
#include "SIMPLView/SIMPLViewPipelineDockWidget.h"

#include "BrandedStrings.h"

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
SIMPLView_UI::SIMPLView_UI(QWidget* parent)
  : QMainWindow(parent)
  , m_Ui(new Ui::SIMPLView_UI)
  , m_FilterManager(nullptr)
  , m_FilterWidgetManager(nullptr)
  , m_OpenedFilePath("")
{
  m_OpenedFilePath = QDir::homePath();

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
  if(HideDockSetting::OnError == m_HideErrorTable)
  {
    m_Ui->issuesDockWidget->setHidden(true);
  }
  if(HideDockSetting::OnError == m_HideStdOutput)
  {
    m_Ui->stdOutDockWidget->setHidden(true);
  }

  // Set window modified to false
  setWindowModified(false);
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
  QSharedPointer<QtSSettings> prefs = QSharedPointer<QtSSettings>(new QtSSettings());
  writeWindowSettings(prefs.data());
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::listenSavePipelineTriggered()
{
  savePipeline(m_ActivePipelineDockWidget);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool SIMPLView_UI::savePipeline(SIMPLViewPipelineDockWidget* dockWidget)
{
  if(dockWidget->isWindowModified() == true)
  {
    QString filePath;
    if(dockWidget->windowFilePath().isEmpty())
    {
      // When the file hasn't been saved before, the same functionality as a "Save As" occurs...
      return savePipelineAs(dockWidget);
    }
    else
    {
      filePath = m_OpenedFilePath;
    }

    // Fix the separators
    filePath = QDir::toNativeSeparators(filePath);

    // Write the pipeline
    SVPipelineView* viewWidget = dockWidget->getPipelineListWidget()->getPipelineView();
    viewWidget->writePipeline(filePath);

    // Set window title and save flag
    QFileInfo prefFileInfo = QFileInfo(filePath);
    dockWidget->setWindowTitle("[*]" + prefFileInfo.baseName());
    dockWidget->setWindowModified(false);

    // Add file to the recent files list
    QtSRecentFileList* list = QtSRecentFileList::Instance();
    list->addFile(filePath);
  }

  return true;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::listenSavePipelineAsTriggered()
{
  savePipelineAs(m_ActivePipelineDockWidget);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool SIMPLView_UI::savePipelineAs(SIMPLViewPipelineDockWidget* dockWidget)
{
  QString proposedFile = m_OpenedFilePath + QDir::separator() + "Untitled.json";
  QString filePath = QFileDialog::getSaveFileName(this, tr("Save Pipeline To File"), proposedFile, tr("Json File (*.json);;SIMPLView File (*.dream3d);;All Files (*.*)"));
  if(true == filePath.isEmpty())
  {
    return false;
  }

  filePath = QDir::toNativeSeparators(filePath);

  // If the filePath already exists - delete it so that we get a clean write to the file
  QFileInfo fi(filePath);
  if(fi.suffix().isEmpty())
  {
    filePath.append(".json");
    fi.setFile(filePath);
  }

  // Write the pipeline
  SVPipelineView* viewWidget = dockWidget->getPipelineListWidget()->getPipelineView();
  int err = viewWidget->writePipeline(filePath);

  if(err >= 0)
  {
    // Set window title and save flag
    dockWidget->setWindowTitle("[*]" + fi.baseName());
    dockWidget->setWindowModified(false);

    // Add file to the recent files list
    QtSRecentFileList* list = QtSRecentFileList::Instance();
    list->addFile(filePath);

    m_OpenedFilePath = filePath;
  }
  else
  {
    return false;
  }

  // Cache the last directory
  m_OpenedFilePath = filePath;

  QMessageBox pipelineSavedMsgBox(this);
  pipelineSavedMsgBox.setWindowTitle("Pipeline Saved");
  pipelineSavedMsgBox.setText("The pipeline has been saved.");
  pipelineSavedMsgBox.setInformativeText("Would you also like to bookmark this pipeline?");
  pipelineSavedMsgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
  pipelineSavedMsgBox.setDefaultButton(QMessageBox::Yes);
  int ret = pipelineSavedMsgBox.exec();

  if(ret == QMessageBox::Yes)
  {
    m_Ui->bookmarksWidget->getBookmarksTreeView()->addBookmark(filePath, QModelIndex());
  }

  return true;
}

// -----------------------------------------------------------------------------
//  Called when the main window is closed.
// -----------------------------------------------------------------------------
void SIMPLView_UI::closeEvent(QCloseEvent* event)
{
  for (int i = 0; i < m_PipelineDockWidgets.size(); i++)
  {
    PipelineListWidget* listWidget = m_PipelineDockWidgets[i]->getPipelineListWidget();
    if(listWidget->getPipelineView()->isPipelineCurrentlyRunning() == true)
    {
      QMessageBox runningPipelineBox;
      runningPipelineBox.setWindowTitle("Pipeline is Running");
      runningPipelineBox.setText("There is a pipeline currently running.\nPlease cancel the running pipeline and try again.");
      runningPipelineBox.setStandardButtons(QMessageBox::Ok);
      runningPipelineBox.setIcon(QMessageBox::Warning);
      runningPipelineBox.exec();
      event->ignore();
      return;
    }
  }

  while (m_PipelineDockWidgets.size() > 0)
  {
    bool success = removePipeline(m_PipelineDockWidgets[0]);
    if(success == false)
    {
      event->ignore();
      return;
    }
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
  readWindowSettings(prefs.data());
  readVersionSettings(prefs.data());

  // Read dock widget settings
  prefs->beginGroup("DockWidgetSettings");

  prefs->beginGroup("Issues Dock Widget");
  readDockWidgetSettings(prefs.data(), m_Ui->issuesDockWidget);
  readHideDockSettings(prefs.data(), m_HideErrorTable);
  prefs->endGroup();

  prefs->beginGroup("Standard Output Dock Widget");
  readDockWidgetSettings(prefs.data(), m_Ui->stdOutDockWidget);
  readHideDockSettings(prefs.data(), m_HideStdOutput);
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
void SIMPLView_UI::readWindowSettings(QtSSettings* prefs)
{
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
  //  restoreDockWidget(dw);

  //  QString name = dw->objectName();
  //  bool b = prefs->value(dw->objectName(), QVariant(false)).toBool();
  //  dw->setHidden(b);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::readHideDockSettings(QtSSettings* prefs, HideDockSetting& value)
{
  int showError = static_cast<int>(HideDockSetting::Ignore);
  int hideDockSetting = prefs->value("Show / Hide On Error", QVariant(showError)).toInt();
  value = static_cast<HideDockSetting>(hideDockSetting);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::readVersionSettings(QtSSettings* prefs)
{
}

// -----------------------------------------------------------------------------
//  Write our Prefs to file
// -----------------------------------------------------------------------------
void SIMPLView_UI::writeSettings()
{
  QSharedPointer<QtSSettings> prefs = QSharedPointer<QtSSettings>(new QtSSettings());

  // Have the pipeline builder write its settings to the prefs file
  writeWindowSettings(prefs.data());
  // Have the version check widet write its preferences.
  writeVersionCheckSettings(prefs.data());

  prefs->beginGroup("DockWidgetSettings");

  prefs->beginGroup("Issues Dock Widget");
  writeDockWidgetSettings(prefs.data(), m_Ui->issuesDockWidget);
  writeHideDockSettings(prefs.data(), m_HideErrorTable);
  prefs->endGroup();

  prefs->beginGroup("Standard Output Dock Widget");
  writeDockWidgetSettings(prefs.data(), m_Ui->stdOutDockWidget);
  writeHideDockSettings(prefs.data(), m_HideStdOutput);
  prefs->endGroup();

  prefs->endGroup();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::writeVersionCheckSettings(QtSSettings* prefs)
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::writeWindowSettings(QtSSettings* prefs)
{
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
void SIMPLView_UI::writeHideDockSettings(QtSSettings* prefs, HideDockSetting value)
{
  int valuei = static_cast<int>(value);
  prefs->setValue("Show / Hide On Error", valuei);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
SIMPLViewPipelineDockWidget* SIMPLView_UI::addPipeline()
{
  SIMPLViewPipelineDockWidget* dockWidget = new SIMPLViewPipelineDockWidget(this);

  m_PipelineDockWidgets.push_back(dockWidget);

  insertPipelineDockWidget(dockWidget);

  connectPipelineSignalsSlots(dockWidget);

  if (!m_ActivePipelineDockWidget)
  {
    setPipelineDockWidgetAsActive(dockWidget);
  }

  return dockWidget;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::insertPipelineDockWidget(SIMPLViewPipelineDockWidget* pipelineDockWidget)
{
  QList<SIMPLViewPipelineDockWidget*> pipelineDockWidgets = findChildren<SIMPLViewPipelineDockWidget*>();
  bool dockWidgetAdded = false;
  if (pipelineDockWidgets.size() > 0)
  {
    for (int i = 0; i < pipelineDockWidgets.size() && dockWidgetAdded == false; i++)
    {
      SIMPLViewPipelineDockWidget* dw = pipelineDockWidgets[i];
      Qt::DockWidgetArea dockWidgetArea = this->dockWidgetArea(dw);
      if (dockWidgetArea != Qt::NoDockWidgetArea && dockWidgetArea != Qt::AllDockWidgetAreas)
      {
        tabifyDockWidget(dw, pipelineDockWidget);
        dockWidgetAdded = true;
      }
    }
  }

  if (dockWidgetAdded == false)
  {
    addDockWidget(Qt::LeftDockWidgetArea, pipelineDockWidget);
  }

  if (!m_EndOfViewMenuSeparator)
  {
    m_MenuView->addAction(pipelineDockWidget->toggleViewAction());
    m_EndOfViewMenuSeparator = m_MenuView->addSeparator();
  }
  else
  {
    m_MenuView->insertAction(m_EndOfViewMenuSeparator, pipelineDockWidget->toggleViewAction());
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::connectPipelineSignalsSlots(SIMPLViewPipelineDockWidget* pipelineDockWidget)
{
  PipelineListWidget* pipelineListWidget = pipelineDockWidget->getPipelineListWidget();
  SVPipelineView* pipelineView = pipelineListWidget->getPipelineView();

  connect(pipelineView, &SVPipelineView::pipelineDeleting, [=] {
    removePipeline(pipelineDockWidget);
  });

  // Connection that displays issues in the Issue Table when the preflight is finished
  connect(pipelineView, &SVPipelineView::preflightFinished, [=] (FilterPipeline::Pointer pipeline, int err) {
    if (pipelineDockWidget == m_ActivePipelineDockWidget)
    {
      m_Ui->dataBrowserWidget->refreshData();

      QVector<PipelineMessage> messages = m_ActivePipelineDockWidget->getPipelineListWidget()->getPipelineView()->getCurrentIssues();
      m_Ui->issuesWidget->displayCachedMessages(messages);
    }

    pipelineListWidget->preflightFinished(pipeline, err);
  });

  // This connection fires when a pipeline has been activated.  It handles clearing all the other pipelines' selections so that only one pipeline
  // has selections at any one time.  We can't do this in the "selectionChanged" connection because it would cause recursion.
  connect(pipelineDockWidget->getPipelineListWidget()->getPipelineView(), &SVPipelineView::pipelineActivated, [=] {
    QList<SIMPLViewPipelineDockWidget*> pipelineDockWidgets = findChildren<SIMPLViewPipelineDockWidget*>();
    for (int i = 0; i < pipelineDockWidgets.size(); i++)
    {
      SIMPLViewPipelineDockWidget* otherPipelineDockWidget = pipelineDockWidgets[i];
      if (otherPipelineDockWidget != pipelineDockWidget)
      {
        otherPipelineDockWidget->getPipelineListWidget()->getPipelineView()->clearSelection();
      }
    }

    if (pipelineDockWidget != m_ActivePipelineDockWidget)
    {
      setPipelineDockWidgetAsActive(pipelineDockWidget);
    }
  });

  connect(pipelineDockWidget->getPipelineListWidget()->getPipelineView()->selectionModel(), &QItemSelectionModel::selectionChanged, [=] {
    QModelIndexList selectedIndexes = pipelineView->selectionModel()->selectedRows();
    qSort(selectedIndexes);

    if (selectedIndexes.size() == 1)
    {
      QModelIndex selectedIndex = selectedIndexes[0];

      PipelineModel* model = pipelineView->getPipelineModel();
      FilterInputWidget* fiw = model->filterInputWidget(selectedIndex);
      setFilterInputWidget(fiw);

      AbstractFilter::Pointer filter = model->filter(selectedIndex);
      m_Ui->dataBrowserWidget->filterActivated(filter);
    }
    else
    {
      clearFilterInputWidget();
      m_Ui->dataBrowserWidget->filterActivated(AbstractFilter::NullPointer());
    }
  });

  connect(pipelineView, &SVPipelineView::pipelineFinished, pipelineListWidget, &PipelineListWidget::pipelineFinished);
  connect(pipelineView, &SVPipelineView::pipelineFinished, this, &SIMPLView_UI::pipelineDidFinish);

  connect(pipelineDockWidget, &SIMPLViewPipelineDockWidget::visibilityChanged, [=] (bool visible) {
    if (visible == true && tabifiedDockWidgets(pipelineDockWidget).isEmpty() == false)
    {
      setPipelineDockWidgetAsActive(pipelineDockWidget);
    }
  });
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool SIMPLView_UI::removePipeline(SIMPLViewPipelineDockWidget* dockWidget)
{
  QMessageBox::StandardButton choice = checkDirtyPipeline(dockWidget);
  if(choice == QMessageBox::Cancel)
  {
    return false;
  }

  m_MenuView->removeAction(dockWidget->toggleViewAction());

  m_PipelineDockWidgets.erase(std::find(m_PipelineDockWidgets.begin(), m_PipelineDockWidgets.end(), dockWidget));

  if (dockWidget == m_ActivePipelineDockWidget)
  {
    QList<SIMPLViewPipelineDockWidget*> pipelineDockWidgets = findChildren<SIMPLViewPipelineDockWidget*>();
    bool pipelineActivated = false;
    for (int i = 0; i < pipelineDockWidgets.size() && !pipelineActivated; i++)
    {
      SIMPLViewPipelineDockWidget* pipelineDockWidget = pipelineDockWidgets[i];
      if (pipelineDockWidget != dockWidget)
      {
        setPipelineDockWidgetAsActive(pipelineDockWidget);
        pipelineActivated = true;
      }
    }

    if (!pipelineActivated)
    {
      setPipelineDockWidgetAsActive(nullptr);
    }
  }

  delete dockWidget;
  return true;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::setupGui()
{
  setWindowTitle(BrandedStrings::ApplicationName);

  createSIMPLViewMenuSystem();

  addPipeline();
  
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
  m_ActionClearCache = new QAction("Clear Cache", this);

  m_ActionCut = new QAction("Cut", this);
  m_ActionCopy = new QAction("Copy", this);
  m_ActionPaste = new QAction("Paste", this);
  m_ActionClearPipeline = new QAction("Clear Pipeline", this);

  m_ActionUndo = new QAction("Undo", this);
  m_ActionRedo = new QAction("Redo", this);

  m_ActionCut->setDisabled(true);
  m_ActionCopy->setDisabled(true);

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
  m_ActionSaveAs->setShortcut(QKeySequence::SaveAs);
  m_ActionExit->setShortcut(QKeySequence::Quit);
  m_ActionCheckForUpdates->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));
  m_ActionShowSIMPLViewHelp->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H));
  m_ActionPluginInformation->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_I));

  m_ActionCut->setShortcut(QKeySequence::Cut);
  m_ActionCopy->setShortcut(QKeySequence::Copy);
  m_ActionPaste->setShortcut(QKeySequence::Paste);
  m_ActionClearPipeline->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Backspace));

  m_ActionUndo->setShortcut(QKeySequence::Undo);
  m_ActionRedo->setShortcut(QKeySequence::Redo);

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

  // Create Edit Menu.  This menu will be filled by the currently active pipeline dock widget.
  m_SIMPLViewMenu->addMenu(m_MenuEdit);
  m_MenuEdit->addAction(m_ActionUndo);
  m_MenuEdit->addAction(m_ActionRedo);
  m_MenuEdit->addSeparator();
  m_MenuEdit->addAction(m_ActionCut);
  m_MenuEdit->addAction(m_ActionCopy);
  m_MenuEdit->addAction(m_ActionPaste);

  // Create View Menu
  m_SIMPLViewMenu->addMenu(m_MenuView);
  m_MenuView->addAction(m_Ui->filterListDockWidget->toggleViewAction());
  m_MenuView->addAction(m_Ui->filterLibraryDockWidget->toggleViewAction());
  m_MenuView->addAction(m_Ui->bookmarksDockWidget->toggleViewAction());
  m_MenuView->addAction(m_Ui->issuesDockWidget->toggleViewAction());
  m_MenuView->addAction(m_Ui->stdOutDockWidget->toggleViewAction());
  m_MenuView->addAction(m_Ui->dataBrowserDockWidget->toggleViewAction());
  m_MenuView->addSeparator();

  // Create Bookmarks Menu
  m_SIMPLViewMenu->addMenu(m_MenuBookmarks);
  m_MenuBookmarks->addAction(actionAddBookmark);
  m_MenuBookmarks->addSeparator();
  m_MenuBookmarks->addAction(actionNewFolder);

  // Create Pipeline Menu. This menu will be filled by the currently active pipeline dock widget.
  m_SIMPLViewMenu->addMenu(m_MenuPipeline);
  m_MenuPipeline->addAction(m_ActionClearPipeline);

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
  m_MenuHelp->addSeparator();
  m_MenuHelp->addAction(m_ActionAboutSIMPLView);
  m_MenuHelp->addAction(m_ActionPluginInformation);

  setMenuBar(m_SIMPLViewMenu);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::connectActivePipelineSignalsSlots(SIMPLViewPipelineDockWidget* dockWidget)
{
  PipelineListWidget* pipelineListWidget = dockWidget->getPipelineListWidget();
  SVPipelineView* pipelineView = pipelineListWidget->getPipelineView();
  PipelineModel* pipelineModel = pipelineView->getPipelineModel();

  /* Filter Library Widget Connections */
  connect(m_Ui->filterLibraryWidget, &FilterLibraryToolboxWidget::filterItemDoubleClicked, pipelineView, &SVPipelineView::addFilterFromClassName);

  /* Filter List Widget Connections */
  connect(m_Ui->filterListWidget, &FilterListToolboxWidget::filterItemDoubleClicked, pipelineView, &SVPipelineView::addFilterFromClassName);

  /* Pipeline View Connections */
  QTextEdit* stdOutTextEdit = pipelineView->getStdOutputTextEdit();
  connect(stdOutTextEdit, &QTextEdit::textChanged, [=] {
    // Allow new messages to open the standard output widget
    if(HideDockSetting::OnStatusAndError == m_HideStdOutput)
    {
      m_Ui->stdOutDockWidget->setVisible(true);
    }
  });

  connect(pipelineView, &SVPipelineView::filterParametersChanged, m_Ui->dataBrowserWidget, &DataStructureWidget::filterActivated);

  connect(pipelineView, &SVPipelineView::clearDataStructureWidgetTriggered, [=] {
    m_Ui->dataBrowserWidget->filterActivated(AbstractFilter::NullPointer());
  });

  connect(pipelineView, &SVPipelineView::filterInputWidgetNeedsCleared, this, &SIMPLView_UI::clearFilterInputWidget);

  connect(pipelineView, &SVPipelineView::displayIssuesTriggered, [=] (QVector<PipelineMessage> messages) {
    m_Ui->issuesWidget->displayCachedMessages(messages);

    // Allow status messages to open the issuesDockWidget as well
    if(HideDockSetting::OnStatusAndError == m_HideErrorTable && messages.size() > 0)
    {
      m_Ui->issuesDockWidget->setVisible(true);
    }
  });

  connect(pipelineView, &SVPipelineView::clearIssuesTriggered, m_Ui->issuesWidget, &IssuesWidget::clearIssues);

  connect(pipelineView, &SVPipelineView::writeSIMPLViewSettingsTriggered, [=] { writeSettings(); });

  connect(pipelineView, &SVPipelineView::pipelineFilePathUpdated, dockWidget, &QDockWidget::setWindowFilePath);

  // Connection that updates the data browser widget when the active pipeline has changed
  connect(pipelineView, &SVPipelineView::pipelineChanged, [=] {
    markActivePipelineAsDirty();

    QModelIndexList selectedIndexes = pipelineView->selectionModel()->selectedRows();
    qSort(selectedIndexes);

    if (selectedIndexes.size() == 1)
    {
      QModelIndex selectedIndex = selectedIndexes[0];
      PipelineModel* model = pipelineView->getPipelineModel();

      AbstractFilter::Pointer filter = model->filter(selectedIndex);
      m_Ui->dataBrowserWidget->filterActivated(filter);
    }
    else
    {
      m_Ui->dataBrowserWidget->filterActivated(AbstractFilter::NullPointer());
    }
  });

  connect(pipelineView->selectionModel(), &QItemSelectionModel::selectionChanged, [=](const QItemSelection& selected, const QItemSelection& deselected) {
    PipelineModel* pipelineModel = pipelineView->getPipelineModel();

    QModelIndexList selectedIndexes = pipelineView->selectionModel()->selectedRows();
    qSort(selectedIndexes);

    // Animate a selection border for selected indexes
    for(const QModelIndex& index : selected.indexes())
    {
      new PipelineItemBorderSizeAnimation(pipelineModel, QPersistentModelIndex(index));
    }

    // Remove selection border from deselected indexes
    for(const QModelIndex& index : deselected.indexes())
    {
      pipelineModel->setData(index, -1, PipelineModel::Roles::BorderSizeRole);
    }

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
  });

  connect(pipelineView, &SVPipelineView::filePathOpened, [=] (const QString &filePath) { m_OpenedFilePath = filePath; });

  connect(pipelineView, SIGNAL(filterInputWidgetEdited()), this, SLOT(markActivePipelineAsDirty()));

  connect(pipelineView, SIGNAL(filterEnabledStateChanged()), this, SLOT(markActivePipelineAsDirty()));

  connect(pipelineView, &SVPipelineView::statusMessage, this, &SIMPLView_UI::setStatusBarMessage);

  /* Pipeline Model Connections */
  connect(pipelineModel, &PipelineModel::pipelineDataChanged, [=] {  });

  connect(m_ActionCut, &QAction::triggered, pipelineView->getActionCut(), &QAction::trigger);
  connect(m_ActionCopy, &QAction::triggered, pipelineView->getActionCopy(), &QAction::trigger);
  connect(m_ActionPaste, &QAction::triggered, pipelineView->getActionPaste(), &QAction::trigger);
  connect(m_ActionClearPipeline, &QAction::triggered, pipelineView->getActionClearPipeline(), &QAction::trigger);

  connect(pipelineView, &SVPipelineView::cutAvailabilityChanged, m_ActionCut, &QAction::setEnabled);
  connect(pipelineView, &SVPipelineView::copyAvailabilityChanged, m_ActionCopy, &QAction::setEnabled);
  connect(pipelineView, &SVPipelineView::pasteAvailabilityChanged, m_ActionPaste, &QAction::setEnabled);
  connect(pipelineView, &SVPipelineView::clearPipelineAvailabilityChanged, m_ActionClearPipeline, &QAction::setEnabled);

  connect(m_ActionUndo, &QAction::triggered, [=] {
    pipelineView->getActionUndo()->trigger();
    m_ActionUndo->setText(pipelineView->getActionUndo()->text());
    m_ActionUndo->setEnabled(pipelineView->getActionUndo()->isEnabled());
    m_ActionRedo->setText(pipelineView->getActionRedo()->text());
    m_ActionRedo->setEnabled(pipelineView->getActionRedo()->isEnabled());
  });
  connect(m_ActionRedo, &QAction::triggered, [=] {
    pipelineView->getActionRedo()->trigger();
    m_ActionUndo->setText(pipelineView->getActionUndo()->text());
    m_ActionUndo->setEnabled(pipelineView->getActionUndo()->isEnabled());
    m_ActionRedo->setText(pipelineView->getActionRedo()->text());
    m_ActionRedo->setEnabled(pipelineView->getActionRedo()->isEnabled());
  });

  connect(pipelineView->getActionUndo(), &QAction::changed, [=] {
    if (m_ActionUndo)
    {
      m_ActionUndo->setText(pipelineView->getActionUndo()->text());
      m_ActionUndo->setEnabled(pipelineView->getActionUndo()->isEnabled());
    }
  });
  connect(pipelineView->getActionRedo(), &QAction::changed, [=] {
    if (m_ActionRedo)
    {
      m_ActionRedo->setText(pipelineView->getActionRedo()->text());
      m_ActionRedo->setEnabled(pipelineView->getActionRedo()->isEnabled());
    }
  });
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::disconnectActivePipelineSignalsSlots(SIMPLViewPipelineDockWidget* dockWidget)
{
  PipelineListWidget* pipelineListWidget = dockWidget->getPipelineListWidget();
  SVPipelineView* pipelineView = pipelineListWidget->getPipelineView();
  PipelineModel* pipelineModel = pipelineView->getPipelineModel();

  /* Filter Library Widget Connections */
  disconnect(m_Ui->filterLibraryWidget, &FilterLibraryToolboxWidget::filterItemDoubleClicked, pipelineView, &SVPipelineView::addFilterFromClassName);

  /* Filter List Widget Connections */
  disconnect(m_Ui->filterListWidget, &FilterListToolboxWidget::filterItemDoubleClicked, pipelineView, &SVPipelineView::addFilterFromClassName);

  /* Pipeline View Connections */
  QTextEdit* stdOutTextEdit = pipelineView->getStdOutputTextEdit();
  disconnect(stdOutTextEdit, &QTextEdit::textChanged, 0, 0);

  disconnect(pipelineView, &SVPipelineView::filterParametersChanged, m_Ui->dataBrowserWidget, &DataStructureWidget::filterActivated);

  disconnect(pipelineView, &SVPipelineView::clearDataStructureWidgetTriggered, 0, 0);

  disconnect(pipelineView, &SVPipelineView::filterInputWidgetNeedsCleared, this, &SIMPLView_UI::clearFilterInputWidget);

  disconnect(pipelineView, &SVPipelineView::displayIssuesTriggered, m_Ui->issuesWidget, &IssuesWidget::displayCachedMessages);

  disconnect(pipelineView, &SVPipelineView::clearIssuesTriggered, m_Ui->issuesWidget, &IssuesWidget::clearIssues);

  disconnect(pipelineView, &SVPipelineView::writeSIMPLViewSettingsTriggered, 0, 0);

  disconnect(pipelineView, &SVPipelineView::pipelineFilePathUpdated, dockWidget, &QDockWidget::setWindowFilePath);

  disconnect(pipelineView, &SVPipelineView::pipelineChanged, 0, 0);

  disconnect(pipelineView->selectionModel(), &QItemSelectionModel::selectionChanged, 0, 0);

  disconnect(pipelineView, &SVPipelineView::filePathOpened, 0, 0);

  disconnect(pipelineView, SIGNAL(filterInputWidgetEdited()), this, SLOT(markActivePipelineAsDirty()));

  disconnect(pipelineView, SIGNAL(filterEnabledStateChanged()), this, SLOT(markActivePipelineAsDirty()));

  disconnect(pipelineView, &SVPipelineView::statusMessage, this, &SIMPLView_UI::setStatusBarMessage);

  /* Pipeline Model Connections */
  disconnect(pipelineModel, &PipelineModel::pipelineDataChanged, 0, 0);

  disconnect(m_ActionCut, &QAction::triggered, pipelineView->getActionCut(), &QAction::trigger);
  disconnect(m_ActionCopy, &QAction::triggered, pipelineView->getActionCopy(), &QAction::trigger);
  disconnect(m_ActionPaste, &QAction::triggered, pipelineView->getActionPaste(), &QAction::trigger);

  disconnect(pipelineView, &SVPipelineView::cutAvailabilityChanged, 0, 0);
  disconnect(pipelineView, &SVPipelineView::copyAvailabilityChanged, 0, 0);
  disconnect(pipelineView, &SVPipelineView::pasteAvailabilityChanged, 0, 0);
  disconnect(pipelineView, &SVPipelineView::clearPipelineAvailabilityChanged, 0, 0);

  disconnect(m_ActionUndo, &QAction::triggered, 0, 0);
  disconnect(m_ActionRedo, &QAction::triggered, 0, 0);

  disconnect(pipelineView->getActionUndo(), &QAction::changed, 0, 0);
  disconnect(pipelineView->getActionRedo(), &QAction::changed, 0, 0);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::connectSignalsSlots()
{
  /* Documentation Requester connections */
  DocRequestManager* docRequester = DocRequestManager::Instance();

  connect(docRequester, SIGNAL(showFilterDocs(const QString&)), this, SLOT(showFilterHelp(const QString&)));

  connect(docRequester, SIGNAL(showFilterDocUrl(const QUrl&)), this, SLOT(showFilterHelpUrl(const QUrl&)));

  /* Bookmarks Widget Connections */
  connect(m_Ui->bookmarksWidget, SIGNAL(updateStatusBar(const QString&)), this, SLOT(setStatusBarMessage(const QString&)));

  connect(m_Ui->bookmarksWidget, &BookmarksToolboxWidget::bookmarkActivated, this, &SIMPLView_UI::openPipeline);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int SIMPLView_UI::openPipeline(const QString& filePath, bool execute)
{
  SIMPLViewPipelineDockWidget* dockWidget = nullptr;
  if (m_ActivePipelineDockWidget && !m_ActivePipelineDockWidget->isWindowModified() && m_ActivePipelineDockWidget->isPipelineEmpty())
  {
    dockWidget = m_ActivePipelineDockWidget;
  }
  else
  {
    dockWidget = addPipeline();
  }

  int err = dockWidget->openPipeline(filePath);

  if (execute)
  {
    dockWidget->executePipeline();
  }

  QFileInfo fi(filePath);
  dockWidget->setWindowTitle(fi.baseName());
  dockWidget->setObjectName(fi.baseName());
  dockWidget->setWindowModified(false);

  QtSRecentFileList* list = QtSRecentFileList::Instance();
  list->addFile(filePath);

  m_OpenedFilePath = filePath;

  return err;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::showFilterParameterTab()
{
  m_Ui->tabWidget->setCurrentIndex(0);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::showVisualizationTab()
{
  m_Ui->tabWidget->setCurrentIndex(1);
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
QMessageBox::StandardButton SIMPLView_UI::checkDirtyPipeline(SIMPLViewPipelineDockWidget* dockWidget)
{
  if(dockWidget->isWindowModified() == true)
  {
    int r = QMessageBox::warning(this, BrandedStrings::ApplicationName, tr("The Pipeline '%1' has been modified.\n\nDo you want to save your changes?").arg(dockWidget->windowTitle()),
                                 QMessageBox::Save | QMessageBox::Cancel | QMessageBox::Discard, QMessageBox::Save);
    if(r == QMessageBox::Save)
    {
      if(savePipeline(dockWidget) == true)
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
void SIMPLView_UI::pipelineDidFinish()
{
  // Re-enable FilterListToolboxWidget signals - resume adding filters
  m_Ui->filterListWidget->blockSignals(false);

  // Re-enable FilterLibraryToolboxWidget signals - resume adding filters
  m_Ui->filterLibraryWidget->blockSignals(false);

  PipelineListWidget* listWidget = m_ActivePipelineDockWidget->getPipelineListWidget();
  SVPipelineView* pipelineView = listWidget->getPipelineView();
  QModelIndexList selectedIndexes = pipelineView->selectionModel()->selectedRows();
  qSort(selectedIndexes);

  if(selectedIndexes.size() == 1)
  {
    QModelIndex selectedIndex = selectedIndexes[0];
    PipelineModel* model = pipelineView->getPipelineModel();

    AbstractFilter::Pointer filter = model->filter(selectedIndex);
    m_Ui->dataBrowserWidget->filterActivated(filter);
  }
  else
  {
    m_Ui->dataBrowserWidget->filterActivated(AbstractFilter::NullPointer());
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
void SIMPLView_UI::setFilterInputWidget(FilterInputWidget* widget)
{
  if(widget == nullptr)
  {
    return;
  }

  if(m_FilterInputWidget)
  {
    emit m_FilterInputWidget->endPathFiltering();
    emit m_FilterInputWidget->endViewPaths();
  }

  // Clear the filter input widget
  clearFilterInputWidget();

  // Alert to DataArrayPath requirements
  connect(widget, SIGNAL(viewPathsMatchingReqs(DataContainerSelectionFilterParameter::RequirementType)), getDataStructureWidget(),
          SLOT(setViewReqs(DataContainerSelectionFilterParameter::RequirementType)), Qt::ConnectionType::UniqueConnection);
  connect(widget, SIGNAL(viewPathsMatchingReqs(AttributeMatrixSelectionFilterParameter::RequirementType)), getDataStructureWidget(),
          SLOT(setViewReqs(AttributeMatrixSelectionFilterParameter::RequirementType)), Qt::ConnectionType::UniqueConnection);
  connect(widget, SIGNAL(viewPathsMatchingReqs(DataArraySelectionFilterParameter::RequirementType)), getDataStructureWidget(), SLOT(setViewReqs(DataArraySelectionFilterParameter::RequirementType)),
          Qt::ConnectionType::UniqueConnection);
  connect(widget, SIGNAL(endViewPaths()), getDataStructureWidget(), SLOT(clearViewRequirements()), Qt::ConnectionType::UniqueConnection);
  connect(getDataStructureWidget(), SIGNAL(filterPath(DataArrayPath)), widget, SIGNAL(filterPath(DataArrayPath)), Qt::ConnectionType::UniqueConnection);
  connect(getDataStructureWidget(), SIGNAL(endPathFiltering()), widget, SIGNAL(endPathFiltering()), Qt::ConnectionType::UniqueConnection);
  connect(getDataStructureWidget(), SIGNAL(applyPathToFilteringParameter(DataArrayPath)), widget, SIGNAL(applyPathToFilteringParameter(DataArrayPath)));

  emit widget->endPathFiltering();

  // Set the widget into the frame
  m_Ui->fiwFrameVLayout->addWidget(widget);
  m_FilterInputWidget = widget;
  widget->show();

  // Force the FilterParameterTab front and center
  showFilterParameterTab();
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
void SIMPLView_UI::markActivePipelineAsDirty()
{
  m_ActivePipelineDockWidget->setWindowModified(true);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::issuesTableHasErrors(bool hasErrors, int errCount, int warnCount)
{
  Q_UNUSED(errCount)
  Q_UNUSED(warnCount)
  if(HideDockSetting::OnError == m_HideErrorTable || HideDockSetting::OnStatusAndError == m_HideErrorTable)
  {
    m_Ui->issuesDockWidget->setVisible(hasErrors);
  }

  if(HideDockSetting::OnError == m_HideStdOutput || HideDockSetting::OnStatusAndError == m_HideStdOutput)
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
void SIMPLView_UI::changeEvent(QEvent* event)
{
  if(event->type() == QEvent::ActivationChange)
  {
    emit dream3dWindowChangedState(this);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLView_UI::setPipelineDockWidgetAsActive(SIMPLViewPipelineDockWidget* dockWidget)
{
  // Deactivate the currently activated dock widget
  if (m_ActivePipelineDockWidget)
  {
    m_ActivePipelineDockWidget->setActive(false);

    m_ActivePipelineDockWidget->getPipelineListWidget()->getPipelineView()->clearSelection();

    disconnectActivePipelineSignalsSlots(m_ActivePipelineDockWidget);

    m_ActionCut->setEnabled(false);
    m_ActionCopy->setEnabled(false);
    m_ActionPaste->setEnabled(false);
    m_ActionClearPipeline->setEnabled(false);

    m_ActionUndo->setText("Undo");
    m_ActionUndo->setEnabled(false);
    m_ActionRedo->setText("Redo");
    m_ActionRedo->setEnabled(false);

    m_Ui->issuesWidget->clearIssues();
    m_Ui->statusbar->clearMessage();
  }

  if (dockWidget)
  {
    // Activate the new dock widget
    dockWidget->setActive(true);

    QModelIndexList selectedIndexes = dockWidget->getPipelineListWidget()->getPipelineView()->selectionModel()->selectedRows();
    if(selectedIndexes.size() == 1)
    {
      PipelineModel* model = dockWidget->getPipelineListWidget()->getPipelineView()->getPipelineModel();
      AbstractFilter::Pointer filter = model->filter(selectedIndexes[0]);
      if (filter.get() != nullptr)
      {
        m_Ui->dataBrowserWidget->filterActivated(filter);
      }
    }

    connectActivePipelineSignalsSlots(dockWidget);

    SVPipelineView* viewWidget = dockWidget->getPipelineListWidget()->getPipelineView();
    m_ActionCut->setEnabled(viewWidget->getActionCut()->isEnabled());
    m_ActionCopy->setEnabled(viewWidget->getActionCopy()->isEnabled());
    m_ActionPaste->setEnabled(viewWidget->getActionPaste()->isEnabled());
    m_ActionClearPipeline->setEnabled(viewWidget->getActionClearPipeline()->isEnabled());

    m_ActionUndo->setText(viewWidget->getActionUndo()->text());
    m_ActionUndo->setEnabled(viewWidget->getActionUndo()->isEnabled());
    m_ActionRedo->setText(viewWidget->getActionRedo()->text());
    m_ActionRedo->setEnabled(viewWidget->getActionRedo()->isEnabled());

    QVector<PipelineMessage> issues = viewWidget->getCurrentIssues();
    m_Ui->issuesWidget->displayCachedMessages(issues);

    QTextEdit* stdOutTextEdit = viewWidget->getStdOutputTextEdit();
    m_Ui->stdOutWidget->setStdOutputTextEdit(stdOutTextEdit);
  }
  else
  {
    m_Ui->stdOutWidget->setStdOutputTextEdit(nullptr);
  }

  m_ActivePipelineDockWidget = dockWidget;
}
