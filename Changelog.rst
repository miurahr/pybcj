===============
PyBCJ ChangeLog
===============

All notable changes to this project will be documented in this file.

`Unreleased`_
=============

v1.0.6_
=======

Changed
-------
- ci: release workflows

Added
-----
- ci: add GitHub Actions as trusted publisher for PyPi

Removed
-------
- ci: azure-pipelines


v1.0.3_
=======

Added
-----
- Support python 3.13

Fixed
-----
- Allow build on git export source tree

Removed
-------
- Support for python 3.8


`v1.0.2`_
=========

Added
-----
- Support python 3.12
- Add cibuildwheel config

`v1.0.1`_
=========

Fixed
-----
- Add missing source distribution on pypi (#1)

`v1.0.0`_
=========

Changed
-------
- test: zipping test case data
- Add type hint
- test: use flake8 plugins
  * black, colors, isort, typing
- Drop github workflows
- Move forge to codeberg.org

Fixed
-----
- gitea: issue template

`v0.6.1`_
=========

Changed
-------

- Publish wheels for python 3.11 beta

`v0.6.0`_
=========

Changed
-------

- Change internal package path for extension (#17)
  - Deprecated package path ``bcj.c``. now we have
    ``bcj._bcj`` and ``bcj._bcjfilter`` and user should
    use ``import bcj``

Added
-----

- Add pure python filter for PyPy (#17)
  that is ``bcj._bcjfilter``
- Add more test cases (#17)

`v0.5.3`_
=========

Fixed
-----

- Fix segmentation fault in a proper condition (#16)

Changed
-------

- Update copyright header.

`v0.5.2`_
=========

Fixed
-----

- Fix pyproject.toml: add missing dynamic property(#14)


`v0.5.1`_
=========

Changed
-------

- Update pyproject.toml to add project section(#13)
- Test on msys2/mingw64(#11)


`v0.5.0`_
=========

Fixed
-----

- Failed to filter when multiple encode/decode call.
- Wrong data size when auto flush happened

Added
-----

- Add fuzzer test.

Changed
-------

- Black/PEP8 rules
- Change status to Beta.

`v0.2.0`_
=========

- Add ARM, ARMT, PPC, Sparc and IA64 encoder/decoder.
- Package is now bcj

`v0.1.1`_
=========

- Introduce BCJEncoder and BCJDecoder.

v0.1.0
======

- First import.


.. _Unreleased: https://codeberg.org/miurahr/pybcj/compare/v1.0.6...HEAD
.. _v1.0.6: https://codeberg.org/miurahr/pybcj/compare/v1.0.3...v1.0.6
.. _v1.0.3: https://codeberg.org/miurahr/pybcj/compare/v1.0.2...v1.0.3
.. _v1.0.2: https://codeberg.org/miurahr/pybcj/compare/v1.0.1...v1.0.2
.. _v1.0.1: https://codeberg.org/miurahr/pybcj/compare/v1.0.0...v1.0.1
.. _v1.0.0: https://codeberg.org/miurahr/pybcj/compare/v0.6.1...v1.0.0
.. _v0.6.1: https://codeberg.org/miurahr/pybcj/compare/v0.6.0...v0.6.1
.. _v0.6.0: https://codeberg.org/miurahr/pybcj/compare/v0.5.3...v0.6.0
.. _v0.5.3: https://codeberg.org/miurahr/pybcj/compare/v0.5.2...v0.5.3
.. _v0.5.2: https://codeberg.org/miurahr/pybcj/compare/v0.5.1...v0.5.2
.. _v0.5.1: https://codeberg.org/miurahr/pybcj/compare/v0.5.0...v0.5.1
.. _v0.5.0: https://codeberg.org/miurahr/pybcj/compare/v0.2.0...v0.5.0
.. _v0.2.0: https://codeberg.org/miurahr/pybcj/compare/v0.1.1...v0.2.0
.. _v0.1.1: https://codeberg.org/miurahr/pybcj/compare/v0.1.0...v0.1.1
