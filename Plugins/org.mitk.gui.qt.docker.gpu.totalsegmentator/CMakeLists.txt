project(org_mitk_gui_qt_docker_gpu_totalsegmentator)

mitk_create_plugin(
  EXPORT_DIRECTIVE EXAMPLE_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS PRIVATE MitkQtWidgetsExt MitkDocker MitkMultilabel
)
