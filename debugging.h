#ifndef __debugging_h__
#define __debugging_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>

static gchar *
__my_strdup (gchar * text, gchar * file, gint line) {
    gchar * tmp = g_strdup (text);
    fprintf(stderr,"g_strdup: %x %s %d\n", tmp, file, line);
    return tmp;
}

static void
__my_free (gpointer text, gchar *file, gint line) {
    fprintf(stderr,"g_free: %x %s %d\n", text, file, line);
    g_free (text);
}

#define g_strdup(text) __my_strdup (text,__FILE__,__LINE__)
#define g_free(text) __my_free(text,__FILE__,__LINE__)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* debugging.h */
