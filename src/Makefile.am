lib_LTLIBRARIES = libcamsrcjpegenc-sw.la

includelibcamsrcjpegenc_swdir = $(includedir)/camsrcjpegenc

libcamsrcjpegenc_sw_la_SOURCES = camsrcjpegenc-sw-libjpeg.c

libcamsrcjpegenc_sw_la_CFLAGS = -I$(srcdir)/include \
                                $(GLIB_CFLAGS) \
                                $(CAMSRCJPEGENC_CFLAGS) \
                                $(MMUTIL_IMGP_CFLAGS)

libcamsrcjpegenc_sw_la_LIBADD = $(GLIB_LIBS) \
                                $(LIBJPEG_LIBS) \
                                $(CAMSRCJPEGENC_LIBS) \
                                $(MMUTIL_IMGP_LIBS)

if USE_DLOG
libcamsrcjpegenc_sw_la_CFLAGS += $(DLOG_CFLAGS) -DUSE_DLOG

libcamsrcjpegenc_sw_la_LIBADD += $(DLOG_LIBS)
endif

