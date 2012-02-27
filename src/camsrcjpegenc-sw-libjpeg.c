/*
 * libcamsrcjpegenc-sw-libjpeg
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jeongmo Yang <jm80.yang@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

/* ==============================================================================
|  INCLUDE FILES								|
===============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <jpeglib.h>

#include <camsrcjpegenc_sub.h>
#include <mm_util_imgp.h>

/*-------------------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS for internal					|
-------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------
|    LOCAL VARIABLE DEFINITIONS for internal					|
-------------------------------------------------------------------------------*/
/*
 * Declaration for jpeg codec
 */
#define JPEG_MODULE_NAME        "LIBJPEGSW"     /* A module name. Printing this name, a user of this submodule knows what module he used. */

/* I420 */
#define JPEGENC_ROUND_UP_2(num)  (((num)+1)&~1)
#define JPEGENC_ROUND_UP_4(num)  (((num)+3)&~3)
#define JPEGENC_ROUND_UP_8(num)  (((num)+7)&~7)

#define I420_Y_ROWSTRIDE(width) (JPEGENC_ROUND_UP_4(width))
#define I420_U_ROWSTRIDE(width) (JPEGENC_ROUND_UP_8(width)/2)
#define I420_V_ROWSTRIDE(width) ((JPEGENC_ROUND_UP_8(I420_Y_ROWSTRIDE(width)))/2)

#define I420_Y_OFFSET(w,h) (0)
#define I420_U_OFFSET(w,h) (I420_Y_OFFSET(w,h)+(I420_Y_ROWSTRIDE(w)*JPEGENC_ROUND_UP_2(h)))
#define I420_V_OFFSET(w,h) (I420_U_OFFSET(w,h)+(I420_U_ROWSTRIDE(w)*JPEGENC_ROUND_UP_2(h)/2))

#define I420_SIZE(w,h)     (I420_V_OFFSET(w,h)+(I420_V_ROWSTRIDE(w)*JPEGENC_ROUND_UP_2(h)/2))

/* Limitations */
#define MAX_JPG_WIDTH               2560
#define MAX_JPG_HEIGHT              1920
#define MAX_YUV_SIZE                (MAX_JPG_WIDTH * MAX_JPG_HEIGHT * 3)
#define MAX_ENCODED_RESULT_SIZE               (MAX_JPG_WIDTH * MAX_JPG_HEIGHT / 3)


/* To use dlog instead of printf */
#ifdef USE_DLOG
#include <dlog.h>
#endif

enum {
    JPEGENC_LIBJPEGSW_LOG = 0,
    JPEGENC_LIBJPEGSW_INFO,
    JPEGENC_LIBJPEGSW_WARNING,
    JPEGENC_LIBJPEGSW_ERROR,
    JPEGENC_LIBJPEGSW_NUM,
};

enum {
    JPEGENC_LIBJPEGSW_COLOR_LOG = 0,
    JPEGENC_LIBJPEGSW_COLOR_INFO = 32,
    JPEGENC_LIBJPEGSW_COLOR_WARNING = 36,
    JPEGENC_LIBJPEGSW_COLOR_ERROR = 31,
    JPEGENC_LIBJPEGSW_COLOR_NUM = 4,
};


#if defined (USE_DLOG)

#define ___jpegenc_libjpegsw_log(class, msg, args...)   SLOG(class, "CAMSRCJPEGENC", msg, ##args)

#define _jpegenc_libjpegsw_log(msg, args...) {\
    ___jpegenc_libjpegsw_log(LOG_DEBUG, "[LOG] [%s:%d] [%s] "msg, __FILE__, __LINE__, __func__, ##args);\
}

#define _jpegenc_libjpegsw_info(msg, args...) {\
    ___jpegenc_libjpegsw_log(LOG_DEBUG, "[INFO] [%s:%d] [%s] "msg, __FILE__, __LINE__, __func__, ##args);\
}

#define _jpegenc_libjpegsw_warning(msg, args...) {\
    ___jpegenc_libjpegsw_log(LOG_WARN, "[WARNING] [%s:%d] [%s] "msg, __FILE__, __LINE__, __func__, ##args);\
}

#define _jpegenc_libjpegsw_error(msg, args...) {\
    ___jpegenc_libjpegsw_log(LOG_ERROR, "[ERROR] [%s:%d] [%s] "msg, __FILE__, __LINE__, __func__, ##args);\
}

#else /* defined (USE_DLOG) */

void _jpegenc_libjpegsw_print(int log_type, char *msg, ...);

#define JPEGENC_LIBJPEGSW_LOG_TMP_BUF_SIZE 256

#define _jpegenc_libjpegsw_log(msg, args...) {\
    char tmp_buf[JPEGENC_LIBJPEGSW_LOG_TMP_BUF_SIZE];\
    snprintf(tmp_buf, JPEGENC_LIBJPEGSW_LOG_TMP_BUF_SIZE-1, "[LOG] [%s:%d] [%s] %s", __FILE__, __LINE__, __func__, (msg));\
    _jpegenc_libjpegsw_print(JPEGENC_LIBJPEGSW_LOG, (tmp_buf), ##args);\
}

#define _jpegenc_libjpegsw_info(msg, args...) {\
    char tmp_buf[JPEGENC_LIBJPEGSW_LOG_TMP_BUF_SIZE];\
    snprintf(tmp_buf, JPEGENC_LIBJPEGSW_LOG_TMP_BUF_SIZE-1, "[INFO] [%s:%d] [%s] %s", __FILE__, __LINE__, __func__, (msg));\
    _jpegenc_libjpegsw_print(JPEGENC_LIBJPEGSW_INFO, (tmp_buf), ##args);\
}

#define _jpegenc_libjpegsw_warning(msg, args...) {\
    char tmp_buf[JPEGENC_LIBJPEGSW_LOG_TMP_BUF_SIZE];\
    snprintf(tmp_buf, JPEGENC_LIBJPEGSW_LOG_TMP_BUF_SIZE-1, "[WARNING] [%s:%d] [%s] %s", __FILE__, __LINE__, __func__, (msg));\
    _jpegenc_libjpegsw_print(JPEGENC_LIBJPEGSW_WARNING, (tmp_buf), ##args);\
}

#define _jpegenc_libjpegsw_error(msg, args...) {\
    char tmp_buf[JPEGENC_LIBJPEGSW_LOG_TMP_BUF_SIZE];\
    snprintf(tmp_buf, JPEGENC_LIBJPEGSW_LOG_TMP_BUF_SIZE-1, "[ERROR] [%s:%d] [%s] %s", __FILE__, __LINE__, __func__, (msg));\
    _jpegenc_libjpegsw_print(JPEGENC_LIBJPEGSW_ERROR, (tmp_buf), ##args);\
}

#endif /* defined (USE_DLOG) */


/*-------------------------------------------------------------------------------
|    LOCAL FUNCTION PROTOTYPES:							|
-------------------------------------------------------------------------------*/
/* STATIC INTERNAL FUNCTION */
static void _jpegenc_libjpegsw_error_handle(j_common_ptr cinfo);
static void _jpegenc_init_destination (j_compress_ptr cinfo);
static boolean _jpegenc_flush_destination (j_compress_ptr cinfo);
static void _jpegenc_term_destination (j_compress_ptr cinfo);
static gboolean _jpegenc_convert_YUV_to_RGB888( unsigned char *src, int src_fmt, guint width, guint height, unsigned char **dst, unsigned int* dst_len );

static void _jpegenc_libjpegsw_print(int log_type, char *msg, ...);


/*===============================================================================
|  FUNCTION DEFINITIONS								|
===============================================================================*/
/*-------------------------------------------------------------------------------
|    GLOBAL FUNCTION DEFINITIONS:						|
-------------------------------------------------------------------------------*/
/*-----------------------------------------------
|		CAMSRCJPEGENC SUBMODULE		|
-----------------------------------------------*/
gboolean camsrcjpegencsub_get_info ( jpegenc_internal_info *info )
{
	gboolean bret = FALSE;
	_jpegenc_libjpegsw_info("%s [SUB:%s]\n",__func__, JPEG_MODULE_NAME);

	if (!info)
	{
		_jpegenc_libjpegsw_error("Null pointer[info].\n");
		goto exit;
	}

	/* input variables */
	info->version = 1;                              /* Interface version that current submodule follows */
	info->mem_addr_type = MEMORY_ADDRESS_VIRTUAL;   /* Memory type of input buffer */
	info->input_fmt_list[0] = COLOR_FORMAT_I420;    /* A color format list that jpeg encoder can retreive */
	info->input_fmt_list[1] = COLOR_FORMAT_YUYV;
	info->input_fmt_list[2] = COLOR_FORMAT_UYVY;
	info->input_fmt_list[3] = COLOR_FORMAT_NV12;
	info->input_fmt_list[4] = COLOR_FORMAT_RGB;
	info->input_fmt_num = 5;                        /* Total number of color format that the encoder supports */
	info->input_fmt_recommend = COLOR_FORMAT_I420;  /* Recommended color format */
	info->progressive_mode_support = TRUE;          /* Whether the encoder supports progressive encoding */

	bret = TRUE;
exit:
	_jpegenc_libjpegsw_info("%s leave\n",__func__);
	return bret;
}


int  camsrcjpegencsub_encode ( jpegenc_parameter *enc_param )
{
	/* video state */
	unsigned int width, height;
	int bufsize = 0;

	/* the jpeg line buffer */
	guchar **line[3];
	guchar *base[3], *end[3];
	gint i, j, k;

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	struct jpeg_destination_mgr jdest;

	/* properties */
	//gint jpeg_mode;

	/* general */
	gboolean bret = FALSE;
	int src_fmt = COLOR_FORMAT_NOT_SUPPORT;
	unsigned int src_len = 0;
	unsigned char *src_data = NULL;

	/* Check supported color format and do converting if needed */
	if( enc_param->src_fmt != COLOR_FORMAT_I420 && enc_param->src_fmt != COLOR_FORMAT_RGB )
	{
		_jpegenc_libjpegsw_error( "Original source length : %d\n", enc_param->src_len );

		if (enc_param->src_fmt == COLOR_FORMAT_YUYV ||
		    enc_param->src_fmt == COLOR_FORMAT_UYVY ||
		    enc_param->src_fmt == COLOR_FORMAT_NV12) {
			bret = _jpegenc_convert_YUV_to_RGB888( enc_param->src_data, enc_param->src_fmt,
			                                              enc_param->width, enc_param->height,
			                                              &src_data, &src_len );
			if( bret == FALSE )
			{
				enc_param->result_data = NULL;
				if( src_data )
				{
					free( src_data );
					src_data = NULL;
				}

				return FALSE;
			}
		}
		else
		{
			_jpegenc_libjpegsw_error( "NOT Supported format [%d]\n", enc_param->src_fmt );

			return FALSE;
		}

		src_fmt = COLOR_FORMAT_RGB;
	}
	else
	{
		_jpegenc_libjpegsw_info( "Do JPEG encode without color converting...\n" );

		src_fmt = enc_param->src_fmt;
		src_data = enc_param->src_data;
	}

	_jpegenc_libjpegsw_info("%s [SUB:%s]\n",__func__, JPEG_MODULE_NAME);
	memset (&cinfo, 0, sizeof (struct jpeg_compress_struct));
	memset (&jerr, 0, sizeof (struct jpeg_error_mgr));
	memset (&jdest, 0, sizeof (struct jpeg_destination_mgr));

	jerr.reset_error_mgr = _jpegenc_libjpegsw_error_handle;
	cinfo.err = jpeg_std_error (&jerr);
	jpeg_create_compress (&cinfo);

	jdest.init_destination = _jpegenc_init_destination;
	jdest.empty_output_buffer = _jpegenc_flush_destination;
	jdest.term_destination = _jpegenc_term_destination;
	cinfo.dest = &jdest;

	cinfo.image_width = width = enc_param->width;
	cinfo.image_height = height = enc_param->height;
	cinfo.input_components = 3;

	_jpegenc_libjpegsw_info ("width %d, height %d, format %d", width, height, src_fmt);

	switch (src_fmt) {
		case COLOR_FORMAT_I420:

			bufsize = I420_SIZE (width, height);
			cinfo.in_color_space = JCS_YCbCr;

			jpeg_set_defaults (&cinfo);

			cinfo.raw_data_in = TRUE;
			cinfo.do_fancy_downsampling = FALSE;

			if (height != -1) {
				line[0] = malloc ( height * sizeof (char *));
				line[1] = malloc ( height * sizeof (char *) / 2);
				line[2] = malloc ( height * sizeof (char *) / 2);
			}

			_jpegenc_libjpegsw_info ("setting format to I420");

		break;
		case COLOR_FORMAT_RGB:
		default:
			/* Wrong value. So set RGB for default. */
			bufsize = width * height * 3;
			_jpegenc_libjpegsw_info ("setting format to RGB24");
			cinfo.in_color_space = JCS_RGB;

			jpeg_set_defaults (&cinfo);

			cinfo.raw_data_in = FALSE;
		break;
	}

	jpeg_suppress_tables (&cinfo, TRUE);

	/* Set remaining settings. */
	if (enc_param->jpeg_mode == JPEG_MODE_PROGRESSIVE)
		cinfo.progressive_mode = TRUE;
	else
		cinfo.progressive_mode = FALSE;
	cinfo.smoothing_factor = 0;
	cinfo.dct_method = JDCT_FASTEST;

	jpeg_set_quality (&cinfo, enc_param->jpeg_quality, TRUE);

	_jpegenc_libjpegsw_info ("Set quality");

	enc_param->result_data = malloc(MAX_ENCODED_RESULT_SIZE);
	jdest.next_output_byte = enc_param->result_data;
	jdest.free_in_buffer = MAX_ENCODED_RESULT_SIZE;

	_jpegenc_libjpegsw_info ("compress start(%p, %d, %p, %d)", src_data, src_len, jdest.next_output_byte, jdest.free_in_buffer);
	jpeg_start_compress (&cinfo, TRUE);

	_jpegenc_libjpegsw_info ("got buffer of %lu bytes", src_len);

	switch (src_fmt) {
		case COLOR_FORMAT_I420:
		{
			base[0] = src_data + I420_Y_OFFSET (width, height);
			base[1] = src_data + I420_U_OFFSET (width, height);
			base[2] = src_data + I420_V_OFFSET (width, height);

			end[0] = base[0] + height * I420_Y_ROWSTRIDE (width);
			end[1] = base[1] + (height / 2) * I420_U_ROWSTRIDE (width);
			end[2] = base[2] + (height / 2) * I420_V_ROWSTRIDE (width);


			/* prepare for raw input */
			_jpegenc_libjpegsw_info ("compressing I420");

			for (i = 0; i < height; i += 2 * DCTSIZE) {
				for (j = 0, k = 0; j < (2 * DCTSIZE); j += 2, k++) {
					line[0][j] = base[0];
					if (base[0] + I420_Y_ROWSTRIDE (width) < end[0])
						base[0] += I420_Y_ROWSTRIDE (width);
					line[0][j + 1] = base[0];
					if (base[0] + I420_Y_ROWSTRIDE (width) < end[0])
						base[0] += I420_Y_ROWSTRIDE (width);
					line[1][k] = base[1];
					if (base[1] + I420_U_ROWSTRIDE (width) < end[1])
						base[1] += I420_U_ROWSTRIDE (width);
					line[2][k] = base[2];
					if (base[2] + I420_V_ROWSTRIDE (width) < end[2])
						base[2] += I420_V_ROWSTRIDE (width);
				}
				jpeg_write_raw_data (&cinfo, line, 2 * DCTSIZE);
			}
		}
		break;
		case COLOR_FORMAT_RGB:
		default:
		{
			/* Wrong value. So set RGB for default. */
			JSAMPROW *jbuf = NULL;
			unsigned char *buf = src_data;

			_jpegenc_libjpegsw_info ("compressing RGB");

			while (cinfo.next_scanline < cinfo.image_height)
			{
				jbuf = (JSAMPROW *) (&buf);
				jpeg_write_scanlines(&cinfo, (JSAMPROW *)jbuf, 1);
				buf += width * 3;
			}
		}
		break;
	}

	_jpegenc_libjpegsw_info ("compressing DONE");

	enc_param->result_len = JPEGENC_ROUND_UP_4 (MAX_ENCODED_RESULT_SIZE - jdest.free_in_buffer);
	
	jpeg_finish_compress (&cinfo);
	jpeg_destroy_compress(&cinfo);

	if( enc_param->src_fmt != COLOR_FORMAT_I420 && enc_param->src_fmt != COLOR_FORMAT_RGB )
	{
		if( src_data != NULL )
		{
			free( src_data );
			src_data = NULL;
		}
	}

	_jpegenc_libjpegsw_info ("compressing done");

	bret = TRUE;

	return bret;
}


/*-------------------------------------------------------------------------------
|    LOCAL FUNCTION DEFINITIONS:						|
-------------------------------------------------------------------------------*/
/* Core functions */
static void
_jpegenc_libjpegsw_error_handle(j_common_ptr cinfo)
{
   _jpegenc_libjpegsw_error("Libjpeg error!!!\n");

   return;
}


static void
_jpegenc_init_destination (j_compress_ptr cinfo)
{
  _jpegenc_libjpegsw_info ("gst_jpegenc_chain: init_destination");
}


static boolean
_jpegenc_flush_destination (j_compress_ptr cinfo)
{
  _jpegenc_libjpegsw_info ("gst_jpegenc_chain: flush_destination: buffer too small !!!");
  return TRUE;
}


static void
_jpegenc_term_destination (j_compress_ptr cinfo)
{
  _jpegenc_libjpegsw_info ("gst_jpegenc_chain: term_source");
}


static gboolean
_jpegenc_convert_YUV_to_RGB888( unsigned char *src, int src_fmt, guint width, guint height, unsigned char **dst, unsigned int* dst_len )
{
	int ret = 0;
	int src_cs = MM_UTIL_IMG_FMT_UYVY;
	int dst_cs = MM_UTIL_IMG_FMT_RGB888;
	unsigned int dst_size = 0;

	if( src_fmt == COLOR_FORMAT_YUYV )
	{
		_jpegenc_libjpegsw_info( "Convert YUYV to RGB888\n" );
		src_cs = MM_UTIL_IMG_FMT_YUYV;
	}
	else if( src_fmt == COLOR_FORMAT_UYVY )
	{
		_jpegenc_libjpegsw_info( "Convert UYVY to RGB888\n" );
		src_cs = MM_UTIL_IMG_FMT_UYVY;
	}
	else if( src_fmt == COLOR_FORMAT_NV12 )
	{
		_jpegenc_libjpegsw_info( "Convert NV12 to RGB888\n" );
		src_cs = MM_UTIL_IMG_FMT_NV12;
	}
	else
	{
		_jpegenc_libjpegsw_error( "NOT supported format [%d]\n", src_fmt );
		return FALSE;
	}

	mm_util_get_image_size( dst_cs, width, height, &dst_size );
	*dst = malloc( dst_size );
	if( *dst == NULL )
	{
		_jpegenc_libjpegsw_error( "malloc failed\n" );
		return FALSE;
	}

	*dst_len = dst_size;
	ret = mm_util_convert_colorspace( src, width, height, src_cs, *dst, dst_cs );
	if( ret == 0 )
	{
		_jpegenc_libjpegsw_info( "Convert [dst_size:%d] OK.\n", dst_size );
		return TRUE;
	}
	else
	{
		free( *dst );
		*dst = NULL;

		_jpegenc_libjpegsw_error( "Convert [size:%d] FAILED.\n", dst_size );
		return FALSE;
	}
}

/* Functions for printf log */
#define BUF_LEN 256
static void _jpegenc_libjpegsw_print(int log_type, char *msg, ...)
{
	int log_color = 0;
	char work_buf1[BUF_LEN+1];
	va_list arg;

	va_start(arg, msg);
	vsnprintf(work_buf1, BUF_LEN-100, msg, arg);
	va_end(arg);

	switch (log_type) {
	case JPEGENC_LIBJPEGSW_LOG:
		log_color = JPEGENC_LIBJPEGSW_COLOR_LOG;
		break;
	case JPEGENC_LIBJPEGSW_INFO:
		log_color = JPEGENC_LIBJPEGSW_COLOR_INFO;
		break;
	case JPEGENC_LIBJPEGSW_WARNING:
		log_color = JPEGENC_LIBJPEGSW_COLOR_WARNING;
		break;
	case JPEGENC_LIBJPEGSW_ERROR:
		log_color = JPEGENC_LIBJPEGSW_COLOR_ERROR;
		break;
	default:
		break;
	}

	printf("\033[%dm%s\033[0m", log_color, work_buf1);

	return;
}
