# -*- python -*-

import os, stat, sys

from distutils.core import setup, Extension, Command
from distutils.errors import DistutilsExecError
from distutils.command.install import install as base_install

version = '1.2.3'


bibtex = [
    'accents.c',
    'author.c',
    'bibparse.c',
    'biblex.c',
    'bibtex.c',
    'bibtexmodule.c',
    'entry.c',
    'field.c',
    'reverse.c',
    'source.c',
    'stringutils.c',
    'struct.c'
    ]


def error (msg):
    sys.stderr.write ('setup.py: error: %s\n' % msg)
    return


# Obtain path for Glib

includes = []
libs     = []
libdirs  = []

def pread (cmd):
    fd = os.popen (cmd)
    data = fd.read ()

    return (data, fd.close ())

include, ix = pread ('pkg-config glib-2.0 --cflags')
library, lx = pread ('pkg-config glib-2.0 --libs')

if ix or lx:
    error ('cannot find Glib-2.0 installation parameters.')
    error ('please check that your glib-2.0 development package')
    error ('has been installed.')
    sys.exit (1)


# Split the path into pieces
for inc in include.split (' '):
    inc = inc.strip ()
    if not inc: continue

    if inc [:2] == '-I':
        includes.append (inc [2:])


for lib in library.split (' '):
    lib = lib.strip ()
    if not lib: continue

    if lib [:2] == '-l':
        libs.append (lib [2:])

    if lib [:2] == '-L':
        libdirs.append (lib [2:])


# Check the state of the generated lex and yacc files
def rebuild (src, deps):

    st = os.stat (src) [stat.ST_MTIME]

    for dep in deps:
        try:  dt = os.stat (dep) [stat.ST_MTIME]
        except OSError: return True

        if st > dt: return True

    return False


def rename (src, dst):

    try: os.unlink (dst)
    except OSError: pass

    os.rename (src, dst)
    return


if rebuild ('bibparse.y', ['bibparse.c',
                           'bibparse.h']):
    print "rebuilding from bibparse.y"

    os.system ('bison -y -d -t -p bibtex_parser_ bibparse.y')

    rename ('y.tab.c', 'bibparse.c')
    rename ('y.tab.h', 'bibparse.h')


if rebuild ('biblex.l', ['biblex.c']):
    print "rebuilding from biblex.l"

    os.system ('flex -Pbibtex_parser_ biblex.l')

    rename ('lex.bibtex_parser_.c', 'biblex.c')




class run_check (Command):

    """ Run all of the tests for the package using uninstalled (local)
    files """
    
    description = "Automatically run the test suite for the package."

    user_options = []

    def initialize_options(self):
        self.build_lib = None
        return


    def finalize_options(self):
        # Obtain the build_lib directory from the build command
        self.set_undefined_options ('build', ('build_lib', 'build_lib'))
        return

    def run(self):
        # Ensure the extension is built
        self.run_command ('build')
        
        # test the uninstalled extensions
        libdir = os.path.join (os.getcwd (), self.build_lib)

        sys.path.insert (0, libdir)

        import testsuite

        try:
            failures = testsuite.run ()

        except RuntimeError, msg:
            sys.stderr.write ('error: %s\n' % msg)
            raise DistutilsExecError ('please consult the "Troubleshooting" section in the README file.')

        if failures > 0:
            raise DistutilsExecError ('check failed.')
        return
    


class run_install (base_install):

    def run(self):
        # The code must pass the tests before being installed
        self.run_command ('check')
        
        base_install.run (self)
        return
    
        
# Actual compilation

setup (name = "python-bibtex",
       version = version,

       description = "A Python extension to parse BibTeX files",
       author = "Frederic Gobry",
       author_email = 'gobry@pybliographer.org',
       url = 'http://pybliographer.org/',

       license = 'GPL',
       
       cmdclass = { 'check':   run_check,
                    'install': run_install },
       
       long_description = \
'''
This module contains two extensions needed for pybliographer:

   - a bibtex parser
   - a simple binding to GNU Recode

It requires the following libraries to be installed:

   - Glib-2.0 (and its development headers)
   - GNU Recode 3.5 (and its development headers)

''',

       ext_modules = [

    Extension("_bibtex", bibtex,
              include_dirs = includes,
              library_dirs = libdirs,
              define_macros = [('G_LOG_DOMAIN', '"BibTeX"')],
              libraries = libs + ['recode']),

    Extension("_recode", ["recodemodule.c"],
              include_dirs = includes,
              libraries = ['recode'])
    ])
