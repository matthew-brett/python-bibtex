#ifndef __logging_h__
#define __logging_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    
#define BIB_LEVEL_ERROR   (1 << (G_LOG_LEVEL_USER_SHIFT + 0))
#define BIB_LEVEL_WARNING (1 << (G_LOG_LEVEL_USER_SHIFT + 1))
#define BIB_LEVEL_MESSAGE (1 << (G_LOG_LEVEL_USER_SHIFT + 2))
    
#ifdef  __GNUC__
#define bibtex_error(format, args...)        g_log (G_LOG_DOMAIN, \
                                               BIB_LEVEL_ERROR, \
                                               format, ##args)
#define bibtex_message(format, args...)      g_log (G_LOG_DOMAIN, \
                                               BIB_LEVEL_MESSAGE, \
                                               format, ##args)
#define bibtex_warning(format, args...)      g_log (G_LOG_DOMAIN, \
                                               BIB_LEVEL_WARNING, \
                                               format, ##args)
#else   /* !__GNUC__ */

    static inline void
    bibtex_error (const gchar *format,
		  ...)
    {
	va_list args;
	va_start (args, format);
	g_logv (G_LOG_DOMAIN, BIB_LEVEL_ERROR, format, args);
	va_end (args);
    }

    static inline void
    bibtex_message (const gchar *format,
		    ...)
    {
	va_list args;
	va_start (args, format);
	g_logv (G_LOG_DOMAIN, BIB_LEVEL_MESSAGE, format, args);
	va_end (args);
    }

    static inline void
    bibtex_warning (const gchar *format,
		    ...)
    {
	va_list args;
	va_start (args, format);
	g_logv (G_LOG_DOMAIN, BIB_LEVEL_WARNING, format, args);
	va_end (args);
    }
#endif /* !GNUC */

    void bibtex_set_default_handler (void);
    
    void bibtex_message_handler (const gchar *log_domain,
				 GLogLevelFlags log_level,
				 const gchar *message,
				 gpointer user_data);


    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* logging.h */
