/* 
 *
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


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "gst-mbmem-src.h"
#include <gst/gstmarshal.h>

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
								   GST_PAD_SRC,
								   GST_PAD_ALWAYS,
								   GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY_STATIC (gst_mbmem_src_debug);
#define GST_CAT_DEFAULT gst_mbmem_src_debug

static const GstElementDetails gst_mbmem_src_details =
  GST_ELEMENT_DETAILS ("Mbmem Source",
		       "Source",
		       "Push cached buffers around",
		       " ");


#define _do_init(bla) \
  GST_DEBUG_CATEGORY_INIT (gst_mbmem_src_debug, "mbmemsrc", 0, "mbmemsrc element");

GST_BOILERPLATE_FULL (GstMbmemSrc, gst_mbmem_src, GstBaseSrc, GST_TYPE_BASE_SRC,
		      _do_init);

static void gst_mbmem_src_finalize (GObject * object);

static gboolean gst_mbmem_src_is_seekable (GstBaseSrc * basesrc);
static GstFlowReturn gst_mbmem_src_create (GstBaseSrc * src, guint64 offset,
					   guint length, GstBuffer ** buf);


static void
gst_mbmem_src_base_init (gpointer g_class)
{
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (g_class);
  
  gst_element_class_add_pad_template (gstelement_class,
				      gst_static_pad_template_get (&srctemplate));
  
  gst_element_class_set_details (gstelement_class, &gst_mbmem_src_details);
  
}


static void
gst_mbmem_src_class_init (GstMbmemSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSrcClass *gstbase_src_class;
  
  gobject_class = G_OBJECT_CLASS (klass);
  gstelement_class = GST_ELEMENT_CLASS (klass);
  gstbase_src_class = GST_BASE_SRC_CLASS (klass);

  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_mbmem_src_finalize);
  
  
  gstbase_src_class->is_seekable = GST_DEBUG_FUNCPTR (gst_mbmem_src_is_seekable);
  gstbase_src_class->create = GST_DEBUG_FUNCPTR (gst_mbmem_src_create);

}

static void
gst_mbmem_src_src_fixate (GstPad * pad, GstCaps * caps)
{
  //  GstMbmemSrc *src = GST_MBMEM_SRC (GST_PAD_PARENT (pad));

  GstStructure *structure;
  structure = gst_caps_get_structure (caps, 0);

  gst_structure_remove_field( structure, "signed");
}


void gst_mbmem_src_set_content(GstMbmemSrc * src,GstMbmemContent * c)
{

  src->content = c;
  GstPad *pad = GST_BASE_SRC_PAD (src);
  gst_pad_use_fixed_caps( pad);
  gst_pad_set_caps( pad,gst_mbmem_content_get_caps( src->content));
  gst_pad_set_fixatecaps_function (pad, gst_mbmem_src_src_fixate);
}

static void
gst_mbmem_src_init (GstMbmemSrc * src, GstMbmemSrcClass * g_class)
{
  GST_BASE_SRC (src)->can_activate_push = TRUE;


  gst_base_src_set_format (GST_BASE_SRC (src), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (src), FALSE);

}

static void
gst_mbmem_src_finalize (GObject * object)
{
  GstMbmemSrc *src;

  src = GST_MBMEM_SRC (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GstFlowReturn
gst_mbmem_src_create (GstBaseSrc * basesrc, guint64 offset, guint length,
		      GstBuffer ** ret)
{
  GstMbmemSrc *src;
  GstBuffer *buf,*buf2;

  GstFlowReturn res;
  src = GST_MBMEM_SRC (basesrc);
  
  /* allocate a new buffer suitable for this pad */
  buf2 = gst_mbmem_content_get_buffer(src->content, src->pos);

  if( buf2 == NULL) { 
    return GST_FLOW_UNEXPECTED;
  }

  if ((res = gst_pad_alloc_buffer_and_set_caps (basesrc->srcpad, 
						src->pos,
						GST_BUFFER_SIZE(buf2),
						GST_PAD_CAPS (basesrc->srcpad),
						&buf)) != GST_FLOW_OK) {
    return res;
  }
  
  src->pos++;
  
  GST_BUFFER_OFFSET (buf) = src->buffer_count++;
  GST_BUFFER_DURATION(buf) = GST_BUFFER_DURATION(buf2);
  memcpy(GST_BUFFER_DATA(buf), GST_BUFFER_DATA(buf2), GST_BUFFER_SIZE(buf));

  *ret = buf;
  return GST_FLOW_OK;
}


static gboolean
gst_mbmem_src_is_seekable (GstBaseSrc * basesrc)
{

  return FALSE;
}
