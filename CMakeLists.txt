cmake_minimum_required(VERSION 3.19)
project(pybcj C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# TARGET PYTHON version
set(PY_VERSION 3.12)
set(Python_FIND_IMPLEMENTATIONS CPython)
set(VENV_PATH "${CMAKE_BINARY_DIR}/venv")
set(DEBUG_BUILD ON)

# ##################################################################################################
# Configuration for python-ext
set(Python_FIND_STRATEGY VERSION)
find_package(Python ${PY_VERSION}.0...${PY_VERSION}.99 COMPONENTS Interpreter Development)
set(PY_EXT_FILE _bcj)
set(PY_EXT_DIR src/bcj)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/get_ext.py
     "import sysconfig\nprint(sysconfig.get_config_var('EXT_SUFFIX'))\n")
execute_process(
  COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/get_ext.py
  OUTPUT_VARIABLE PY_EXT_EXT
  OUTPUT_STRIP_TRAILING_WHITESPACE)
set(PY_EXT ${PY_EXT_DIR}/${PY_EXT_FILE}.${PY_EXT_EXT})
set(PY_CFFI ${PY_CFFI_DIR}/PY_CFFI_FILE}.${PY_EXT_EXT})
# bulid ext by setup.py
if (WIN32)
  if(DEBUG_BUILD)
    set(BUILD_EXT_PYTHON ${VENV_PATH}/Scripts/python_d.exe)
  else()
    set(BUILD_EXT_PYTHON ${VENV_PATH}/Scripts/python.exe)
  endif()
  set(BUILD_EXT_OPTION)
else()
  set(BUILD_EXT_PYTHON ${VENV_PATH}/bin/python)
  set(BUILD_EXT_OPTION --warning-as-error)
endif()
set(pybcj_sources src/ext/Bra.c src/ext/Bra86.c src/ext/BraIA64.c)
set(pybcj_ext_src src/ext/_bcjmodule.c)
add_custom_target(
  generate_ext
  BYPRODUCTS ${PY_EXT}
  COMMAND ${BUILD_EXT_PYTHON} setup.py build_ext ${BUILD_EXT_OPTION} --inplace
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS venv.stamp
  SOURCES ${pybcj_sources} ${pybcj_ext_src})
add_library(_pybcj_ext MODULE ${pybcj_sources} ${pybcj_ext_src})
target_include_directories(_pybcj_ext PRIVATE ${Python_INCLUDE_DIRS} src/ext)
target_link_libraries(_pybcj_ext PRIVATE ${Python_LIBRARIES})
# ##################################################################################################
# create virtualenv
file(
        WRITE ${CMAKE_CURRENT_BINARY_DIR}/requirements.txt
        "
hypothesis
pytest>=6.0
")
if (WIN32)
  set(PIP_COMMAND ${VENV_PATH}/Scripts/pip.exe)
else()
  set(PIP_COMMAND ${VENV_PATH}/bin/pip)
endif()
add_custom_target(
        venv.stamp
        BYPRODUCTS venv.stamp
        COMMAND ${Python_EXECUTABLE} -m venv ${VENV_PATH}
        COMMAND ${PIP_COMMAND} install -r ${CMAKE_BINARY_DIR}/requirements.txt
        COMMAND ${CMAKE_COMMAND} -E touch venv.stamp)
set(SRC_PATH "${CMAKE_SOURCE_DIR}/src")
set(VPKG_PATH_A "${VENV_PATH}/lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages/")
set(VPKG_PATH_B "${VENV_PATH}/Lib/site-packages/")
set(VPKG_PATH_C "${CMAKE_BINARY_DIR}")

# ##################################################################################################
# For pytest
file(
        WRITE ${CMAKE_CURRENT_BINARY_DIR}/pytest_runner.cpp
        "
#include <string>
#include <filesystem>
#include <unistd.h>
int main(int argc, char **argv) {
    std::string args;
    if ( argc > 1) {
        args.append(\"[\");
        for (int i = 1; i < argc; i++) {
            if (i > 2)
                args.append(\",\");
            args.append(\"\\\"\");
            args.append(argv[i]);
            args.append(\"\\\"\");
        }
        args.append(\"]\");
    }
    std::filesystem::path src_path = \"${SRC_PATH}\";
    std::filesystem::path vsite_path_a = \"${VPKG_PATH_A}\";
    std::filesystem::path vsite_path_b = \"${VPKG_PATH_B}\";
    std::filesystem::path vsite_path_c = \"${VPKG_PATH_C}\";
    std::string pycode =
        \"import sys\\n\"
        \"sys.path.append('\" + src_path.string() + \"')\\n\"
        \"sys.path.append('\" + vsite_path_a.string() + \"')\\n\"
        \"sys.path.append('\" + vsite_path_b.string() + \"')\\n\"
        \"import pytest\\n\"
        \"pytest.main(\" + args + \")\\n\";
    execl(\"${Python_EXECUTABLE}\", \"${Python_EXECUTABLE}\", \"-c\", pycode.c_str(), (char*)0);
    return 0;
}")
add_executable(pytest_runner ${CMAKE_CURRENT_BINARY_DIR}/pytest_runner.cpp)
target_include_directories(pytest_runner PRIVATE ${Python_INCLUDE_DIRS})
target_link_libraries(pytest_runner PRIVATE ${Python_LIBRARIES})
add_dependencies(pytest_runner venv.stamp generate_ext)
