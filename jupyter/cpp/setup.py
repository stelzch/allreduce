from setuptools import setup, Extension

module1 = Extension('radtree',
        sources=['tree.cpp', '../../src/critical_path.cpp'],
        extra_compile_args=["-O3", "-fopenmp", "-I../../src"],
        extra_link_args=['-lgomp'])

setup(name='RADTree',
        version='1.0',
        packages=['RADTree'],
        description='Fast calculations for distributed binary trees',
        ext_modules=[module1])
