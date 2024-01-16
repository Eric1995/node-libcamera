/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2020, Raspberry Pi Ltd
 *
 * dng_writer.h - DNG writer
 */

#pragma once

#include <libcamera/camera.h>
#include <libcamera/controls.h>
#include <libcamera/framebuffer.h>
#include <libcamera/stream.h>

class DNGWriter
{
  public:
    static int write(const char *filename, const libcamera::Camera *camera,
                     const libcamera::StreamConfiguration &config, const libcamera::ControlList &metadata,
                     const libcamera::FrameBuffer *buffer, const void *data);
};
