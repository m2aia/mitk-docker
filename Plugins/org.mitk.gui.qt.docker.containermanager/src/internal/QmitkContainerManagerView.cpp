/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "QmitkContainerManagerView.h"

#include <QMessageBox>
#include <QHeaderView>
#include <QMenu>
#include <QClipboard>
#include <QApplication>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDesktopServices>
#include <QUrl>
#include <mitkDockerHelper.h>
#include <mitkDockerImageManager.h>
#include <mitkProgressBar.h>
#include <mitkLogMacros.h>

const std::string QmitkContainerManagerView::VIEW_ID = "org.mitk.views.docker.containermanager";

void QmitkContainerManagerView::CreateQtPartControl(QWidget *parent)
{
  // Setting up the UI
  m_Controls.setupUi(parent);
  
  // Initialize Docker processes
  m_DockerProcess = new QProcess(this);
  m_QueryProcess = new QProcess(this);
  m_CheckProcess = new QProcess(this);
  m_HelpProcess = new QProcess(this);
  
  // Setup image table
  m_Controls.imageTable->setColumnCount(3);
  m_Controls.imageTable->setHorizontalHeaderLabels(QStringList() << "Image" << "Version" << "Actions");
  m_Controls.imageTable->horizontalHeader()->setStretchLastSection(false);
  m_Controls.imageTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  m_Controls.imageTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  m_Controls.imageTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  m_Controls.imageTable->verticalHeader()->setVisible(false);
  m_Controls.imageTable->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_Controls.imageTable, &QTableWidget::customContextMenuRequested, 
          this, &QmitkContainerManagerView::OnImageTableContextMenu);
  
  // Wire up the UI widgets with functionality
  connect(m_Controls.btnAddImage, &QPushButton::clicked, this, &QmitkContainerManagerView::OnAddImage);
  connect(m_Controls.btnReloadImages, &QPushButton::clicked, this, &QmitkContainerManagerView::OnReloadImages);
  connect(m_Controls.btnLoadJson, &QPushButton::clicked, this, &QmitkContainerManagerView::OnLoadJson);
  connect(m_Controls.btnSaveJson, &QPushButton::clicked, this, &QmitkContainerManagerView::OnSaveJson);
  
  // Connect image table selection changed
  connect(m_Controls.imageTable, &QTableWidget::itemSelectionChanged, 
          this, &QmitkContainerManagerView::OnImageTableSelectionChanged);
  connect(m_Controls.imageTable, &QTableWidget::itemChanged,
          this, &QmitkContainerManagerView::OnImageTableItemChanged);
  
  // Connect details view editing
  connect(m_Controls.repositoryEdit, &QLineEdit::editingFinished,
          this, &QmitkContainerManagerView::OnRepositoryEditingFinished);
  connect(m_Controls.repositoryEdit, &QLineEdit::customContextMenuRequested,
          this, &QmitkContainerManagerView::OnRepositoryContextMenu);
  connect(m_Controls.notesEdit, &QLineEdit::editingFinished,
          this, &QmitkContainerManagerView::OnNotesEditingFinished);
  connect(m_Controls.fetchHelpButton, &QPushButton::clicked,
          this, &QmitkContainerManagerView::OnFetchHelp);
  
  // Set monospace font for help text
  QFont monoFont("Courier");
  monoFont.setStyleHint(QFont::Monospace);
  m_Controls.helpTextEdit->setFont(monoFont);
  
  // Initialize details view as disabled
  ClearDetailsView();
  
  // Connect Docker process signals
  connect(m_DockerProcess, &QProcess::finished, this, &QmitkContainerManagerView::OnDockerProcessFinished);
  connect(m_DockerProcess, &QProcess::errorOccurred, this, &QmitkContainerManagerView::OnDockerProcessError);
  connect(m_DockerProcess, &QProcess::readyReadStandardOutput, this, &QmitkContainerManagerView::OnDockerProcessOutput);
  connect(m_DockerProcess, &QProcess::readyReadStandardError, this, &QmitkContainerManagerView::OnDockerProcessOutput);
  
  // Connect help process signals
  connect(m_HelpProcess, &QProcess::finished, this, &QmitkContainerManagerView::OnHelpProcessFinished);
  
  // Connect query process signals
  connect(m_QueryProcess, &QProcess::finished, this, &QmitkContainerManagerView::OnQueryTagsFinished);
  
  // Connect check process signals
  connect(m_CheckProcess, &QProcess::finished, this, &QmitkContainerManagerView::OnCheckLocalImagesFinished);
  
  // Initialize image manager
  m_ImageManager = new mitk::DockerImageManager();
  
  // Load persisted images from preferences
  LoadPersistedImages();
  
  // Check Docker status
  UpdateDockerStatus();
  CheckLocalImages();
}

void QmitkContainerManagerView::SetFocus()
{
  m_Controls.imageUrlInput->setFocus();
}

void QmitkContainerManagerView::EnableWidgets(bool enable)
{
  m_Controls.btnAddImage->setEnabled(enable);
  m_Controls.imageUrlInput->setEnabled(enable);
  m_Controls.imageTable->setEnabled(enable);
}

void QmitkContainerManagerView::UpdateDockerStatus()
{
  if (mitk::DockerHelper::CanRunDocker())
  {
    m_Controls.statusLabel->setText("Docker Status: <span style='color: green;'>Available</span>");
    EnableWidgets(true);
  }
  else
  {
    m_Controls.statusLabel->setText("Docker Status: <span style='color: red;'>Not Available</span>");
    EnableWidgets(false);
    QMessageBox::warning(nullptr, "Docker Error", 
      "Docker is not available. Please ensure Docker is installed and running.");
  }
}

void QmitkContainerManagerView::OnAddImage()
{
  QString imageUrl = m_Controls.imageUrlInput->text().trimmed();
  
  if (imageUrl.isEmpty())
  {
    QMessageBox::warning(nullptr, "Input Error", "Please enter a Docker image URL.");
    return;
  }
  
  // Check if image already exists in the list
  if (m_Images.contains(imageUrl))
  {
    QMessageBox::information(nullptr, "Already Added", "This image is already in the list.");
    return;
  }
  
  // Add to persistent storage
  mitk::DockerImageManager::DockerImage dockerImage(imageUrl.toStdString(), "latest");
  if (m_ImageManager->AddImage(dockerImage))
  {
    m_Controls.outputTextEdit->append(QString("Querying tags for: %1").arg(imageUrl));
    m_CurrentQueryImage = imageUrl;
    
    // Query available tags from registry
    QueryImageTags(imageUrl);
  }
  else
  {
    QMessageBox::warning(nullptr, "Error", "Failed to add image to persistent storage.");
  }
}

void QmitkContainerManagerView::QueryImageTags(const QString& imageName)
{
  m_Controls.outputTextEdit->append(QString("Adding image: %1").arg(imageName));
  
  // Add the image with default tag "latest"
  // Users can manually edit the tag to specify the version they need
  AddImageRow(imageName);
  m_Controls.imageUrlInput->clear();
}

void QmitkContainerManagerView::OnQueryTagsFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  (void)exitCode;
  (void)exitStatus;
  // This slot is no longer used since we don't query the API
  // Kept for compatibility
}

void QmitkContainerManagerView::AddImageRow(const QString& imageName)
{
  int row = m_Controls.imageTable->rowCount();
  m_Controls.imageTable->insertRow(row);
  
  // Block signals to avoid triggering itemChanged during setup
  m_Controls.imageTable->blockSignals(true);
  
  // Image name item
  QTableWidgetItem* nameItem = new QTableWidgetItem(imageName);
  nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
  nameItem->setData(Qt::UserRole, imageName); // Store original name
  m_Controls.imageTable->setItem(row, 0, nameItem);
  
  // Version item (default to "latest")
  QTableWidgetItem* versionItem = new QTableWidgetItem("latest");
  versionItem->setFlags(versionItem->flags() | Qt::ItemIsEditable);
  versionItem->setData(Qt::UserRole, imageName); // Store associated image name
  m_Controls.imageTable->setItem(row, 1, versionItem);
  
  m_Controls.imageTable->blockSignals(false);
  
  // Actions widget (pull and remove buttons)
  QWidget* actionsWidget = new QWidget();
  QHBoxLayout* actionsLayout = new QHBoxLayout(actionsWidget);
  actionsLayout->setContentsMargins(4, 4, 4, 4);
  actionsLayout->setSpacing(4);
  
  QPushButton* pullButton = new QPushButton("Pull");
  pullButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  pullButton->setMinimumWidth(60);
  pullButton->setProperty("imageName", imageName);
  pullButton->setProperty("row", row);
  connect(pullButton, &QPushButton::clicked, this, &QmitkContainerManagerView::OnPullImage);
  
  QPushButton* removeButton = new QPushButton("Remove");
  removeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  removeButton->setMinimumWidth(70);
  removeButton->setProperty("imageName", imageName);
  removeButton->setProperty("row", row);
  connect(removeButton, &QPushButton::clicked, this, &QmitkContainerManagerView::OnRemoveImage);
  
  actionsLayout->addWidget(pullButton);
  actionsLayout->addWidget(removeButton);
  actionsLayout->addStretch();
  actionsWidget->setLayout(actionsLayout);
  
  m_Controls.imageTable->setCellWidget(row, 2, actionsWidget);
  
  // Store image info
  ImageInfo info;
  info.imageName = imageName;
  info.pullButton = pullButton;
  info.removeButton = removeButton;
  info.tableRow = row;
  m_Images[imageName] = info;
  
  // Update pull button state
  UpdatePullButtons();
  
  m_Controls.outputTextEdit->append(QString("<span style='color: green;'>Added image: %1</span>").arg(imageName));
}

void QmitkContainerManagerView::OnRemoveImage()
{
  QPushButton* button = qobject_cast<QPushButton*>(sender());
  if (!button) return;
  
  QString imageName = button->property("imageName").toString();
  int row = button->property("row").toInt();
  
  // Clear details view if this image is currently selected
  if (m_CurrentSelectedImage == imageName)
  {
    ClearDetailsView();
  }
  
  m_Controls.imageTable->removeRow(row);
  m_Images.remove(imageName);
  
  // Remove from persistent storage
  m_ImageManager->RemoveImage(imageName.toStdString());
  
  // Update row properties for remaining images
  for (auto it = m_Images.begin(); it != m_Images.end(); ++it)
  {
    if (it->tableRow > row)
    {
      it->tableRow--;
      it->pullButton->setProperty("row", it->tableRow);
      it->removeButton->setProperty("row", it->tableRow);
    }
  }
  
  m_Controls.outputTextEdit->append(QString("Removed image: %1").arg(imageName));
}

void QmitkContainerManagerView::OnPullImage()
{
  if (!mitk::DockerHelper::CanRunDocker())
  {
    QMessageBox::warning(nullptr, "Docker Error", "Docker is not available.");
    return;
  }
  
  QPushButton* button = qobject_cast<QPushButton*>(sender());
  if (!button) return;
  
  QString imageName = button->property("imageName").toString();
  
  if (!m_Images.contains(imageName)) return;
  
  ImageInfo& info = m_Images[imageName];
  QTableWidgetItem* versionItem = m_Controls.imageTable->item(info.tableRow, 1);
  QString version = versionItem ? versionItem->text().trimmed() : "latest";
  if (version.isEmpty())
  {
    version = "latest";
  }
  QString fullImageName = QString("%1:%2").arg(imageName).arg(version);
  
  // Disable widgets while pulling
  EnableWidgets(false);
  m_Controls.progressBar->setValue(0);
  m_Controls.progressBar->setVisible(true);
  m_Controls.outputTextEdit->append(QString("Pulling image: %1\n").arg(fullImageName));
  
  m_CurrentPullImage = fullImageName;
  
  // Start Docker pull process
  QStringList arguments;
  arguments << "pull" << fullImageName;
  
  mitk::ProgressBar::GetInstance()->AddStepsToDo(1);
  
  m_DockerProcess->start("docker", arguments);
}

void QmitkContainerManagerView::CheckLocalImages()
{
  QStringList arguments;
  arguments << "images" << "--format" << "{{.Repository}}:{{.Tag}}";
  
  m_CheckProcess->start("docker", arguments);
}

void QmitkContainerManagerView::OnCheckLocalImagesFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  if (exitStatus == QProcess::NormalExit && exitCode == 0)
  {
    QString output = m_CheckProcess->readAllStandardOutput();
    m_LocalImages = output.split('\n', Qt::SkipEmptyParts);
    
    m_Controls.outputTextEdit->append(QString("Found %1 local image(s)").arg(m_LocalImages.size()));
    UpdatePullButtons();
  }
}

void QmitkContainerManagerView::UpdatePullButtons()
{
  for (auto it = m_Images.begin(); it != m_Images.end(); ++it)
  {
    QString imageName = it->imageName;
    QTableWidgetItem* versionItem = m_Controls.imageTable->item(it->tableRow, 1);
    QString version = versionItem ? versionItem->text().trimmed() : "latest";
    if (version.isEmpty())
    {
      version = "latest";
    }
    QString fullImage = QString("%1:%2").arg(imageName).arg(version);
    
    bool isLocal = IsImageLocallyAvailable(imageName, version);
    
    if (isLocal)
    {
      it->pullButton->setEnabled(false);
    }
    else
    {
      it->pullButton->setEnabled(true);
    }
  }
}

bool QmitkContainerManagerView::IsImageLocallyAvailable(const QString& imageName, const QString& tag)
{
  QString fullImage = QString("%1:%2").arg(imageName).arg(tag);
  return m_LocalImages.contains(fullImage);
}

void QmitkContainerManagerView::OnDockerProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  mitk::ProgressBar::GetInstance()->Progress();
  
  m_Controls.progressBar->setVisible(false);
  EnableWidgets(true);
  
  if (exitStatus == QProcess::NormalExit && exitCode == 0)
  {
    m_Controls.outputTextEdit->append(QString("\n<span style='color: green;'>Image pulled successfully: %1</span>").arg(m_CurrentPullImage));
    QMessageBox::information(nullptr, "Success", "Docker image pulled successfully!");
    
    // Refresh local images list
    CheckLocalImages();
  }
  else
  {
    m_Controls.outputTextEdit->append(QString("\n<span style='color: red;'>Process failed with exit code: %1</span>").arg(exitCode));
    QMessageBox::warning(nullptr, "Error", "Failed to pull Docker image. Check the output for details.");
  }
}

void QmitkContainerManagerView::OnDockerProcessError(QProcess::ProcessError error)
{
  mitk::ProgressBar::GetInstance()->Progress();
  
  m_Controls.progressBar->setVisible(false);
  EnableWidgets(true);
  
  QString errorMessage;
  switch (error)
  {
    case QProcess::FailedToStart:
      errorMessage = "Failed to start Docker process. Is Docker installed?";
      break;
    case QProcess::Crashed:
      errorMessage = "Docker process crashed.";
      break;
    case QProcess::Timedout:
      errorMessage = "Docker process timed out.";
      break;
    case QProcess::WriteError:
      errorMessage = "Write error occurred.";
      break;
    case QProcess::ReadError:
      errorMessage = "Read error occurred.";
      break;
    default:
      errorMessage = "Unknown error occurred.";
  }
  
  m_Controls.outputTextEdit->append(QString("\n<span style='color: red;'>Error: %1</span>").arg(errorMessage));
  QMessageBox::critical(nullptr, "Docker Error", errorMessage);
}

void QmitkContainerManagerView::OnDockerProcessOutput()
{
  // Read and display standard output
  QString output = m_DockerProcess->readAllStandardOutput();
  if (!output.isEmpty())
  {
    m_Controls.outputTextEdit->append(output);
  }
  
  // Read and display standard error
  QString errorOutput = m_DockerProcess->readAllStandardError();
  if (!errorOutput.isEmpty())
  {
    m_Controls.outputTextEdit->append(QString("<span style='color: orange;'>%1</span>").arg(errorOutput));
  }
  
  // Update progress bar (simplified - in reality, you'd parse the output for progress)
  static int progress = 0;
  progress = (progress + 10) % 100;
  m_Controls.progressBar->setValue(progress);
}

void QmitkContainerManagerView::LoadPersistedImages()
{
  if (!m_ImageManager)
  {
    MITK_ERROR << "Image manager not initialized";
    return;
  }
  
  std::vector<mitk::DockerImageManager::DockerImage> persistedImages = m_ImageManager->GetImages();
  
  MITK_INFO << "Loading " << persistedImages.size() << " persisted Docker image(s)";
  
  for (const auto& image : persistedImages)
  {
    if (image.IsValid())
    {
      QString imageName = QString::fromStdString(image.imageName);
      AddImageRow(imageName);
      
      // Set the saved tag
      if (m_Images.contains(imageName))
      {
        ImageInfo& info = m_Images[imageName];
        QTableWidgetItem* versionItem = m_Controls.imageTable->item(info.tableRow, 1);
        if (versionItem)
        {
          m_Controls.imageTable->blockSignals(true);
          versionItem->setText(QString::fromStdString(image.tag));
          m_Controls.imageTable->blockSignals(false);
        }
      }
      
      MITK_INFO << "Loaded persisted image: " << image.FullImageName();
    }
  }
}

void QmitkContainerManagerView::OnImageTableContextMenu(const QPoint& pos)
{
  // Get the item at the position
  QModelIndex index = m_Controls.imageTable->indexAt(pos);
  if (!index.isValid())
    return;
  
  int row = index.row();
  if (row < 0 || row >= m_Controls.imageTable->rowCount())
    return;
  
  // Get image name and tag from table items
  QTableWidgetItem* nameItem = m_Controls.imageTable->item(row, 0);
  QTableWidgetItem* versionItem = m_Controls.imageTable->item(row, 1);
  
  if (!nameItem)
    return;
  
  QString imageName = nameItem->text().trimmed();
  QString tag = versionItem ? versionItem->text().trimmed() : "latest";
  if (tag.isEmpty())
    tag = "latest";
  
  QString fullImageTag = QString("%1:%2").arg(imageName).arg(tag);
  
  // Create context menu
  QMenu contextMenu(m_Controls.imageTable);
  
  QAction* copyImageTagAction = contextMenu.addAction("Copy Image Tag");
  copyImageTagAction->setIcon(QIcon::fromTheme("edit-copy"));
  
  QAction* copyImageNameAction = contextMenu.addAction("Copy Image Name");
  copyImageNameAction->setIcon(QIcon::fromTheme("edit-copy"));
  
  contextMenu.addSeparator();
  
  QAction* copyTagOnlyAction = contextMenu.addAction("Copy Tag Only");
  copyTagOnlyAction->setIcon(QIcon::fromTheme("edit-copy"));
  
  // Show menu and handle selection
  QAction* selectedAction = contextMenu.exec(m_Controls.imageTable->viewport()->mapToGlobal(pos));
  
  if (selectedAction == copyImageTagAction)
  {
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(fullImageTag);
    m_Controls.outputTextEdit->append(QString("<span style='color: green;'>Copied to clipboard: %1</span>").arg(fullImageTag));
  }
  else if (selectedAction == copyImageNameAction)
  {
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(imageName);
    m_Controls.outputTextEdit->append(QString("<span style='color: green;'>Copied to clipboard: %1</span>").arg(imageName));
  }
  else if (selectedAction == copyTagOnlyAction)
  {
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(tag);
    m_Controls.outputTextEdit->append(QString("<span style='color: green;'>Copied to clipboard: %1</span>").arg(tag));
  }
}

void QmitkContainerManagerView::OnImageTableItemChanged(QTableWidgetItem* item)
{
  if (!item || !m_ImageManager)
    return;
  
  int row = item->row();
  int column = item->column();
  
  // Column 0: Image name
  // Column 1: Version/Tag
  
  if (column == 0) // Image name changed
  {
    QString newImageName = item->text().trimmed();
    QString oldImageName = item->data(Qt::UserRole).toString();
    
    if (newImageName.isEmpty())
    {
      QMessageBox::warning(nullptr, "Invalid Name", "Image name cannot be empty.");
      m_Controls.imageTable->blockSignals(true);
      item->setText(oldImageName);
      m_Controls.imageTable->blockSignals(false);
      return;
    }
    
    if (oldImageName == newImageName)
      return;
    
    // Check if new name already exists
    if (m_Images.contains(newImageName))
    {
      QMessageBox::warning(nullptr, "Duplicate Name", "An image with this name already exists.");
      m_Controls.imageTable->blockSignals(true);
      item->setText(oldImageName);
      m_Controls.imageTable->blockSignals(false);
      return;
    }
    
    if (!m_Images.contains(oldImageName))
    {
      item->setData(Qt::UserRole, newImageName);
      return;
    }
    
    // Get current tag
    QTableWidgetItem* versionItem = m_Controls.imageTable->item(row, 1);
    QString currentTag = versionItem ? versionItem->text().trimmed() : "latest";
    if (currentTag.isEmpty())
      currentTag = "latest";
    
    // Remove old image from persistent storage
    m_ImageManager->RemoveImage(oldImageName.toStdString());
    
    // Add new image to persistent storage
    mitk::DockerImageManager::DockerImage dockerImage(newImageName.toStdString(), currentTag.toStdString());
    m_ImageManager->AddImage(dockerImage);
    m_ImageManager->SaveToPreferences();
    
    // Update the info in the map
    ImageInfo info = m_Images[oldImageName];
    info.imageName = newImageName;
    info.pullButton->setProperty("imageName", newImageName);
    info.removeButton->setProperty("imageName", newImageName);
    
    // Update version item's associated image name
    if (versionItem)
      versionItem->setData(Qt::UserRole, newImageName);
    
    // Update stored name
    item->setData(Qt::UserRole, newImageName);
    
    // Remove old entry and add new one
    m_Images.remove(oldImageName);
    m_Images[newImageName] = info;
    
    // Update current selection if needed
    if (m_CurrentSelectedImage == oldImageName)
      m_CurrentSelectedImage = newImageName;
    
    m_Controls.outputTextEdit->append(QString("<span style='color: blue;'>Renamed image: %1 -> %2</span>")
                                       .arg(oldImageName).arg(newImageName));
    
    UpdatePullButtons();
  }
  else if (column == 1) // Version/Tag changed
  {
    QString imageName = item->data(Qt::UserRole).toString();
    if (imageName.isEmpty() || !m_Images.contains(imageName))
      return;
    
    QString newTag = item->text().trimmed();
    if (newTag.isEmpty())
    {
      newTag = "latest";
      m_Controls.imageTable->blockSignals(true);
      item->setText(newTag);
      m_Controls.imageTable->blockSignals(false);
    }
    
    // Update tag in persistent storage
    m_ImageManager->UpdateImageTag(imageName.toStdString(), newTag.toStdString());
    m_ImageManager->SaveToPreferences();
    
    m_Controls.outputTextEdit->append(QString("<span style='color: blue;'>Updated tag for %1 to %2</span>")
                                       .arg(imageName).arg(newTag));
    
    UpdatePullButtons();
  }
}

void QmitkContainerManagerView::OnLoadJson()
{
  QString fileName = QFileDialog::getOpenFileName(nullptr, 
                                                  tr("Load Docker Images from JSON"),
                                                  QString(),
                                                  tr("JSON Files (*.json);;All Files (*)"));
  
  if (fileName.isEmpty())
    return;
  
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QMessageBox::critical(nullptr, "Error", 
                         QString("Failed to open file: %1").arg(fileName));
    m_Controls.outputTextEdit->append(QString("<span style='color: red;'>Failed to open file: %1</span>")
                                     .arg(fileName));
    return;
  }
  
  QByteArray jsonData = file.readAll();
  file.close();
  
  std::string jsonStr = jsonData.toStdString();
  
  if (!m_ImageManager)
  {
    QMessageBox::critical(nullptr, "Error", "Image manager not initialized");
    return;
  }
  
  // Load JSON into image manager
  if (m_ImageManager->FromJson(jsonStr))
  {
    m_ImageManager->SaveToPreferences();
    
    // Clear current table and selection
    m_Controls.imageTable->setRowCount(0);
    m_Images.clear();
    ClearDetailsView();
    
    // Reload images from the updated manager
    LoadPersistedImages();
    
    m_Controls.outputTextEdit->append(QString("<span style='color: green;'>Successfully loaded %1 image(s) from %2</span>")
                                     .arg(m_ImageManager->Count())
                                     .arg(fileName));
    
    // Update pull buttons and check local images
    UpdatePullButtons();
    CheckLocalImages();
  }
  else
  {
    QMessageBox::critical(nullptr, "Error", 
                         "Failed to parse JSON file. Please check the file format.");
    m_Controls.outputTextEdit->append(QString("<span style='color: red;'>Failed to parse JSON from %1</span>")
                                     .arg(fileName));
  }
}

void QmitkContainerManagerView::OnSaveJson()
{
  QString fileName = QFileDialog::getSaveFileName(nullptr,
                                                  tr("Save Docker Images to JSON"),
                                                  "docker_images.json",
                                                  tr("JSON Files (*.json);;All Files (*)"));
  
  if (fileName.isEmpty())
    return;
  
  // Ensure .json extension
  if (!fileName.endsWith(".json", Qt::CaseInsensitive))
  {
    fileName += ".json";
  }
  
  if (!m_ImageManager)
  {
    QMessageBox::critical(nullptr, "Error", "Image manager not initialized");
    return;
  }
  
  std::string jsonStr = m_ImageManager->ToJson();
  
  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QMessageBox::critical(nullptr, "Error", 
                         QString("Failed to create file: %1").arg(fileName));
    m_Controls.outputTextEdit->append(QString("<span style='color: red;'>Failed to create file: %1</span>")
                                     .arg(fileName));
    return;
  }
  
  QTextStream out(&file);
  out << QString::fromStdString(jsonStr);
  file.close();
  
  m_Controls.outputTextEdit->append(QString("<span style='color: green;'>Successfully saved %1 image(s) to %2</span>")
                                   .arg(m_ImageManager->Count())
                                   .arg(fileName));
  
  QMessageBox::information(nullptr, "Success", 
                          QString("Docker images saved to:\n%1").arg(fileName));
}

void QmitkContainerManagerView::OnReloadImages()
{
  // Clear current images from table
  m_Controls.imageTable->setRowCount(0);
  m_Images.clear();
  ClearDetailsView();
  
  // Reload from preferences
  if (m_ImageManager)
  {
    m_ImageManager->LoadFromPreferences();
    LoadPersistedImages();
    UpdatePullButtons();
    
    m_Controls.outputTextEdit->append("<span style='color: green;'>Images reloaded from preferences</span>");
  }
}

void QmitkContainerManagerView::OnImageTableSelectionChanged()
{
  QList<QTableWidgetItem*> selectedItems = m_Controls.imageTable->selectedItems();
  
  if (selectedItems.isEmpty())
  {
    ClearDetailsView();
    return;
  }
  
  int row = selectedItems.first()->row();
  
  // Get image name from table item
  QTableWidgetItem* nameItem = m_Controls.imageTable->item(row, 0);
  
  if (!nameItem)
  {
    ClearDetailsView();
    return;
  }
  
  QString imageName = nameItem->text().trimmed();
  UpdateDetailsView(imageName);
}

void QmitkContainerManagerView::UpdateDetailsView(const QString& imageName)
{
  m_CurrentSelectedImage = imageName;
  
  if (!m_ImageManager || !m_ImageManager->HasImage(imageName.toStdString()))
  {
    ClearDetailsView();
    return;
  }
  
  mitk::DockerImageManager::DockerImage image = m_ImageManager->GetImage(imageName.toStdString());
  
  // Block signals to avoid triggering save while loading
  m_Controls.repositoryEdit->blockSignals(true);
  m_Controls.notesEdit->blockSignals(true);
  
  // Enable the fields for editing
  m_Controls.repositoryEdit->setEnabled(true);
  m_Controls.notesEdit->setEnabled(true);
  m_Controls.repositoryEdit->setReadOnly(false);
  m_Controls.notesEdit->setReadOnly(false);
  
  // If repository is empty, suggest one based on image name
  QString repository = QString::fromStdString(image.repository);
  if (repository.isEmpty())
  {
    repository = SuggestRepositoryUrl(imageName);
    // Auto-save the suggestion
    if (!repository.isEmpty())
    {
      image.repository = repository.toStdString();
      m_ImageManager->RemoveImage(imageName.toStdString());
      m_ImageManager->AddImage(image);
      m_ImageManager->SaveToPreferences();
    }
  }
  
  m_Controls.repositoryEdit->setText(repository);
  m_Controls.notesEdit->setText(QString::fromStdString(image.notes));
  m_Controls.fetchHelpButton->setEnabled(true);
  
  m_Controls.repositoryEdit->blockSignals(false);
  m_Controls.notesEdit->blockSignals(false);
  
  // Automatically fetch help text for the selected image
  OnFetchHelp();
}

void QmitkContainerManagerView::ClearDetailsView()
{
  m_CurrentSelectedImage.clear();
  
  m_Controls.repositoryEdit->blockSignals(true);
  m_Controls.notesEdit->blockSignals(true);
  
  m_Controls.repositoryEdit->clear();
  m_Controls.notesEdit->clear();
  m_Controls.repositoryEdit->setReadOnly(true);
  m_Controls.notesEdit->setReadOnly(true);
  m_Controls.repositoryEdit->setEnabled(false);
  m_Controls.notesEdit->setEnabled(false);
  m_Controls.helpTextEdit->clear();
  m_Controls.fetchHelpButton->setEnabled(false);
  
  m_Controls.repositoryEdit->blockSignals(false);
  m_Controls.notesEdit->blockSignals(false);
}

void QmitkContainerManagerView::OnRepositoryEditingFinished()
{
  if (m_CurrentSelectedImage.isEmpty() || !m_ImageManager)
    return;
  
  QString repository = m_Controls.repositoryEdit->text().trimmed();
  
  if (!m_ImageManager->HasImage(m_CurrentSelectedImage.toStdString()))
    return;
  
  mitk::DockerImageManager::DockerImage image = m_ImageManager->GetImage(m_CurrentSelectedImage.toStdString());
  image.repository = repository.toStdString();
  
  // Update by removing and re-adding
  m_ImageManager->RemoveImage(m_CurrentSelectedImage.toStdString());
  m_ImageManager->AddImage(image);
  m_ImageManager->SaveToPreferences();
  
  m_Controls.outputTextEdit->append(QString("<span style='color: blue;'>Updated repository for %1</span>")
                                   .arg(m_CurrentSelectedImage));
}

void QmitkContainerManagerView::OnRepositoryContextMenu(const QPoint& pos)
{
  QString repository = m_Controls.repositoryEdit->text().trimmed();
  
  if (repository.isEmpty())
    return;
  
  QMenu contextMenu(m_Controls.repositoryEdit);
  
  QAction* openInBrowserAction = contextMenu.addAction("Open in Browser");
  openInBrowserAction->setIcon(QIcon::fromTheme("internet-web-browser"));
  
  QAction* selectedAction = contextMenu.exec(m_Controls.repositoryEdit->mapToGlobal(pos));
  
  if (selectedAction == openInBrowserAction)
  {
    // Ensure the URL has a scheme
    QString url = repository;
    if (!url.startsWith("http://") && !url.startsWith("https://"))
    {
      url = "https://" + url;
    }
    
    if (QDesktopServices::openUrl(QUrl(url)))
    {
      m_Controls.outputTextEdit->append(QString("<span style='color: blue;'>Opened repository in browser: %1</span>")
                                       .arg(url));
    }
    else
    {
      m_Controls.outputTextEdit->append(QString("<span style='color: red;'>Failed to open URL: %1</span>")
                                       .arg(url));
    }
  }
}

void QmitkContainerManagerView::OnNotesEditingFinished()
{
  if (m_CurrentSelectedImage.isEmpty() || !m_ImageManager)
    return;
  
  QString notes = m_Controls.notesEdit->text().trimmed();
  
  if (!m_ImageManager->HasImage(m_CurrentSelectedImage.toStdString()))
    return;
  
  mitk::DockerImageManager::DockerImage image = m_ImageManager->GetImage(m_CurrentSelectedImage.toStdString());
  image.notes = notes.toStdString();
  
  // Update by removing and re-adding
  m_ImageManager->RemoveImage(m_CurrentSelectedImage.toStdString());
  m_ImageManager->AddImage(image);
  m_ImageManager->SaveToPreferences();
  
  m_Controls.outputTextEdit->append(QString("<span style='color: blue;'>Updated notes for %1</span>")
                                   .arg(m_CurrentSelectedImage));
}

void QmitkContainerManagerView::OnFetchHelp()
{
  if (m_CurrentSelectedImage.isEmpty())
  {
    QMessageBox::information(nullptr, "No Image Selected", "Please select an image first.");
    return;
  }
  
  if (!m_ImageManager || !m_ImageManager->HasImage(m_CurrentSelectedImage.toStdString()))
    return;
  
  mitk::DockerImageManager::DockerImage image = m_ImageManager->GetImage(m_CurrentSelectedImage.toStdString());
  QString fullImageName = QString("%1:%2").arg(QString::fromStdString(image.imageName))
                                           .arg(QString::fromStdString(image.tag));
  
  // Check if process is already running
  if (m_HelpProcess->state() != QProcess::NotRunning)
  {
    QMessageBox::warning(nullptr, "Process Running", "Help fetch is already in progress.");
    return;
  }
  
  m_Controls.helpTextEdit->clear();
  m_Controls.helpTextEdit->setPlainText("Fetching help text...");
  m_Controls.fetchHelpButton->setEnabled(false);
  
  // Run docker run <image> without arguments to get help text
  QStringList arguments;
  arguments << "run" << "--rm" << fullImageName << "--help";
  
  m_Controls.outputTextEdit->append(QString("<span style='color: blue;'>Fetching help for %1...</span>")
                                   .arg(fullImageName));
  
  m_HelpProcess->start("docker", arguments);
}

void QmitkContainerManagerView::OnHelpProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  m_Controls.fetchHelpButton->setEnabled(true);
  
  QString output;
  
  // Get stdout and stderr (help text might be in either)
  QString stdOut = QString::fromUtf8(m_HelpProcess->readAllStandardOutput());
  QString stdErr = QString::fromUtf8(m_HelpProcess->readAllStandardError());
  
  // Combine both outputs
  if (!stdOut.isEmpty())
    output += stdOut;
  if (!stdErr.isEmpty())
  {
    if (!output.isEmpty())
      output += "\n";
    output += stdErr;
  }
  
  if (exitStatus == QProcess::NormalExit)
  {
    if (output.isEmpty())
    {
      output = "No help text available (container exited without output).";
    }
    
    m_Controls.helpTextEdit->setPlainText(output);
    m_Controls.outputTextEdit->append(QString("<span style='color: green;'>Help text retrieved successfully (exit code: %1)</span>")
                                     .arg(exitCode));
  }
  else
  {
    m_Controls.helpTextEdit->setPlainText(QString("Failed to fetch help text.\n\nExit code: %1\n\nOutput:\n%2")
                                         .arg(exitCode).arg(output.isEmpty() ? "(no output)" : output));
    m_Controls.outputTextEdit->append(QString("<span style='color: red;'>Failed to fetch help text (exit code: %1)</span>")
                                     .arg(exitCode));
  }
}

QString QmitkContainerManagerView::SuggestRepositoryUrl(const QString& imageName) const
{
  if (imageName.isEmpty())
    return QString();
  
  // Parse the image name to extract registry and path
  // Format: [registry/]path[:tag]
  QString nameWithoutTag = imageName;
  int tagPos = imageName.lastIndexOf(':');
  if (tagPos > 0 && !imageName.mid(tagPos).contains('/'))
  {
    // Remove tag if present
    nameWithoutTag = imageName.left(tagPos);
  }
  
  // GitHub Container Registry (ghcr.io)
  if (nameWithoutTag.startsWith("ghcr.io/"))
  {
    QString path = nameWithoutTag.mid(8); // Remove "ghcr.io/"
    return QString("https://github.com/%1").arg(path);
  }
  
  // Docker Hub (docker.io or no registry prefix)
  if (nameWithoutTag.startsWith("docker.io/"))
  {
    QString path = nameWithoutTag.mid(10); // Remove "docker.io/"
    return QString("https://hub.docker.com/r/%1").arg(path);
  }
  
  // Google Container Registry
  if (nameWithoutTag.startsWith("gcr.io/") || nameWithoutTag.startsWith("us.gcr.io/") || 
      nameWithoutTag.startsWith("eu.gcr.io/") || nameWithoutTag.startsWith("asia.gcr.io/"))
  {
    int slashPos = nameWithoutTag.indexOf('/');
    if (slashPos > 0)
    {
      QString project = nameWithoutTag.mid(slashPos + 1).section('/', 0, 0);
      return QString("https://console.cloud.google.com/gcr/images/%1").arg(project);
    }
  }
  
  // Quay.io
  if (nameWithoutTag.startsWith("quay.io/"))
  {
    QString path = nameWithoutTag.mid(8); // Remove "quay.io/"
    return QString("https://quay.io/repository/%1").arg(path);
  }
  
  // Amazon ECR (public)
  if (nameWithoutTag.startsWith("public.ecr.aws/"))
  {
    QString path = nameWithoutTag.mid(15); // Remove "public.ecr.aws/"
    return QString("https://gallery.ecr.aws/%1").arg(path.section('/', 0, 0));
  }
  
  // GitLab Container Registry
  if (nameWithoutTag.startsWith("registry.gitlab.com/"))
  {
    QString path = nameWithoutTag.mid(20); // Remove "registry.gitlab.com/"
    return QString("https://gitlab.com/%1").arg(path);
  }
  
  // If no registry specified, assume Docker Hub
  if (!nameWithoutTag.contains('/'))
  {
    // Official Docker image (library/)
    return QString("https://hub.docker.com/_/%1").arg(nameWithoutTag);
  }
  else if (nameWithoutTag.count('/') == 1)
  {
    // User/organization image on Docker Hub
    return QString("https://hub.docker.com/r/%1").arg(nameWithoutTag);
  }
  
  // Unknown registry or complex path
  return QString();
}


