/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

#include <QmitkSingleNodeSelectionWidget.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateOr.h>
#include <mitkNodePredicateProperty.h>

#include <usModuleRegistry.h>

#include <QMessageBox>

#include "QmitkTotalSegmentatorView.h"
#include <mitkDockerHelper.h>
#include <mitkLabel.h>
#include <mitkLabelSetImage.h>

// Don't forget to initialize the VIEW_ID.
const std::string QmitkTotalSegmentatorView::VIEW_ID =
    "org.mitk.views.docker.gpu.totalsegmentator";

void QmitkTotalSegmentatorView::CreateQtPartControl(QWidget *parent) {
  // Setting up the UI is a true pleasure when using .ui files, isn't it?
  m_Controls.setupUi(parent);

  m_Controls.selectionWidget->SetDataStorage(this->GetDataStorage());
  m_Controls.selectionWidget->SetSelectionIsOptional(true);
  m_Controls.selectionWidget->SetEmptyInfo(QStringLiteral("Select an image"));
  m_Controls.selectionWidget->SetAutoSelectNewNodes(true);
  m_Controls.selectionWidget->SetNodePredicate(mitk::NodePredicateAnd::New(
      mitk::TNodePredicateDataType<mitk::Image>::New(),
      mitk::NodePredicateNot::New(mitk::NodePredicateOr::New(
          mitk::NodePredicateProperty::New("helper object"),
          mitk::NodePredicateProperty::New("hidden object")))));

  // // Wire up the UI widgets with our functionality.
  // connect(m_Controls.selectionWidget,
  //         &QmitkSingleNodeSelectionWidget::CurrentSelectionChanged, this,
  //         &QmitkTotalSegmentatorView::OnImageChanged);
  connect(m_Controls.btnRunTotalSegmentator, SIGNAL(clicked()), this,
          SLOT(OnStartTotalSegmentator()));
}

void QmitkTotalSegmentatorView::SetFocus() {
  m_Controls.btnRunTotalSegmentator->setFocus();
}

void QmitkTotalSegmentatorView::OnStartTotalSegmentator() {

  using namespace itksys;
  auto selectedDataNode = m_Controls.selectionWidget->GetSelectedNode();
  auto data = selectedDataNode->GetData();

  // We don't use the auto keyword here, which would evaluate to a native
  // image pointer. Instead, we want a smart pointer to ensure that
  // the image isn't deleted somewhere else while we're using it.
  mitk::Image::Pointer image = dynamic_cast<mitk::Image *>(data);

  mitk::DockerHelper helper("wasserth/totalsegmentator_container:master");
  helper.AddRunArgument("--gpus", "device=0");
  helper.AddRunArgument("--ipc=host");

  // arguments
  helper.AddApplicationArgument("TotalSegmentator");

  mitk::DockerHelper::LoadDataInfo *mlResult = nullptr;
  if (m_Controls.cbMultiLabel->isChecked())
    mlResult =
        helper.AddLoadLaterOutput("--ml", "results.nii", helper.FLAG_ONLY);

  if (m_Controls.cbFast->isChecked())
    helper.AddApplicationArgument("--fast");

  if (!m_Controls.textEdit->toPlainText().isEmpty())
    helper.AddApplicationArgument(
        "--roi_subset", m_Controls.textEdit->toPlainText().toStdString());

  // inputs
  helper.AddAutoSaveData(image, "-i", "input_image.nii.gz");

  // output
  helper.AddAutoLoadOutputFolder("-o", "results",
                                 m_TotalSegmentatorResultFileNames);

  if (m_Controls.cbStatistics->isChecked())
    helper.AddLoadLaterOutput("--statistics", "statistics.json", helper.FLAG_ONLY);

  if (m_Controls.cbRadiomics->isChecked())
    helper.AddLoadLaterOutput("--radiomics", "statistics_radiomics.json", helper.FLAG_ONLY);

  if (m_Controls.cbPreview->isChecked())
    helper.AddAutoLoadOutput("--preview", "preview.png", helper.FLAG_ONLY);

  helper.EnableAutoRemoveContainer(true);
  auto results = helper.GetResults();
  std::string filePath;
  for (auto image : results) {
    data->GetPropertyList()->GetStringProperty("MITK.IO.reader.inputlocation",

                                               filePath);
    data->GetPropertyList()->RemoveProperty("MITK.IO.reader.inputlocation");

    auto node = mitk::DataNode::New();
    node->SetData(image);
    node->SetName(SystemTools::GetFilenameWithoutExtension(filePath));
    this->GetDataStorage()->Add(node, selectedDataNode);
  }

  if (mlResult) {
    auto mlPath = helper.GetFilePath(mlResult->path);
    auto results = mitk::IOUtil::Load(mlPath);
    auto lsImage = mitk::LabelSetImage::New();
    lsImage->InitializeByLabeledImage(
        dynamic_cast<mitk::Image *>(results[0].GetPointer()));
    auto node = mitk::DataNode::New();
    node->SetData(lsImage);
    node->SetName("TotalSegmentator_multilabel");
    // auto ls = lsImage->GetActiveLabelSet();
    // for (unsigned t = 0; t < ls->GetNumberOfLabels(); ++t) {
    //   lsImage->GetLabel(t)->SetName(SystemTools::GetFilenameWithoutExtension(m_TotalSegmentatorResultFileNames[t]));
    // }

    this->GetDataStorage()->Add(node, selectedDataNode);
  }
}
