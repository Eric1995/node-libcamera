/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2021, Raspberry Pi (Trading) Ltd.
 *
 * image.hpp - still image encoder declarations
 */

#pragma once

#include <string>
#include <iostream>
#include <string.h>
#include <valarray>

#include <libcamera/base/span.h>

#include <libcamera/controls.h>

#include "../core/stream_info.hpp"
#include "../core/still_options.hpp"


#define LOG(level, text)                                                                                               \
	do                                                                                                                 \
	{                                                                                                                  \
		if (0 >= level)                                                                     \
			std::cerr << text << std::endl;                                                                            \
	} while (0)
#define LOG_ERROR(text) std::cerr << text << std::endl

// struct StillOptions;

// In jpeg.cpp:
void jpeg_save(std::vector<libcamera::Span<uint8_t>> const &mem, StreamInfo const &info,
			   libcamera::ControlList const &metadata, std::string const &filename, std::string const &cam_model,
			   StillOptions const *options);

// In yuv.cpp:
void yuv_save(std::vector<libcamera::Span<uint8_t>> const &mem, StreamInfo const &info,
			  std::string const &filename, StillOptions const *options);

// In dng.cpp:
void dng_save(std::vector<libcamera::Span<uint8_t>> const &mem, StreamInfo const &info,
			  libcamera::ControlList const &metadata, std::string const &filename, std::string const &cam_model,
			  StillOptions const *options);

// In png.cpp:
void png_save(std::vector<libcamera::Span<uint8_t>> const &mem, StreamInfo const &info,
			  std::string const &filename, StillOptions const *options);

// In bmp.cpp:
void bmp_save(std::vector<libcamera::Span<uint8_t>> const &mem, StreamInfo const &info,
			  std::string const &filename, StillOptions const *options);

// In qoi.cpp:
void qoi_save(std::vector<libcamera::Span<uint8_t>> const &mem, StreamInfo const &info,
			  std::string const &filename, StillOptions const *options);
