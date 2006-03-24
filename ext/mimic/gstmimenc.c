   /* 
 * GStreamer
 * Copyright (c) 2005 INdT.
 * @author Andre Moreira Magalhaes <andre.magalhaes@indt.org.br>
 * @author Philippe Khalaf <burger@speedy.org>
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
#include "config.h"
#endif

#include <gst/gst.h>

#include "gstmimenc.h"

GST_DEBUG_CATEGORY (mimenc_debug);
#define GST_CAT_DEFAULT (mimenc_debug)

#define MAX_INTERFRAMES 15

static GstStaticPadTemplate sink_factory =
GST_STATIC_PAD_TEMPLATE (
    "sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (
      "video/x-raw-rgb, "
        "bpp = (int) 24, "
        "depth = (int) 24, "
        "endianness = (int) 4321, "
        "framerate = (double) [1.0, 30.0], "
        "red_mask = (int) 16711680, "
        "green_mask = (int) 65280, "
        "blue_mask = (int) 255, "
        "width = (int) 320, "
        "height = (int) 240"
      ";video/x-raw-rgb, "
        "bpp = (int) 24, "
        "depth = (int) 24, "
        "endianness = (int) 4321, "
        "framerate = (double) [1.0, 30.0], "
        "red_mask = (int) 16711680, "
        "green_mask = (int) 65280, "
        "blue_mask = (int) 255, "
        "width = (int) 160, "
        "height = (int) 120"
    )
);

static GstStaticPadTemplate src_factory =
GST_STATIC_PAD_TEMPLATE (
  "src",
  GST_PAD_SRC,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS ("video/x-msnwebcam") 
);

/* props */
// FIXME remove or fix 
/*
enum
{
  ARG_0,
  ARG_RESOLUTION
};
*/

static void          gst_mimenc_class_init	      (GstMimEncClass *klass);
static void          gst_mimenc_base_init	      (GstMimEncClass *klass);
static void          gst_mimenc_init              (GstMimEnc      *mimenc);

// FIXME remove or fix 
/*
static void          gst_mimenc_set_property      (GObject        *object, 
                                                   guint           prop_id,
                                                   const GValue   *value, 
                                                   GParamSpec     *spec);
static void          gst_mimenc_get_property      (GObject        *object, 
                                                   guint           prop_id,
                                                   GValue         *value, 
                                                   GParamSpec     *spec);
*/

static gboolean      gst_mimenc_setcaps           (GstPad         *pad, 
                                                   GstCaps        *caps);
static GstFlowReturn gst_mimenc_chain             (GstPad         *pad, 
                                                   GstBuffer      *in);
static GstBuffer*    gst_mimenc_create_tcp_header (GstMimEnc      *mimenc, 
                                                   gint            payload_size);

#if (GST_VERSION_MAJOR == 0) && (GST_VERSION_MINOR == 9) && (GST_VERSION_MICRO <= 1)
static GstElementStateReturn
                     gst_mimenc_change_state      (GstElement     *element);
#else
static GstStateChangeReturn
                     gst_mimenc_change_state      (GstElement     *element, 
                                                  GstStateChange   transition);
#endif

static GstElementClass *parent_class = NULL;

GType
gst_gst_mimenc_get_type (void)
{
  static GType plugin_type = 0;

  if (!plugin_type)
  {
    static const GTypeInfo plugin_info =
    {
      sizeof (GstMimEncClass),
      (GBaseInitFunc) gst_mimenc_base_init,
      NULL,
      (GClassInitFunc) gst_mimenc_class_init,
      NULL,
      NULL,
      sizeof (GstMimEnc),
      0,
      (GInstanceInitFunc) gst_mimenc_init,
    };
    plugin_type = g_type_register_static (GST_TYPE_ELEMENT,
   	                                      "GstMimEnc",
                                          &plugin_info, 0);
  }
  return plugin_type;
}

static void
gst_mimenc_base_init (GstMimEncClass *klass)
{
  static GstElementDetails plugin_details = {
    "MimEnc",
    "Codec/Encoder/Video",
    "Mimic encoder",
    "Andre Moreira Magalhaes <andre.magalhaes@indt.org.br>"
  };
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
	gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (element_class,
	gst_static_pad_template_get (&sink_factory));
  gst_element_class_set_details (element_class, &plugin_details);
}

static void
gst_mimenc_class_init (GstMimEncClass *klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass*) klass;
  gstelement_class = (GstElementClass*) klass;
  gstelement_class->change_state = gst_mimenc_change_state;

/*
  gobject_class->set_property = gst_mimenc_set_property;
  gobject_class->get_property = gst_mimenc_get_property;
  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_RESOLUTION,
          g_param_spec_uint ("resolution", "Input resolution", 
             "The input resolution (1 = 320x240 default) (2 = 160x120)",
             0, G_MAXUINT16, 0, G_PARAM_READWRITE));
*/
  parent_class = g_type_class_ref (GST_TYPE_ELEMENT);

  GST_DEBUG_CATEGORY_INIT (mimenc_debug, "mimenc", 0, "Mimic encoder plugin");
}

static void
gst_mimenc_init (GstMimEnc *mimenc)
{
  mimenc->sinkpad = gst_pad_new_from_template (
	gst_static_pad_template_get (&sink_factory), "sink");
  gst_element_add_pad (GST_ELEMENT (mimenc), mimenc->sinkpad);
  gst_pad_set_setcaps_function (mimenc->sinkpad, gst_mimenc_setcaps);
  gst_pad_set_chain_function (mimenc->sinkpad, gst_mimenc_chain);

  mimenc->srcpad = gst_pad_new_from_template (
	gst_static_pad_template_get (&src_factory), "src");
  gst_element_add_pad (GST_ELEMENT (mimenc), mimenc->srcpad);

  mimenc->enc = NULL;

  // TODO property to set resolution
  mimenc->res = MIMIC_RES_HIGH;
  mimenc->buffer_size = -1;
  mimenc->width = 0;
  mimenc->height = 0;
  mimenc->frames = 0;
}

static gboolean
gst_mimenc_setcaps (GstPad *pad, GstCaps *caps)
{
  GstMimEnc *filter;
  GstStructure *structure;
  int ret, height, width;

  filter = GST_MIMENC (gst_pad_get_parent (pad));
  g_return_val_if_fail (filter != NULL, FALSE);
  g_return_val_if_fail (GST_IS_MIMENC (filter), FALSE);

  structure = gst_caps_get_structure( caps, 0 );
  ret = gst_structure_get_int( structure, "width", &width );
  ret = gst_structure_get_int( structure, "height", &height );
  filter->width = (guint16)width;
  filter->height = (guint16)height;
  filter->res = (width == 320) ? MIMIC_RES_HIGH : MIMIC_RES_LOW;
  GST_DEBUG ("Got info from caps w : %d, h : %d", filter->width, filter->height);
  if (!ret) {
      gst_object_unref(filter);
      return FALSE;
  }
  gst_object_unref(filter);
  return TRUE;
}

static GstFlowReturn
gst_mimenc_chain (GstPad *pad, GstBuffer *in)
{
  GstMimEnc *mimenc;
  GstBuffer *out_buf, *buf;
  guchar *data;
  gint buffer_size;
  GstBuffer * header = NULL;

  g_return_val_if_fail (GST_IS_PAD (pad), GST_FLOW_ERROR);
  mimenc = GST_MIMENC (GST_OBJECT_PARENT (pad));

  g_return_val_if_fail (GST_IS_MIMENC (mimenc), GST_FLOW_ERROR);
  g_return_val_if_fail (GST_PAD_IS_LINKED (mimenc->srcpad), GST_FLOW_ERROR);

  if (mimenc->enc == NULL) {
    mimenc->enc = mimic_open ();
    if (mimenc->enc == NULL) {
      GST_WARNING ("mimic_open error\n");
      return GST_FLOW_ERROR;
    }
    
    if (!mimic_encoder_init (mimenc->enc, mimenc->res)) {
      GST_WARNING ("mimic_encoder_init error\n");
      mimic_close (mimenc->enc);
      mimenc->enc = NULL;
      return GST_FLOW_ERROR;
    }
    
    if (!mimic_get_property (mimenc->enc, "buffer_size", &mimenc->buffer_size)) {
      GST_WARNING ("mimic_get_property('buffer_size') error\n");
      mimic_close (mimenc->enc);
      mimenc->enc = NULL;
      return GST_FLOW_ERROR;
    }
  }

  buf = in;
  data = GST_BUFFER_DATA (buf);

  out_buf = gst_buffer_new_and_alloc (mimenc->buffer_size);
  GST_BUFFER_TIMESTAMP(out_buf) = GST_BUFFER_TIMESTAMP(buf);
  buffer_size = mimenc->buffer_size;
  if (!mimic_encode_frame (mimenc->enc, data, GST_BUFFER_DATA (out_buf), 
      &buffer_size, ((mimenc->frames % MAX_INTERFRAMES) == 0 ? TRUE : FALSE))) {
    GST_WARNING ("mimic_encode_frame error\n");
    gst_buffer_unref (out_buf);
    gst_buffer_unref (buf);
    return GST_FLOW_ERROR;
  }
  GST_BUFFER_SIZE (out_buf) = buffer_size;

  GST_DEBUG ("incoming buf size %d, encoded size %d", GST_BUFFER_SIZE(buf), GST_BUFFER_SIZE(out_buf));
  ++mimenc->frames;

  // now let's create that tcp header
  header = gst_mimenc_create_tcp_header (mimenc, buffer_size);

  if (header)
  {
      gst_pad_push (mimenc->srcpad, header);
      gst_pad_push (mimenc->srcpad, out_buf);
  }
  else
  {
      GST_DEBUG("header not created succesfully");
      return GST_FLOW_ERROR;
  }

  gst_buffer_unref (buf);

  return GST_FLOW_OK;
}

static GstBuffer* 
gst_mimenc_create_tcp_header (GstMimEnc *mimenc, gint payload_size)
{
    // 24 bytes
    GstBuffer *buf_header = gst_buffer_new_and_alloc (24);
    guchar *p = (guchar *) GST_BUFFER_DATA(buf_header);

    p[0] = 24;
    *((guchar *) (p + 1)) = 0;
    *((guint16 *) (p + 2)) = GUINT16_TO_LE(mimenc->width);
    *((guint16 *) (p + 4)) = GUINT16_TO_LE(mimenc->height);
    *((guint16 *) (p + 6)) = 0;
    *((guint32 *) (p + 8)) = GUINT32_TO_LE(payload_size); 
    *((guint32 *) (p + 12)) = GUINT32_TO_LE(GST_MAKE_FOURCC ('M', 'L', '2', '0')); 
    *((guint32 *) (p + 16)) = 0; 
    *((guint32 *) (p + 20)) = 0; 

    return buf_header;
}

#if (GST_VERSION_MAJOR == 0) && (GST_VERSION_MINOR == 9) && (GST_VERSION_MICRO <= 1)
static GstElementStateReturn
gst_mimenc_change_state (GstElement * element)
{
  GstMimEnc *mimenc;

  switch (GST_STATE_TRANSITION (element)) {
    case GST_STATE_READY_TO_NULL:
#else
static GstStateChangeReturn
gst_mimenc_change_state (GstElement * element, GstStateChange transition)
{
  GstMimEnc *mimenc;

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_NULL:
#endif
      mimenc = GST_MIMENC (element);
      if (mimenc->enc != NULL) {
        mimic_close (mimenc->enc);
        mimenc->enc = NULL;
        mimenc->buffer_size = -1;
        mimenc->frames = 0;
      }
      break;

    default:
      break;
  }

#if (GST_VERSION_MAJOR == 0) && (GST_VERSION_MINOR == 9) && (GST_VERSION_MICRO <= 1)
  return GST_ELEMENT_CLASS (parent_class)->change_state (element);
#else
  return GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
#endif
}

// FIXME remove or fix 
/*
static void
gst_mimenc_set_property (GObject      *object, 
                         guint         prop_id,
                         const GValue *value, 
                         GParamSpec   *pspec)
{
  GstMimEnc *mimenc;
  guint res;

  g_return_if_fail (GST_IS_MIMENC (object));

  mimenc = GST_MIMENC (object);

  switch (prop_id) {
    case ARG_RESOLUTION:
      res = g_value_get_uint(value);
      if (res == 1)
          mimenc->res = MIMIC_RES_HIGH;
      else
          mimenc->res = MIMIC_RES_LOW;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_mimenc_get_property (GObject    *object, 
                         guint       prop_id, 
                         GValue     *value,
                         GParamSpec *pspec)
{
  GstMimEnc *mimenc;

  g_return_if_fail (GST_IS_MIMENC (object));

  mimenc = GST_MIMENC (object);

  switch (prop_id) {
    case ARG_RESOLUTION:
      if (mimenc->res == MIMIC_RES_HIGH)
        g_value_set_uint (value, 1);
      else
        g_value_set_uint (value, 2);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
*/