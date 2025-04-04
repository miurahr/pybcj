# pybcj library.
# Copyright 2020-2022 Hiroshi Miura
# SPDX-License-Identifier: LGPL-2.1-or-later
#
import os
import sys

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext
from setuptools.command.egg_info import egg_info

sources = ["src/ext/Bra.c", "src/ext/Bra86.c", "src/ext/BraIA64.c", "src/ext/_bcjmodule.c"]
kwargs = {
    "name": "bcj._bcj",
    "include_dirs": ["src/ext"],
    "library_dirs": [],
    "libraries": [],
    "sources": sources,
    "define_macros": [],
}
packages = ["bcj"]
binary_extension = Extension(**kwargs)


def has_option(option):
    if option in sys.argv:
        sys.argv = [s for s in sys.argv if s != option]
        return True
    else:
        return False


WARNING_AS_ERROR = has_option("--warning-as-error")


class build_ext_compiler_check(build_ext):
    def build_extensions(self):
        for extension in self.extensions:
            if self.compiler.compiler_type.lower() in ("unix", "mingw32"):
                if WARNING_AS_ERROR:
                    extension.extra_compile_args.append("-Werror")
            elif self.compiler.compiler_type.lower() == "msvc":
                # /GF eliminates duplicate strings
                # /Gy does function level linking
                more_options = ["/GF", "/Gy"]
                if WARNING_AS_ERROR:
                    more_options.append("/WX")
                extension.extra_compile_args.extend(more_options)
        super().build_extensions()


# Work around pypa/setuptools#436.
class my_egg_info(egg_info):
    def run(self):
        try:
            os.remove(os.path.join(self.egg_info, "SOURCES.txt"))
        except FileNotFoundError:
            pass
        super().run()


setup(
    package_dir={"": "src"},
    ext_modules=[binary_extension],
    packages=packages,
    cmdclass={"build_ext": build_ext_compiler_check, "egg_info": my_egg_info},
)
