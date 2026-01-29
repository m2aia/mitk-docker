project(org_mitk_gui_qt_docker_containermanager)

mitk_create_plugin(
  EXPORT_DIRECTIVE CONTAINERMANAGER_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS PRIVATE MitkQtWidgetsExt MitkDocker
)
