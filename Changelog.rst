===============
PyBCJ ChangeLog
===============

All notable changes to this project will be documented in this file.

`Unreleased`_
=============

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

- Fix pyproject.toml: add missing dynamic propety(#14)


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


.. _Unreleased: https://github.com/miurahr/pybcj/compare/v0.5.3...HEAD
.. _v0.5.3: https://github.com/miurahr/pybcj/compare/v0.5.2...v0.5.3
.. _v0.5.2: https://github.com/miurahr/pybcj/compare/v0.5.1...v0.5.2
.. _v0.5.1: https://github.com/miurahr/pybcj/compare/v0.5.0...v0.5.1
.. _v0.5.0: https://github.com/miurahr/pybcj/compare/v0.2.0...v0.5.0
.. _v0.2.0: https://github.com/miurahr/pybcj/compare/v0.1.1...v0.2.0
.. _v0.1.1: https://github.com/miurahr/pybcj/compare/v0.1.0...v0.1.1
