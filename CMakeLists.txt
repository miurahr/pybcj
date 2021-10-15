cmake_minimum_required(VERSION 3.19)
project(pybcj C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# TARGET PYTHON version
set(PY_VERSION 3.8)
set(Python_FIND_IMPLEMENTATIONS CPython)
set(VENV_PATH "${CMAKE_BINARY_DIR}/venv")
set(DEBUG_BUILD ON)

# ##################################################################################################
# Configuration for python-ext
set(Python_FIND_STRATEGY VERSION)
find_package(Python ${PY_VERSION}.0...${PY_VERSION}.99 COMPONENTS Interpreter Development)
set(PY_EXT_FILE _bcj)
set(PY_EXT_DIR src/bcj/c)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/get_ext.py
     "import sysconfig\nprint(sysconfig.get_config_var('EXT_SUFFIX'))\n")
execute_process(
  COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/get_ext.py
  OUTPUT_VARIABLE PY_EXT_EXT
  OUTPUT_STRIP_TRAILING_WHITESPACE)
set(PY_EXT ${PY_EXT_DIR}/${PY_EXT_FILE}.${PY_EXT_EXT})
set(PY_CFFI ${PY_CFFI_DIR}/PY_CFFI_FILE}.${PY_EXT_EXT})
if(DEBUG_BUILD)
  if (WIN32)
    set(BUILD_EXT_PYTHON ${VENV_PATH}/Scripts/python_d.exe)
    set(BUILD_EXT_OPTION -g)
  else()
    set(BUILD_EXT_PYTHON ${Python_EXECUTABLE})
    set(BUILD_EXT_OPTION -g)
  endif()
else()
  set(BUILD_EXT_PYTHON ${Python_EXECUTABLE})
  set(BUILD_EXT_OPTION)
endif()
add_custom_target(
  generate_ext
  BYPRODUCTS ${PY_EXT}
  COMMAND ${BUILD_EXT_PYTHON} setup.py build_ext ${BUILD_EXT_OPTION} --inplace
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS venv.stamp
  SOURCES ${pybcj_sources})
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
set(VPKG_PATH_A "${VENV_PATH}/lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages/")
set(VPKG_PATH_B "${VENV_PATH}/Lib/site-packages/")

# ##################################################################################################
# For pytest
file(
  WRITE ${CMAKE_CURRENT_BINARY_DIR}/pytest_runner.cpp
  "
#include <string>
#include <filesystem>
#include <Python.h>
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
    std::filesystem::path vsite_path_a = \"${VPKG_PATH_A}\";
    std::filesystem::path vsite_path_b = \"${VPKG_PATH_B}\";
    std::string pycode =
        \"import sys\\n\"
        \"sys.path.append('src')\\n\"
        \"sys.path.append('\" + vsite_path_a.string() + \"')\\n\"
        \"sys.path.append('\" + vsite_path_b.string() + \"')\\n\"
        \"import pytest\\n\"
        \"pytest.main(\" + args + \")\\n\";
    wchar_t * program_name = Py_DecodeLocale(argv[0], NULL);
    Py_SetProgramName(program_name);
    Py_Initialize();
    PyRun_SimpleString(&*pycode.begin());
    Py_Finalize();
    return 0;
}")
add_executable(pytest_runner ${CMAKE_CURRENT_BINARY_DIR}/pytest_runner.cpp)
target_include_directories(pytest_runner PRIVATE ${Python_INCLUDE_DIRS})
target_link_libraries(pytest_runner PRIVATE ${Python_LIBRARIES})
add_dependencies(pytest_runner venv.stamp generate_ext)
