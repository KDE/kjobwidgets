
include(ECMMarkAsTest)

find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)

macro(kjobwidgets_executable_tests)
  foreach(_testname ${ARGN})
    add_executable(${_testname} ${_testname}.cpp)
    target_link_libraries(${_testname} Qt5::Test KF5::JobWidgets)
    ecm_mark_as_test(${_testname})
  endforeach(_testname)
endmacro()

kjobwidgets_executable_tests(
   kjobtrackerstest
)
