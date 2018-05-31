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

#include "SIMPLViewPipelineDockWidget.h"

#include <QtWidgets/QGridLayout>

#include "SVWidgetsLib/Widgets/PipelineListWidget.h"
#include "SVWidgetsLib/Widgets/PipelineItemDelegate.h"
#include "SVWidgetsLib/Widgets/PipelineModel.h"

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
SIMPLViewPipelineDockWidget::SIMPLViewPipelineDockWidget(QWidget* parent)
  : QDockWidget(parent)
{
  setupGui();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
SIMPLViewPipelineDockWidget::~SIMPLViewPipelineDockWidget() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLViewPipelineDockWidget::setupGui()
{
  QWidget* internalWidget = new QWidget();
  QGridLayout* gridLayout = new QGridLayout(internalWidget);
  gridLayout->setContentsMargins(0, 0, 0, 0);
  m_PipelineListWidget = new PipelineListWidget(internalWidget);

  gridLayout->addWidget(m_PipelineListWidget, 0, 0, 1, 1);

  setWidget(internalWidget);

  setMinimumSize(QSize(62, 38));

  QString name = "Untitled Pipeline";
  setWindowTitle(name);
  setObjectName(name);

  setupPipelineView();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLViewPipelineDockWidget::setupPipelineView()
{
  SVPipelineView* pipelineView = m_PipelineListWidget->getPipelineView();

  PipelineItemDelegate* delegate = new PipelineItemDelegate(pipelineView);
  pipelineView->setItemDelegate(delegate);

  // Create the model
  PipelineModel* model = new PipelineModel(this);
  model->setMaxNumberOfPipelines(1);

  pipelineView->setModel(model);

  /* Pipeline List Widget Connections */
  connect(m_PipelineListWidget, &PipelineListWidget::pipelineCanceled, pipelineView, &SVPipelineView::cancelPipeline);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int SIMPLViewPipelineDockWidget::openPipeline(const QString& filePath)
{
  int err = m_PipelineListWidget->getPipelineView()->openPipeline(filePath);
  return err;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLViewPipelineDockWidget::executePipeline()
{
  m_PipelineListWidget->getPipelineView()->executePipeline();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SIMPLViewPipelineDockWidget::setActive(bool active)
{
  m_Active = active;
  m_PipelineListWidget->setActive(active);

  style()->unpolish(this);
  style()->polish(this);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool SIMPLViewPipelineDockWidget::getActive()
{
  return m_Active;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
PipelineListWidget* SIMPLViewPipelineDockWidget::getPipelineListWidget()
{
  return m_PipelineListWidget;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
PipelineModel* SIMPLViewPipelineDockWidget::getPipelineModel()
{
  SVPipelineView* pipelineView = m_PipelineListWidget->getPipelineView();
  return pipelineView->getPipelineModel();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool SIMPLViewPipelineDockWidget::isPipelineEmpty()
{
  PipelineModel* model = getPipelineModel();
  return (model->rowCount() <= 0);
}
