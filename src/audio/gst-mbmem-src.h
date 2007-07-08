/* GstMbemSrc : audio src mem  plugin for GStreamer
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GST_MBMEM_SRC_H__
#define __GST_MBMEM_SRC_H__

#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>

#include "gst-mbmem-content.h"
G_BEGIN_DECLS


#define GST_TYPE_MBMEM_SRC \
  (gst_mbmem_src_get_type())
#define GST_MBMEM_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MBMEM_SRC,GstMbmemSrc))
#define GST_MBMEM_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MBMEM_SRC,GstMbmemSrcClass))
#define GST_IS_MBMEM_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MBMEM_SRC))
#define GST_IS_MBMEM_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MBMEM_SRC))

typedef struct _GstMbmemSrc GstMbmemSrc;
typedef struct _GstMbmemSrcClass GstMbmemSrcClass;

/**
 * GstMbmemSrc:
 *
 * Opaque #GstMbmemSrc data structure.
 */
struct _GstMbmemSrc {
  GstBaseSrc     element;

  /*< private >*/
  GstMbmemContent * content;

  guint64        bytes_sent;
  guint         buffer_count;
  guint         pos;
  gchar		*last_message;
};

struct _GstMbmemSrcClass {
  GstBaseSrcClass parent_class;

  /*< public >*/
  /* signals */
};

GType gst_mbmem_src_get_type (void);


void gst_mbmem_src_set_content(GstMbmemSrc * src,GstMbmemContent * c);
G_END_DECLS

#endif /* __GST_MBMEM_SRC_H__ */

