configure_file(config-kjobwidgets.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kjobwidgets.h )

add_library(KF5JobWidgets)
add_library(KF5::JobWidgets ALIAS KF5JobWidgets)

set_target_properties(KF5JobWidgets PROPERTIES
    VERSION     ${KJOBWIDGETS_VERSION}
    SOVERSION   ${KJOBWIDGETS_SOVERSION}
    EXPORT_NAME JobWidgets
)

ecm_create_qm_loader(KF5JobWidgets kjobwidgets5_qt)

set(kjobwidgets_dbus_SRCS)
if (TARGET Qt5::DBus)
    qt_add_dbus_interfaces(kjobwidgets_dbus_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.JobViewServer.xml )
    qt_add_dbus_interface(kjobwidgets_dbus_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.JobView.xml jobviewiface )

    qt_add_dbus_interfaces(kjobwidgets_dbus_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.JobViewServerV2.xml )
    qt_add_dbus_interface(kjobwidgets_dbus_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.JobViewV3.xml jobviewv3iface )
endif()

target_sources(KF5JobWidgets PRIVATE
    kabstractwidgetjobtracker.cpp
    kdialogjobuidelegate.cpp
    kjobtrackerformatters.cpp
    kjobwidgets.cpp
    kstatusbarjobtracker.cpp
    kwidgetjobtracker.cpp
    ${kjobwidgets_dbus_SRCS}
)
if (TARGET Qt5::DBus)
    target_sources(KF5JobWidgets PRIVATE
        kuiserverjobtracker.cpp
        kuiserverv2jobtracker.cpp
    )
endif()

ecm_qt_declare_logging_category(KF5JobWidgets
    HEADER debug.h
    IDENTIFIER KJOBWIDGETS
    CATEGORY_NAME kf.jobwidgets
    OLD_CATEGORY_NAMES kf5.kjobwidgets
    DESCRIPTION "KJobWidgets"
    EXPORT KJOBWIDGETS
)

ecm_generate_export_header(KF5JobWidgets
    BASE_NAME KJobWidgets
    GROUP_BASE_NAME KF
    VERSION ${KF_VERSION}
    DEPRECATED_BASE_VERSION 0
    DEPRECATION_VERSIONS 5.79
    EXCLUDE_DEPRECATED_BEFORE_AND_AT ${EXCLUDE_DEPRECATED_BEFORE_AND_AT}
)

target_include_directories(KF5JobWidgets INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF5}/KJobWidgets>")

target_link_libraries(KF5JobWidgets
   PUBLIC
      Qt5::Widgets
      KF5::CoreAddons    # KJob
   PRIVATE
      KF5::WidgetsAddons # KSqueezedTextLabel
)
if (TARGET Qt5::DBus)
    target_link_libraries(KF5JobWidgets PRIVATE Qt5::DBus)
endif()

if (HAVE_X11)
  find_package(Qt5X11Extras ${REQUIRED_QT_VERSION} REQUIRED NO_MODULE)
  target_link_libraries(KF5JobWidgets PRIVATE Qt5::X11Extras)
endif ()

ecm_generate_headers(KJobWidgets_HEADERS
  HEADER_NAMES
  KAbstractWidgetJobTracker
  KDialogJobUiDelegate
  KUiServerJobTracker
  KUiServerV2JobTracker
  KJobWidgets
  KStatusBarJobTracker
  KWidgetJobTracker

  REQUIRED_HEADERS KJobWidgets_HEADERS
)

find_package(PythonModuleGeneration)

if (PythonModuleGeneration_FOUND)
  ecm_generate_python_binding(
    TARGET KF5::JobWidgets
    PYTHONNAMESPACE PyKF5
    MODULENAME KJobWidgets
    SIP_DEPENDS
      QtWidgets/QtWidgetsmod.sip
      PyKF5/KCoreAddons/KCoreAddonsmod.sip
    HEADERS
      kabstractwidgetjobtracker.h
      kdialogjobuidelegate.h
      kuiserverjobtracker.h
      kuiserverv2jobtracker.h
      kjobwidgets.h
      kstatusbarjobtracker.h
      kwidgetjobtracker.h
  )
endif()

install(TARGETS KF5JobWidgets EXPORT KF5JobWidgetsTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
  org.kde.JobViewServer.xml
  DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR}
  RENAME kf5_org.kde.JobViewServer.xml)

install(FILES
  org.kde.JobView.xml
  DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR}
  RENAME kf5_org.kde.JobView.xml)

install(FILES
  org.kde.JobViewV2.xml
  DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR}
  RENAME kf5_org.kde.JobViewV2.xml)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/kjobwidgets_export.h
  ${KJobWidgets_HEADERS}
  DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/KJobWidgets COMPONENT Devel
)

ecm_qt_install_logging_categories(
    EXPORT KJOBWIDGETS
    FILE kjobwidgets.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)

if(BUILD_QCH)
    ecm_add_qch(
        KF5JobWidgets_QCH
        NAME KJobWidgets
        BASE_NAME KF5JobWidgets
        VERSION ${KF_VERSION}
        ORG_DOMAIN org.kde
        SOURCES # using only public headers, to cover only public API
            ${KJobWidgets_HEADERS}
        MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        LINK_QCHS
            KF5CoreAddons_QCH
            Qt5Widgets_QCH
            Qt5Gui_QCH
            Qt5Core_QCH
        INCLUDE_DIRS
            ${CMAKE_CURRENT_BINARY_DIR}
        BLANK_MACROS
            KJOBWIDGETS_EXPORT
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
    )
endif()

include(ECMGeneratePriFile)
ecm_generate_pri_file(BASE_NAME KJobWidgets LIB_NAME KF5JobWidgets DEPS "widgets KCoreAddons" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR_KF5}/KJobWidgets)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
