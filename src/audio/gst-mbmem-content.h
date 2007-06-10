/*
 * Copyright/Licensing information.
 */

#ifndef GST_MBMEM_CONTENT_H
#define GST_MBMEM_CONTENT_H


#include <glib-object.h>

/*
 * Type macros.
 */
#define GST_TYPE_MBMEM_CONTENT           (gst_mbmem_content_get_type ())
#define GST_MBMEM_CONTENT(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_MBMEM_CONTENT, GstMbmemContent))


typedef struct _GstMbmemContent GstMbmemContent;
typedef struct _GstMbmemContentClass GstMbmemContentClass;

struct _GstMbmemContent {
  GObject object;
  /* instance megsters */
  GstCaps * caps;
  GSList * buffers;
};

struct _GstMbmemContentClass {
  GObjectClass parent;
  /* class megsters */
  /* signals */
};

/* used by GST_MBMEM_CONTENT_TYPE */
GType gst_mbmem_content_get_type (void);

/*
 * Method definitions.
 */

void gst_mbmem_content_load_file(GstMbmemContent * content,
				 const gchar * path,
				 GError ** error);

GstCaps * gst_mbmem_content_get_caps(GstMbmemContent * content);

GstBuffer * gst_mbmem_content_get_buffer(GstMbmemContent * content, int buff );
#endif
