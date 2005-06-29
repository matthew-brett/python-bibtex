""" Run is the main function that will check if the recode and bibtex
modules are working """

import sys, os


def check_recode ():
    try:
        import _recode

    except SystemError:
        raise RuntimeError ('the recode library is probably broken.')

    # First, check if the recode version has the famous 3.6 bug
    rq = _recode.request ('latin1..latex')
    
    if _recode.recode (rq, 'abc') != 'abc':
        raise RuntimeError ('the _recode module is broken.')

    return 0



def check_bibtex ():

    _debug = False
    
    import _bibtex


    def checkfile (filename, strict = 1):
        
        def expand (file, entry):

            items = entry [4]
    
            for k in items.keys ():
                items [k] = _bibtex.expand (file, items [k], -1)

            return
        
        file   = _bibtex.open_file (filename, strict)
        result = open (filename + '-ok', 'r')

        line     = 1
        failures = 0
        checks   = 0
        
        while 1:

            try:
                entry = _bibtex.next (file)

                if entry is None: break
                            
                expand (file, entry)
                obtained = `entry`
                
            except IOError, msg:
                obtained = 'ParserError'
                

            if _debug: print obtained

            valid = result.readline ().strip ()
            
            if obtained != valid:
                sys.stderr.write ('error: %s: line %d: unexpected result:\n' % (
                    filename, line))
                sys.stderr.write ('error: %s: line %d:    obtained %s\n' % (
                    filename, line, obtained))
                sys.stderr.write ('error: %s: line %d:    expected %s\n' % (
                    filename, line, valid))

                failures = failures + 1

            checks = checks + 1
                
        return failures, checks

    def checkunfiltered (filename, strict = 1):
        
        def expand (file, entry):

            if entry [0] in ('preamble', 'string'): return
            
            items = entry [1] [4]
    
            for k in items.keys ():
                items [k] = _bibtex.expand (file, items [k], -1)

            return
        
        file   = _bibtex.open_file (filename, strict)
        result = open (filename + '-ok', 'r')

        line     = 1
        failures = 0
        checks   = 0
        
        while 1:

            try:
                entry = _bibtex.next_unfiltered (file)

                if entry is None: break

                expand (file, entry)
                obtained = `entry`
                
            except IOError, msg:
                obtained = 'ParserError'
                

            if _debug: print obtained

            valid = result.readline ().strip ()
            
            if obtained != valid:
                sys.stderr.write ('error: %s: line %d: unexpected result:\n' % (
                    filename, line))
                sys.stderr.write ('error: %s: line %d:    obtained %s\n' % (
                    filename, line, obtained))
                sys.stderr.write ('error: %s: line %d:    expected %s\n' % (
                    filename, line, valid))

                failures = failures + 1

            checks = checks + 1
                
        return failures, checks

    failures = 0
    checks   = 0

    
    for file in ('tests/preamble.bib',
                 'tests/string.bib',
                 'tests/simple-2.bib'):
        
        f, c = checkunfiltered (file)
        
        failures = failures + f
        checks   = checks   + c

    failures += f
    checks   += c
    
    for file in ('tests/simple.bib',
                 'tests/eof.bib',
                 'tests/paren.bib'):
        
        f, c = checkfile (file)
        
        failures = failures + f
        checks   = checks   + c

    print "testsuite: %d checks, %d failures" % (checks, failures)
    return failures



def run ():
    failures = 0
    
    failures += check_recode ()
    failures += check_bibtex ()
    
    return failures



    
