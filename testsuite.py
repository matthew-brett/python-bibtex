""" Run is the main function that will check if the recode and bibtex
modules are working """

def check_recode ():
    try:
        import _recode

    except SystemError:
        raise RuntimeError ('the recode library is probably broken.')

    # First, check if the recode version has the famous 3.6 bug
    rq = _recode.request ('latin1..latex')
    
    if _recode.recode (rq, 'abc') != 'abc':
        raise RuntimeError ('the _recode module is broken.')

    return


def check_bibtex ():
    import _bibtex
    
    return



def run ():
    check_recode ()
    check_bibtex ()
    return


    
