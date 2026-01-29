/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#pragma once

#include <berryISelectionListener.h>
#include <QmitkAbstractView.h>
#include <QFutureWatcher>
#include <QProcess>
#include <QMap>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>

namespace mitk
{
  class DockerImageManager;
}

// Generated from the associated UI file, it encapsulates all the widgets
// of our view.
#include <ui_ViewControls.h>

/**
 * @brief View for managing M2aia Docker containers
 * 
 * This view provides functionality to:
 * - Add and manage Docker image URLs
 * - Query and display available versions/tags for each image
 * - Pull Docker images that are not locally available
 * - Monitor container status
 */
class QmitkContainerManagerView : public QmitkAbstractView
{
  Q_OBJECT

public:
  static const std::string VIEW_ID;

  void CreateQtPartControl(QWidget* parent) override;

private slots:
  void OnAddImage();
  void OnRemoveImage();
  void OnPullImage();
  void OnReloadImages();
  void OnDockerProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void OnDockerProcessError(QProcess::ProcessError error);
  void OnDockerProcessOutput();
  void OnQueryTagsFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void OnCheckLocalImagesFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void OnImageTableContextMenu(const QPoint& pos);
  void OnImageTableItemChanged(QTableWidgetItem* item);
  void OnLoadJson();
  void OnSaveJson();
  void OnImageTableSelectionChanged();
  void OnRepositoryEditingFinished();
  void OnRepositoryContextMenu(const QPoint& pos);
  void OnNotesEditingFinished();
  void OnFetchHelp();
  void OnHelpProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
  void SetFocus() override;
  void EnableWidgets(bool enable);
  void UpdateDockerStatus();
  void QueryImageTags(const QString& imageName);
  void CheckLocalImages();
  void AddImageRow(const QString& imageName);
  void UpdatePullButtons();
  bool IsImageLocallyAvailable(const QString& imageName, const QString& tag);
  void LoadPersistedImages();
  void UpdateDetailsView(const QString& imageName);
  void ClearDetailsView();
  QString SuggestRepositoryUrl(const QString& imageName) const;
  
  // UI controls
  Ui::ViewControls m_Controls;
  
  // Docker process management
  QProcess* m_DockerProcess;
  QProcess* m_QueryProcess;
  QProcess* m_CheckProcess;
  QProcess* m_HelpProcess;
  
  // Persistent image management
  mitk::DockerImageManager* m_ImageManager;
  
  // Image management
  struct ImageInfo {
    QString imageName;
    QPushButton* pullButton;
    QPushButton* removeButton;
    int tableRow;
  };
  
  QMap<QString, ImageInfo> m_Images;
  QStringList m_LocalImages;
  QString m_CurrentQueryImage;
  QString m_CurrentPullImage;
  QString m_CurrentSelectedImage;
};
