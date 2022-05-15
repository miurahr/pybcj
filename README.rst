=======================================
BCJ(Branch-Call-Jump) filter for python
=======================================

.. image:: https://badge.fury.io/py/pybcj.svg
  :target: https://badge.fury.io/py/pybcj

.. image:: https://img.shields.io/conda/vn/conda-forge/pybcj
  :target: https://anaconda.org/conda-forge/pybcj

.. image:: https://github.com/miurahr/pybcj/workflows/Run%20Tox%20tests/badge.svg
  :target: https://github.com/miurahr/pybcj/actions

.. image:: https://coveralls.io/repos/github/miurahr/pybcj/badge.svg?branch=main
  :target: https://coveralls.io/github/miurahr/pybcj?branch=main


In data compression, BCJ, short for Branch-Call-Jump, refers to a technique that improves the compression of
machine code of executable binaries by replacing relative branch addresses with absolute ones.
This allows a LZMA compressor to identify duplicate targets and archive higher compression rate.

BCJ is used in 7-zip compression utility as default filter for executable binaries.

pybcj is a python bindings with BCJ implementation by C language.
The C codes are derived from p7zip, portable 7-zip implementation.
pybcj support Intel/Amd x86/x86_64, Arm/Arm64, ArmThumb, Sparc, PPC, and IA64.


Development status
==================

A development status is considered as ``Alpha`` state.


Installation
============

As usual, you can install pybcj using python standard pip command.

.. code-block::

    pip install pybcj

Alternatively, one can also use conda:

.. code-block::

    conda install -c conda-forge pybcj

WARNING
-------

* When use it on MSYS2/Mingw64 environment, please set environment variable
  `SETUPTOOLS_USE_DISTUTILS=stdlib` to install.

License
=======

pybcj library is provided under
  SPDX-License-Identifier: LGPL-2.1-or-later
  SPDX-URL: https://spdx.org/licenses/LGPL-2.1-or-later.html

* Copyright (C) 2020-2022 Hiroshi Miura

* 7-Zip Copyright (C) 1999-2010 Igor Pavlov
* LZMA SDK Copyright (C) 1999-2010 Igor Pavlov

LGPL-2.1 license is stated at LICENSE file.
