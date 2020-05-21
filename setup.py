#!/usr/bin/env python3
# encoding: utf-8

from distutils.core import setup, Extension

pyrtsignal_module = Extension('pyrtsignal', sources = ['pyrtsignal.cc'])

setup(name='pyrtsignal',
      version='0.1.0',
      description='Python real time signal module',
      ext_modules=[pyrtsignal_module])
