/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2003 University of Southern California
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@cworth.org>
 */

#include "cairoint.h"

static const cairo_image_surface_t _cairo_image_surface_nil_invalid_format = {
    {
	&cairo_image_surface_backend,	/* backend */
	CAIRO_SURFACE_TYPE_IMAGE,
	CAIRO_CONTENT_COLOR,
	CAIRO_REF_COUNT_INVALID,	/* ref_count */
	CAIRO_STATUS_INVALID_FORMAT,	/* status */
	FALSE,				/* finished */
	{ 0,	/* size */
	  0,	/* num_elements */
	  0,	/* element_size */
	  NULL,	/* elements */
	},				/* user_data */
	{ 1.0, 0.0,
	  0.0, 1.0,
	  0.0, 0.0
	},				/* device_transform */
	{ 1.0, 0.0,
	  0.0, 1.0,
	  0.0, 0.0
	},				/* device_transform_inverse */
	0.0,				/* x_fallback_resolution */
	0.0,				/* y_fallback_resolution */
	NULL,				/* clip */
	0,				/* next_clip_serial */
	0,				/* current_clip_serial */
	FALSE,				/* is_snapshot */
	FALSE,				/* has_font_options */
	{ CAIRO_ANTIALIAS_DEFAULT,
	  CAIRO_SUBPIXEL_ORDER_DEFAULT,
	  CAIRO_HINT_STYLE_DEFAULT,
	  CAIRO_HINT_METRICS_DEFAULT
	}				/* font_options */
    },					/* base */
    CAIRO_FORMAT_ARGB32,		/* format */
    NULL,				/* data */
    FALSE,				/* owns_data */
    FALSE,				/* has_clip */
    0,					/* width */
    0,					/* height */
    0,					/* stride */
    0,					/* depth */
    NULL				/* pixman_image */
};


cairo_surface_t *
_cairo_image_surface_create_for_pixman_image (pixman_image_t *pixman_image,
					      cairo_format_t  format)
{
    cairo_image_surface_t *surface;

    surface = malloc (sizeof (cairo_image_surface_t));
    if (surface == NULL) {
	_cairo_error (CAIRO_STATUS_NO_MEMORY);
	return (cairo_surface_t*) &_cairo_surface_nil;
    }

    _cairo_surface_init (&surface->base, &cairo_image_surface_backend,
			 _cairo_content_from_format (format));

    surface->pixman_image = pixman_image;

    surface->format = format;
    surface->data = (unsigned char *) pixman_image_get_data (pixman_image);
    surface->owns_data = FALSE;
    surface->has_clip = FALSE;

    surface->width = pixman_image_get_width (pixman_image);
    surface->height = pixman_image_get_height (pixman_image);
    surface->stride = pixman_image_get_stride (pixman_image);
    surface->depth = pixman_image_get_depth (pixman_image);

    return &surface->base;
}

static cairo_bool_t
_CAIRO_MASK_FORMAT (cairo_format_masks_t *masks, cairo_format_t *format)
{
    /* XXX: many formats are simply not supported by pixman, so this function
     * converts the masks into something we know will be supported.
     */
    switch (masks->bpp) {
    case 32:
	if (masks->alpha_mask == 0xff000000 &&
	    masks->red_mask == 0x00ff0000 &&
	    masks->green_mask == 0x0000ff00 &&
	    masks->blue_mask == 0x000000ff)
	{
	    *format = CAIRO_FORMAT_ARGB32;
	    return TRUE;
	}
	if (masks->alpha_mask == 0x00000000 &&
	    masks->red_mask == 0x00ff0000 &&
	    masks->green_mask == 0x0000ff00 &&
	    masks->blue_mask == 0x000000ff)
	{
	    *format = CAIRO_FORMAT_RGB24;
	    return TRUE;
	}
	break;
    case 8:
	if (masks->alpha_mask == 0xff)
	{
	    *format = CAIRO_FORMAT_A8;
	    return TRUE;
	}
	break;
    case 1:
	if (masks->alpha_mask == 0x1)
	{
	    *format = CAIRO_FORMAT_A1;
	    return TRUE;
	}
	break;
    }
    return FALSE;
}

/* XXX: This function really should be eliminated. We don't really
 * want to advertise a cairo image surface that supports any possible
 * format. A minimal step would be to replace this function with one
 * that accepts a cairo_internal_format_t rather than mask values. */
cairo_surface_t *
_cairo_image_surface_create_with_masks (unsigned char	       *data,
					cairo_format_masks_t   *masks,
					int			width,
					int			height,
					int			stride)
{
    cairo_format_t format;
    
    if (!_CAIRO_MASK_FORMAT (masks, &format)) {
	_cairo_error (CAIRO_STATUS_INVALID_FORMAT);
	return (cairo_surface_t*) &_cairo_surface_nil;
    }

    return cairo_image_surface_create_for_data (data,
						format,
						width,
						height,
						stride);
}

static pixman_format_code_t 
_cairo_format_to_pixman_format_code (cairo_format_t format)
{
    int ret = 0;
    switch (format) {
    case CAIRO_FORMAT_A1:
	ret = PIXMAN_a1;
	break;
    case CAIRO_FORMAT_A8:
	ret = PIXMAN_a8;
	break;
    case CAIRO_FORMAT_RGB24:
	ret = PIXMAN_x8r8g8b8;
	break;
    case CAIRO_FORMAT_ARGB32:
    default:
	ret = PIXMAN_a8r8g8b8;
	break;
    }
    assert (ret);
    return ret;
}

/**
 * cairo_image_surface_create:
 * @format: format of pixels in the surface to create
 * @width: width of the surface, in pixels
 * @height: height of the surface, in pixels
 *
 * Creates an image surface of the specified format and
 * dimensions. Initially the surface contents are all
 * 0. (Specifically, within each pixel, each color or alpha channel
 * belonging to format will be 0. The contents of bits within a pixel,
 * but not belonging to the given format are undefined).
 *
 * Return value: a pointer to the newly created surface. The caller
 * owns the surface and should call cairo_surface_destroy when done
 * with it.
 *
 * This function always returns a valid pointer, but it will return a
 * pointer to a "nil" surface if an error such as out of memory
 * occurs. You can use cairo_surface_status() to check for this.
 **/
cairo_surface_t *
cairo_image_surface_create (cairo_format_t	format,
			    int			width,
			    int			height)
{
    cairo_surface_t	*surface;
    pixman_format_code_t pixman_format;
    pixman_image_t	*pixman_image;

    if (! CAIRO_FORMAT_VALID (format)) {
	_cairo_error (CAIRO_STATUS_INVALID_FORMAT);
	return (cairo_surface_t*) &_cairo_image_surface_nil_invalid_format;
    }

    pixman_format = _cairo_format_to_pixman_format_code (format);
    
    pixman_image = pixman_image_create_bits (pixman_format, width, height,
					     NULL, -1);
    if (pixman_image == NULL) {
	_cairo_error (CAIRO_STATUS_NO_MEMORY);
	return (cairo_surface_t*) &_cairo_surface_nil;
    }

    surface = _cairo_image_surface_create_for_pixman_image (pixman_image, format);
    if (cairo_surface_status (surface)) {
	pixman_image_unref (pixman_image);
    }

    return surface;
}
slim_hidden_def (cairo_image_surface_create);

cairo_surface_t *
_cairo_image_surface_create_with_content (cairo_content_t	content,
					  int			width,
					  int			height)
{
    if (! CAIRO_CONTENT_VALID (content))
	return (cairo_surface_t*) &_cairo_surface_nil;

    return cairo_image_surface_create (_cairo_format_from_content (content),
				       width, height);
}

/**
 * cairo_image_surface_create_for_data:
 * @data: a pointer to a buffer supplied by the application
 *    in which to write contents.
 * @format: the format of pixels in the buffer
 * @width: the width of the image to be stored in the buffer
 * @height: the height of the image to be stored in the buffer
 * @stride: the number of bytes between the start of rows
 *   in the buffer. Having this be specified separate from @width
 *   allows for padding at the end of rows, or for writing
 *   to a subportion of a larger image.
 *
 * Creates an image surface for the provided pixel data. The output
 * buffer must be kept around until the #cairo_surface_t is destroyed
 * or cairo_surface_finish() is called on the surface.  The initial
 * contents of @buffer will be used as the initial image contents; you
 * must explicitly clear the buffer, using, for example,
 * cairo_rectangle() and cairo_fill() if you want it cleared.
 *
 * Return value: a pointer to the newly created surface. The caller
 * owns the surface and should call cairo_surface_destroy when done
 * with it.
 *
 * This function always returns a valid pointer, but it will return a
 * pointer to a "nil" surface if an error such as out of memory
 * occurs. You can use cairo_surface_status() to check for this.
 *
 * See cairo_surface_set_user_data() for a means of attaching a
 * destroy-notification fallback to the surface if necessary.
 **/
cairo_surface_t *
cairo_image_surface_create_for_data (unsigned char     *data,
				     cairo_format_t	format,
				     int		width,
				     int		height,
				     int		stride)
{
    cairo_surface_t	*surface;
    pixman_format_code_t pixman_format;
    pixman_image_t	*pixman_image;

    if (! CAIRO_FORMAT_VALID (format))
	return (cairo_surface_t*) &_cairo_surface_nil;

    pixman_format = _cairo_format_to_pixman_format_code (format);

    pixman_image = pixman_image_create_bits (pixman_format, width, height,
					     (uint32_t *) data, stride);
    
    if (pixman_image == NULL) {
	_cairo_error (CAIRO_STATUS_NO_MEMORY);
	return (cairo_surface_t*) &_cairo_surface_nil;
    }

    surface = _cairo_image_surface_create_for_pixman_image (pixman_image, format);
    if (cairo_surface_status (surface)) {
	pixman_image_unref (pixman_image);
    }

    return surface;
}
slim_hidden_def (cairo_image_surface_create_for_data);

cairo_surface_t *
_cairo_image_surface_create_for_data_with_content (unsigned char	*data,
						   cairo_content_t	 content,
						   int			 width,
						   int			 height,
						   int			 stride)
{
    if (! CAIRO_CONTENT_VALID (content))
	return (cairo_surface_t*) &_cairo_surface_nil;

    return cairo_image_surface_create_for_data (data,
						_cairo_format_from_content (content),
						width, height, stride);
}

/**
 * cairo_image_surface_get_data:
 * @surface: a #cairo_image_surface_t
 *
 * Get a pointer to the data of the image surface, for direct
 * inspection or modification.
 *
 * Return value: a pointer to the image data of this surface or NULL
 * if @surface is not an image surface.
 *
 * Since: 1.2
 **/
unsigned char *
cairo_image_surface_get_data (cairo_surface_t *surface)
{
    cairo_image_surface_t *image_surface = (cairo_image_surface_t *) surface;

    if (!_cairo_surface_is_image (surface)) {
	_cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
	return NULL;
    }

    return image_surface->data;
}

/**
 * cairo_image_surface_get_format:
 * @surface: a #cairo_image_surface_t
 *
 * Get the format of the surface.
 *
 * Return value: the format of the surface
 *
 * Since: 1.2
 **/
cairo_format_t
cairo_image_surface_get_format (cairo_surface_t *surface)
{
    cairo_image_surface_t *image_surface = (cairo_image_surface_t *) surface;

    if (!_cairo_surface_is_image (surface)) {
	_cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
	return 0;
    }

    assert (CAIRO_FORMAT_VALID (image_surface->format));

    return image_surface->format;
}

/**
 * cairo_image_surface_get_width:
 * @surface: a #cairo_image_surface_t
 *
 * Get the width of the image surface in pixels.
 *
 * Return value: the width of the surface in pixels.
 **/
int
cairo_image_surface_get_width (cairo_surface_t *surface)
{
    cairo_image_surface_t *image_surface = (cairo_image_surface_t *) surface;

    if (!_cairo_surface_is_image (surface)) {
	_cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
	return 0;
    }

    return image_surface->width;
}
slim_hidden_def (cairo_image_surface_get_width);

/**
 * cairo_image_surface_get_height:
 * @surface: a #cairo_image_surface_t
 *
 * Get the height of the image surface in pixels.
 *
 * Return value: the height of the surface in pixels.
 **/
int
cairo_image_surface_get_height (cairo_surface_t *surface)
{
    cairo_image_surface_t *image_surface = (cairo_image_surface_t *) surface;

    if (!_cairo_surface_is_image (surface)) {
	_cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
	return 0;
    }

    return image_surface->height;
}
slim_hidden_def (cairo_image_surface_get_height);

/**
 * cairo_image_surface_get_stride:
 * @surface: a #cairo_image_surface_t
 *
 * Get the stride of the image surface in bytes
 *
 * Return value: the stride of the image surface in bytes (or 0 if
 * @surface is not an image surface). The stride is the distance in
 * bytes from the beginning of one row of the image data to the
 * beginning of the next row.
 *
 * Since: 1.2
 **/
int
cairo_image_surface_get_stride (cairo_surface_t *surface)
{

    cairo_image_surface_t *image_surface = (cairo_image_surface_t *) surface;

    if (!_cairo_surface_is_image (surface)) {
	_cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
	return 0;
    }

    return image_surface->stride;
}

cairo_format_t
_cairo_format_from_content (cairo_content_t content)
{
    switch (content) {
    case CAIRO_CONTENT_COLOR:
	return CAIRO_FORMAT_RGB24;
    case CAIRO_CONTENT_ALPHA:
	return CAIRO_FORMAT_A8;
    case CAIRO_CONTENT_COLOR_ALPHA:
	return CAIRO_FORMAT_ARGB32;
    }

    ASSERT_NOT_REACHED;
    return CAIRO_FORMAT_ARGB32;
}

cairo_content_t
_cairo_content_from_format (cairo_format_t format)
{
    /* XXX: Use an int to avoid the warnings from mixed cairo_format_t
     * and cairo_internal_format_t values. The warnings are extremely
     * valuable since mixing enums can lead to subtle bugs. It's just
     * that cairo_internal_format_t is an interim approach to getting
     * bug #7294 fixed so we can release cairo 1.2.2 . */
    int f = format;

    switch (f) {
    case CAIRO_FORMAT_ARGB32:
    case CAIRO_INTERNAL_FORMAT_ABGR32:
	return CAIRO_CONTENT_COLOR_ALPHA;
    case CAIRO_FORMAT_RGB24:
    case CAIRO_INTERNAL_FORMAT_RGB16_565:
    case CAIRO_INTERNAL_FORMAT_BGR24:
	return CAIRO_CONTENT_COLOR;
    case CAIRO_FORMAT_A8:
    case CAIRO_FORMAT_A1:
	return CAIRO_CONTENT_ALPHA;
    }

    ASSERT_NOT_REACHED;
    return CAIRO_CONTENT_COLOR_ALPHA;
}

static cairo_surface_t *
_cairo_image_surface_create_similar (void	       *abstract_src,
				     cairo_content_t	content,
				     int		width,
				     int		height)
{
    assert (CAIRO_CONTENT_VALID (content));

    return _cairo_image_surface_create_with_content (content,
						     width, height);
}

static cairo_status_t
_cairo_image_surface_finish (void *abstract_surface)
{
    cairo_image_surface_t *surface = abstract_surface;

    if (surface->pixman_image) {
	pixman_image_unref (surface->pixman_image);
	surface->pixman_image = NULL;
    }

    if (surface->owns_data) {
	free (surface->data);
	surface->data = NULL;
    }

    return CAIRO_STATUS_SUCCESS;
}

void
_cairo_image_surface_assume_ownership_of_data (cairo_image_surface_t *surface)
{
    surface->owns_data = 1;
}

static cairo_status_t
_cairo_image_surface_acquire_source_image (void                    *abstract_surface,
					   cairo_image_surface_t  **image_out,
					   void                   **image_extra)
{
    *image_out = abstract_surface;
    *image_extra = NULL;

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_image_surface_release_source_image (void                   *abstract_surface,
					   cairo_image_surface_t  *image,
					   void                   *image_extra)
{
}

static cairo_status_t
_cairo_image_surface_acquire_dest_image (void                    *abstract_surface,
					 cairo_rectangle_int_t   *interest_rect,
					 cairo_image_surface_t  **image_out,
					 cairo_rectangle_int_t   *image_rect_out,
					 void                   **image_extra)
{
    cairo_image_surface_t *surface = abstract_surface;

    image_rect_out->x = 0;
    image_rect_out->y = 0;
    image_rect_out->width = surface->width;
    image_rect_out->height = surface->height;

    *image_out = surface;
    *image_extra = NULL;

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_image_surface_release_dest_image (void                    *abstract_surface,
					 cairo_rectangle_int_t   *interest_rect,
					 cairo_image_surface_t   *image,
					 cairo_rectangle_int_t   *image_rect,
					 void                    *image_extra)
{
}

static cairo_status_t
_cairo_image_surface_clone_similar (void		*abstract_surface,
				    cairo_surface_t	*src,
				    int                  src_x,
				    int                  src_y,
				    int                  width,
				    int                  height,
				    cairo_surface_t    **clone_out)
{
    cairo_image_surface_t *surface = abstract_surface;

    if (src->backend == surface->base.backend) {
	*clone_out = cairo_surface_reference (src);

	return CAIRO_STATUS_SUCCESS;
    }

    return CAIRO_INT_STATUS_UNSUPPORTED;
}

static cairo_status_t
_cairo_image_surface_set_matrix (cairo_image_surface_t	*surface,
				 const cairo_matrix_t	*matrix)
{
    pixman_transform_t pixman_transform;

    _cairo_matrix_to_pixman_matrix (matrix, &pixman_transform);

    if (!pixman_image_set_transform (surface->pixman_image, &pixman_transform))
	return CAIRO_STATUS_NO_MEMORY;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_image_surface_set_filter (cairo_image_surface_t *surface, cairo_filter_t filter)
{
    pixman_filter_t pixman_filter;

    switch (filter) {
    case CAIRO_FILTER_FAST:
	pixman_filter = PIXMAN_FILTER_FAST;
	break;
    case CAIRO_FILTER_GOOD:
	pixman_filter = PIXMAN_FILTER_GOOD;
	break;
    case CAIRO_FILTER_BEST:
	pixman_filter = PIXMAN_FILTER_BEST;
	break;
    case CAIRO_FILTER_NEAREST:
	pixman_filter = PIXMAN_FILTER_NEAREST;
	break;
    case CAIRO_FILTER_BILINEAR:
	pixman_filter = PIXMAN_FILTER_BILINEAR;
	break;
    case CAIRO_FILTER_GAUSSIAN:
	/* XXX: The GAUSSIAN value has no implementation in cairo
	 * whatsoever, so it was really a mistake to have it in the
	 * API. We could fix this by officially deprecating it, or
	 * else inventing semantics and providing an actual
	 * implementation for it. */
    default:
	pixman_filter = PIXMAN_FILTER_BEST;
    }

    pixman_image_set_filter (surface->pixman_image, pixman_filter, NULL, 0);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_image_surface_set_attributes (cairo_image_surface_t      *surface,
				     cairo_surface_attributes_t *attributes)
{
    cairo_int_status_t status;

    status = _cairo_image_surface_set_matrix (surface, &attributes->matrix);
    if (status)
	return status;

    switch (attributes->extend) {
    case CAIRO_EXTEND_NONE:
        pixman_image_set_repeat (surface->pixman_image, PIXMAN_REPEAT_NONE);
	break;
    case CAIRO_EXTEND_REPEAT:
        pixman_image_set_repeat (surface->pixman_image, PIXMAN_REPEAT_NORMAL);
	break;
    case CAIRO_EXTEND_REFLECT:
        pixman_image_set_repeat (surface->pixman_image, PIXMAN_REPEAT_REFLECT);
	break;
    case CAIRO_EXTEND_PAD:
        pixman_image_set_repeat (surface->pixman_image, PIXMAN_REPEAT_PAD);
	break;
    }

    status = _cairo_image_surface_set_filter (surface, attributes->filter);

    return status;
}

/* XXX: I think we should fix pixman to match the names/order of the
 * cairo operators, but that will likely be better done at the same
 * time the X server is ported to pixman, (which will change a lot of
 * things in pixman I think).
 */
static pixman_op_t
_pixman_operator (cairo_operator_t op)
{
    switch (op) {
    case CAIRO_OPERATOR_CLEAR:
	return PIXMAN_OP_CLEAR;

    case CAIRO_OPERATOR_SOURCE:
	return PIXMAN_OP_SRC;
    case CAIRO_OPERATOR_OVER:
	return PIXMAN_OP_OVER;
    case CAIRO_OPERATOR_IN:
	return PIXMAN_OP_IN;
    case CAIRO_OPERATOR_OUT:
	return PIXMAN_OP_OUT;
    case CAIRO_OPERATOR_ATOP:
	return PIXMAN_OP_ATOP;

    case CAIRO_OPERATOR_DEST:
	return PIXMAN_OP_DST;
    case CAIRO_OPERATOR_DEST_OVER:
	return PIXMAN_OP_OVER_REVERSE;
    case CAIRO_OPERATOR_DEST_IN:
	return PIXMAN_OP_IN_REVERSE;
    case CAIRO_OPERATOR_DEST_OUT:
	return PIXMAN_OP_OUT_REVERSE;
    case CAIRO_OPERATOR_DEST_ATOP:
	return PIXMAN_OP_ATOP_REVERSE;

    case CAIRO_OPERATOR_XOR:
	return PIXMAN_OP_XOR;
    case CAIRO_OPERATOR_ADD:
	return PIXMAN_OP_ADD;
    case CAIRO_OPERATOR_SATURATE:
	return PIXMAN_OP_SATURATE;
    default:
	return PIXMAN_OP_OVER;
    }
}

static cairo_int_status_t
_cairo_image_surface_composite (cairo_operator_t	op,
				cairo_pattern_t		*src_pattern,
				cairo_pattern_t		*mask_pattern,
				void			*abstract_dst,
				int			src_x,
				int			src_y,
				int			mask_x,
				int			mask_y,
				int			dst_x,
				int			dst_y,
				unsigned int		width,
				unsigned int		height)
{
    cairo_surface_attributes_t	src_attr, mask_attr;
    cairo_image_surface_t	*dst = abstract_dst;
    cairo_image_surface_t	*src;
    cairo_image_surface_t	*mask;
    cairo_int_status_t		status;

    status = _cairo_pattern_acquire_surfaces (src_pattern, mask_pattern,
					      &dst->base,
					      src_x, src_y,
					      mask_x, mask_y,
					      width, height,
					      (cairo_surface_t **) &src,
					      (cairo_surface_t **) &mask,
					      &src_attr, &mask_attr);
    if (status)
	return status;

    status = _cairo_image_surface_set_attributes (src, &src_attr);
    if (status)
      goto CLEANUP_SURFACES;

    if (mask)
    {
	status = _cairo_image_surface_set_attributes (mask, &mask_attr);
	if (status)
	    goto CLEANUP_SURFACES;

	pixman_image_composite (_pixman_operator (op),
				src->pixman_image,
				mask->pixman_image,
				dst->pixman_image,
				src_x + src_attr.x_offset,
				src_y + src_attr.y_offset,
				mask_x + mask_attr.x_offset,
				mask_y + mask_attr.y_offset,
				dst_x, dst_y,
				width, height);
    }
    else
    {
	pixman_image_composite (_pixman_operator (op),
				src->pixman_image,
				NULL,
				dst->pixman_image,
				src_x + src_attr.x_offset,
				src_y + src_attr.y_offset,
				0, 0,
				dst_x, dst_y,
				width, height);
    }

    if (!_cairo_operator_bounded_by_source (op))
	status = _cairo_surface_composite_fixup_unbounded (&dst->base,
							   &src_attr, src->width, src->height,
							   mask ? &mask_attr : NULL,
							   mask ? mask->width : 0,
							   mask ? mask->height : 0,
							   src_x, src_y,
							   mask_x, mask_y,
							   dst_x, dst_y, width, height);

 CLEANUP_SURFACES:
    if (mask)
	_cairo_pattern_release_surface (mask_pattern, &mask->base, &mask_attr);

    _cairo_pattern_release_surface (src_pattern, &src->base, &src_attr);

    return status;
}

#define STACK_RECTS_LEN (CAIRO_STACK_BUFFER_SIZE / sizeof(pixman_rectangle16_t))

static cairo_int_status_t
_cairo_image_surface_fill_rectangles (void		      *abstract_surface,
				      cairo_operator_t	       op,
				      const cairo_color_t     *color,
				      cairo_rectangle_int_t   *rects,
				      int		       num_rects)
{
    cairo_image_surface_t *surface = abstract_surface;

    pixman_color_t pixman_color;
    pixman_rectangle16_t stack_rects[STACK_RECTS_LEN];
    pixman_rectangle16_t *pixman_rects = stack_rects;
    int i;

    cairo_int_status_t status = CAIRO_STATUS_SUCCESS;

    pixman_color.red   = color->red_short;
    pixman_color.green = color->green_short;
    pixman_color.blue  = color->blue_short;
    pixman_color.alpha = color->alpha_short;

    if (num_rects > ARRAY_LENGTH(stack_rects)) {
	pixman_rects = _cairo_malloc_ab (num_rects, sizeof(pixman_rectangle16_t));
	if (pixman_rects == NULL)
	    return CAIRO_STATUS_NO_MEMORY;
    }		 

    for (i = 0; i < num_rects; i++) {
	pixman_rects[i].x = rects[i].x;
	pixman_rects[i].y = rects[i].y;
	pixman_rects[i].width = rects[i].width;
	pixman_rects[i].height = rects[i].height;
    }

    /* XXX: pixman_fill_rectangles() should be implemented */
    if (!pixman_image_fill_rectangles (_pixman_operator(op),
				       surface->pixman_image,
				       &pixman_color,
				       num_rects,
				       pixman_rects))
	status = CAIRO_STATUS_NO_MEMORY;

    if (pixman_rects != stack_rects)
	free (pixman_rects);

    return status;
}

#undef STACK_RECTS_LEN

#define STACK_TRAPS_LEN ((int) (CAIRO_STACK_BUFFER_SIZE / sizeof(pixman_trapezoid_t)))

static cairo_int_status_t
_cairo_image_surface_composite_trapezoids (cairo_operator_t	op,
					   cairo_pattern_t	*pattern,
					   void			*abstract_dst,
					   cairo_antialias_t	antialias,
					   int			src_x,
					   int			src_y,
					   int			dst_x,
					   int			dst_y,
					   unsigned int		width,
					   unsigned int		height,
					   cairo_trapezoid_t	*traps,
					   int			num_traps)
{
    cairo_surface_attributes_t	attributes;
    cairo_image_surface_t	*dst = abstract_dst;
    cairo_image_surface_t	*src;
    cairo_int_status_t		status;
    pixman_image_t		*mask;
    pixman_format_code_t	 format;
    uint32_t			*mask_data;
    pixman_trapezoid_t		 stack_traps[STACK_TRAPS_LEN];
    pixman_trapezoid_t		*pixman_traps = stack_traps;
    int				 mask_stride;
    int				 mask_bpp;
    int				 ret, i;

    /* Convert traps to pixman traps */
    if (num_traps > ARRAY_LENGTH(stack_traps)) {
	pixman_traps = _cairo_malloc_ab (num_traps, sizeof(pixman_trapezoid_t));
	if (pixman_traps == NULL)
	    return CAIRO_STATUS_NO_MEMORY;
    }

    for (i = 0; i < num_traps; i++) {
	pixman_traps[i].top = _cairo_fixed_to_16_16 (traps[i].top);
	pixman_traps[i].bottom = _cairo_fixed_to_16_16 (traps[i].bottom);
	pixman_traps[i].left.p1.x = _cairo_fixed_to_16_16 (traps[i].left.p1.x);
	pixman_traps[i].left.p1.y = _cairo_fixed_to_16_16 (traps[i].left.p1.y);
	pixman_traps[i].left.p2.x = _cairo_fixed_to_16_16 (traps[i].left.p2.x);
	pixman_traps[i].left.p2.y = _cairo_fixed_to_16_16 (traps[i].left.p2.y);
	pixman_traps[i].right.p1.x = _cairo_fixed_to_16_16 (traps[i].right.p1.x);
	pixman_traps[i].right.p1.y = _cairo_fixed_to_16_16 (traps[i].right.p1.y);
	pixman_traps[i].right.p2.x = _cairo_fixed_to_16_16 (traps[i].right.p2.x);
	pixman_traps[i].right.p2.y = _cairo_fixed_to_16_16 (traps[i].right.p2.y);
    }

    /* Special case adding trapezoids onto a mask surface; we want to avoid
     * creating an intermediate temporary mask unecessarily.
     *
     * We make the assumption here that the portion of the trapezoids
     * contained within the surface is bounded by [dst_x,dst_y,width,height];
     * the Cairo core code passes bounds based on the trapezoid extents.
     *
     * Currently the check surface->has_clip is needed for correct
     * functioning, since pixman_add_trapezoids() doesn't obey the
     * surface clip, which is a libpixman bug , but there's no harm in
     * falling through to the general case when the surface is clipped
     * since libpixman would have to generate an intermediate mask anyways.
     */
    if (op == CAIRO_OPERATOR_ADD &&
	_cairo_pattern_is_opaque_solid (pattern) &&
	dst->base.content == CAIRO_CONTENT_ALPHA &&
	!dst->has_clip &&
	antialias != CAIRO_ANTIALIAS_NONE)
    {
	pixman_add_trapezoids (dst->pixman_image, 0, 0,
			       num_traps, pixman_traps);
	status = CAIRO_STATUS_SUCCESS;
	goto finish;
    }

    status = _cairo_pattern_acquire_surface (pattern, &dst->base,
					     src_x, src_y, width, height,
					     (cairo_surface_t **) &src,
					     &attributes);
    if (status)
	goto finish;

    status = _cairo_image_surface_set_attributes (src, &attributes);
    if (status)
	goto CLEANUP_SOURCE;

    switch (antialias) {
    case CAIRO_ANTIALIAS_NONE:
	format = PIXMAN_a1;
	ret = 1;
	assert (ret);
	mask_stride = ((width + 31) / 8) & ~0x03;
	mask_bpp = 1;
 	break;
    case CAIRO_ANTIALIAS_GRAY:
    case CAIRO_ANTIALIAS_SUBPIXEL:
    case CAIRO_ANTIALIAS_DEFAULT:
    default:
	format = PIXMAN_a8;
	ret = 1;
	assert (ret);
	mask_stride = (width + 3) & ~3;
	mask_bpp = 8;
 	break;
    }

    /* The image must be initially transparent */
    mask_data = calloc (1, mask_stride * height);
    if (mask_data == NULL) {
	status = CAIRO_STATUS_NO_MEMORY;
	goto CLEANUP_SOURCE;
    }

    mask = pixman_image_create_bits (format, width, height,
				     mask_data, mask_stride);
    if (mask == NULL) {
	status = CAIRO_STATUS_NO_MEMORY;
	goto CLEANUP_IMAGE_DATA;
    }

    pixman_add_trapezoids (mask, - dst_x, - dst_y,
			   num_traps, pixman_traps);

    pixman_image_composite (_pixman_operator (op),
			    src->pixman_image,
			    mask,
			    dst->pixman_image,
			    src_x + attributes.x_offset,
			    src_y + attributes.y_offset,
			    0, 0,
			    dst_x, dst_y,
			    width, height);

    if (!_cairo_operator_bounded_by_mask (op))
	status = _cairo_surface_composite_shape_fixup_unbounded (&dst->base,
								 &attributes, src->width, src->height,
								 width, height,
								 src_x, src_y,
								 0, 0,
								 dst_x, dst_y, width, height);
    pixman_image_unref (mask);

 CLEANUP_IMAGE_DATA:
    free (mask_data);

 CLEANUP_SOURCE:
    _cairo_pattern_release_surface (pattern, &src->base, &attributes);

 finish:
    if (pixman_traps != stack_traps)
	free (pixman_traps);

    return status;
}

#undef STACK_TRAPS_LEN

cairo_int_status_t
_cairo_image_surface_set_clip_region (void *abstract_surface,
				      cairo_region_t *region)
{
    cairo_image_surface_t *surface = (cairo_image_surface_t *) abstract_surface;

    if (!pixman_image_set_clip_region (surface->pixman_image, &region->rgn))
	return CAIRO_STATUS_NO_MEMORY;

    surface->has_clip = region != NULL;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_image_surface_get_extents (void			  *abstract_surface,
				  cairo_rectangle_int_t   *rectangle)
{
    cairo_image_surface_t *surface = abstract_surface;

    rectangle->x = 0;
    rectangle->y = 0;
    rectangle->width  = surface->width;
    rectangle->height = surface->height;

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_image_surface_get_font_options (void                  *abstract_surface,
				       cairo_font_options_t  *options)
{
    _cairo_font_options_init_default (options);

    cairo_font_options_set_hint_metrics (options, CAIRO_HINT_METRICS_ON);
}

static cairo_status_t
_cairo_image_surface_reset (void *abstract_surface)
{
    cairo_image_surface_t *surface = abstract_surface;
    cairo_status_t status;

    status = _cairo_image_surface_set_clip_region (surface, NULL);
    if (status)
	return status;

    return CAIRO_STATUS_SUCCESS;
}

/**
 * _cairo_surface_is_image:
 * @surface: a #cairo_surface_t
 *
 * Checks if a surface is an #cairo_image_surface_t
 *
 * Return value: TRUE if the surface is an image surface
 **/
cairo_bool_t
_cairo_surface_is_image (const cairo_surface_t *surface)
{
    return surface->backend == &cairo_image_surface_backend;
}

const cairo_surface_backend_t cairo_image_surface_backend = {
    CAIRO_SURFACE_TYPE_IMAGE,
    _cairo_image_surface_create_similar,
    _cairo_image_surface_finish,
    _cairo_image_surface_acquire_source_image,
    _cairo_image_surface_release_source_image,
    _cairo_image_surface_acquire_dest_image,
    _cairo_image_surface_release_dest_image,
    _cairo_image_surface_clone_similar,
    _cairo_image_surface_composite,
    _cairo_image_surface_fill_rectangles,
    _cairo_image_surface_composite_trapezoids,
    NULL, /* copy_page */
    NULL, /* show_page */
    _cairo_image_surface_set_clip_region,
    NULL, /* intersect_clip_path */
    _cairo_image_surface_get_extents,
    NULL, /* old_show_glyphs */
    _cairo_image_surface_get_font_options,
    NULL, /* flush */
    NULL, /* mark_dirty_rectangle */
    NULL, //* font_fini */
    NULL, //* glyph_fini */

    NULL, /* paint */
    NULL, /* mask */
    NULL, /* stroke */
    NULL, /* fill */
    NULL, /* show_glyphs */
    NULL,  /* snapshot */
    NULL, /* is_similar */

    _cairo_image_surface_reset
};

/* A convenience function for when one needs to coerce an image
 * surface to an alternate format. */
cairo_image_surface_t *
_cairo_image_surface_clone (cairo_image_surface_t	*surface,
			    cairo_format_t		 format)
{
    cairo_image_surface_t *clone;
    cairo_t *cr;
    double x, y;

    clone = (cairo_image_surface_t *)
	cairo_image_surface_create (format,
				    surface->width, surface->height);

    /* Use _cairo_surface_composite directly */
    cr = cairo_create (&clone->base);
    cairo_surface_get_device_offset (&surface->base, &x, &y);
    cairo_set_source_surface (cr, &surface->base, x, y);
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint (cr);
    cairo_destroy (cr);

    cairo_surface_set_device_offset (&clone->base, x, y);

    return clone;
}
