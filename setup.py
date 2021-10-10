import os
import platform
import sys

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext
from setuptools.command.egg_info import egg_info

sources = [
    "lib/Bra.c",
    "lib/Bra86.c",
    "lib/BraIA64.c"
]

_bcj_extension = Extension("pybcj._bcj", sources)
kwargs = {"include_dirs": ["lib"], "library_dirs": [], "libraries": [], "sources": sources, "define_macros": []}


def has_option(option):
    if option in sys.argv:
        sys.argv = [s for s in sys.argv if s != option]
        return True
    else:
        return False


if has_option("--cffi") or platform.python_implementation() == "PyPy":
    packages = ["pybcj", "pybcj.cffi"]
    kwargs["module_name"] = "pybcj.cffi._cffi_bcj"
    sys.path.append("src/ext")
    import ffi_build
    ffi_build.set_kwargs(**kwargs)
    binary_extension = ffi_build.ffibuilder.distutils_extension()
else:
    packages = ["pybcj", "pybcj.c"]
    kwargs["name"] = "pybcj.c._bcj"
    kwargs["sources"].append("src/ext/_bcjmodule.c")
    binary_extension = Extension(**kwargs)


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
            os.remove(os.path.join(self.egg_info, 'SOURCES.txt'))
        except FileNotFoundError:
            pass
        super().run()


setup(use_scm_version={"local_scheme": "no-local-version"},
      package_dir={"": "src"},
      ext_modules=[binary_extension],
      packages=packages,
      cmdclass={"build_ext": build_ext_compiler_check, "egg_info": my_egg_info},
      )
