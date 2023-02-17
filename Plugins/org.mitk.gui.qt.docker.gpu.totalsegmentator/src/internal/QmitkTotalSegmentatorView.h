/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef QmitkExampleView_h
#define QmitkExampleView_h

#include <berryISelectionListener.h>
#include <QmitkAbstractView.h>
#include <QmitkSingleNodeSelectionWidget.h>
#include <ui_QmitkTotalSegmentatorViewControls.h>

// All views in MITK derive from QmitkAbstractView. You have to override
// at least the two methods CreateQtPartControl() and SetFocus().
class QmitkTotalSegmentatorView : public QmitkAbstractView
{
  // As ExampleView derives from QObject and we want to use the Qt
  // signal and slot mechanism, we must not forget the Q_OBJECT macro.
  // This header file also has to be listed in MOC_H_FILES in files.cmake
  // so that the Qt Meta-Object compiler can find and process this
  // class declaration.
  Q_OBJECT

public:
  // This is a tricky one and will give you some headache later on in
  // your debug sessions if it has been forgotten. Also, don't forget
  // to initialize it in the implementation file.
  static const std::string VIEW_ID;

  // In this method we initialize the GUI components and connect the
  // associated signals and slots.
  void CreateQtPartControl(QWidget* parent) override;

private slots:
  void OnStartTotalSegmentator();

private:
  // Typically a one-liner. Set the focus to the default widget.
  void SetFocus() override;

  void EnableWidgets(bool enable);

  // Generated from the associated UI file, it encapsulates all the widgets
  // of our view.
  Ui::QmitkTotalSegmentatorViewControls m_Controls;
  const std::vector<std::string> m_TotalSegmentatorResultFileNames = {
    "adrenal_gland_left.nii.gz",
    "adrenal_gland_right.nii.gz",
    "aorta.nii.gz",
    "autochthon_left.nii.gz",
    "autochthon_right.nii.gz",
    "brain.nii.gz",
    "clavicula_left.nii.gz",
    "clavicula_right.nii.gz",
    "colon.nii.gz",
    "duodenum.nii.gz",
    "esophagus.nii.gz",
    "face.nii.gz",
    "femur_left.nii.gz",
    "femur_right.nii.gz",
    "gallbladder.nii.gz",
    "gluteus_maximus_left.nii.gz",
    "gluteus_maximus_right.nii.gz",
    "gluteus_medius_left.nii.gz",
    "gluteus_medius_right.nii.gz",
    "gluteus_minimus_left.nii.gz",
    "gluteus_minimus_right.nii.gz",
    "heart_atrium_left.nii.gz",
    "heart_atrium_right.nii.gz",
    "heart_myocardium.nii.gz",
    "heart_ventricle_left.nii.gz",
    "heart_ventricle_right.nii.gz",
    "hip_left.nii.gz",
    "hip_right.nii.gz",
    "humerus_left.nii.gz",
    "humerus_right.nii.gz",
    "iliac_artery_left.nii.gz",
    "iliac_artery_right.nii.gz",
    "iliac_vena_left.nii.gz",
    "iliac_vena_right.nii.gz",
    "iliopsoas_left.nii.gz",
    "iliopsoas_right.nii.gz",
    "inferior_vena_cava.nii.gz",
    "kidney_left.nii.gz",
    "kidney_right.nii.gz",
    "liver.nii.gz",
    "lung_lower_lobe_left.nii.gz",
    "lung_lower_lobe_right.nii.gz",
    "lung_middle_lobe_right.nii.gz",
    "lung_upper_lobe_left.nii.gz",
    "lung_upper_lobe_right.nii.gz",
    "pancreas.nii.gz",
    "portal_vein_and_splenic_vein.nii.gz",
    "pulmonary_artery.nii.gz",
    "rib_left_1.nii.gz",
    "rib_left_2.nii.gz",
    "rib_left_3.nii.gz",
    "rib_left_4.nii.gz",
    "rib_left_5.nii.gz",
    "rib_left_6.nii.gz",
    "rib_left_7.nii.gz",
    "rib_left_8.nii.gz",
    "rib_left_9.nii.gz",
    "rib_left_10.nii.gz",
    "rib_left_11.nii.gz",
    "rib_left_12.nii.gz",
    "rib_right_1.nii.gz",
    "rib_right_2.nii.gz",
    "rib_right_3.nii.gz",
    "rib_right_4.nii.gz",
    "rib_right_5.nii.gz",
    "rib_right_6.nii.gz",
    "rib_right_7.nii.gz",
    "rib_right_8.nii.gz",
    "rib_right_9.nii.gz",
    "rib_right_10.nii.gz",
    "rib_right_11.nii.gz",
    "rib_right_12.nii.gz",
    "sacrum.nii.gz",
    "scapula_left.nii.gz",
    "scapula_right.nii.gz",
    "small_bowel.nii.gz",
    "spleen.nii.gz",
    "stomach.nii.gz",
    "trachea.nii.gz",
    "urinary_bladder.nii.gz",
    "vertebrae_C1.nii.gz",
    "vertebrae_C2.nii.gz",
    "vertebrae_C3.nii.gz",
    "vertebrae_C4.nii.gz",
    "vertebrae_C5.nii.gz",
    "vertebrae_C6.nii.gz",
    "vertebrae_C7.nii.gz",
    "vertebrae_L1.nii.gz",
    "vertebrae_L2.nii.gz",
    "vertebrae_L3.nii.gz",
    "vertebrae_L4.nii.gz",
    "vertebrae_L5.nii.gz",
    "vertebrae_T1.nii.gz",
    "vertebrae_T2.nii.gz",
    "vertebrae_T3.nii.gz",
    "vertebrae_T4.nii.gz",
    "vertebrae_T5.nii.gz",
    "vertebrae_T6.nii.gz",
    "vertebrae_T7.nii.gz",
    "vertebrae_T8.nii.gz",
    "vertebrae_T9.nii.gz",
    "vertebrae_T10.nii.gz",
    "vertebrae_T11.nii.gz",
    "vertebrae_T12.nii.gz",
    "preview.png"
  };
};

#endif
