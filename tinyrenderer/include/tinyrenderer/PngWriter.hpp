#pragma once

#include "TGAImage.hpp"

/**
 * @brief Writes a TGAImage to a PNG file via stb_image_write.
 *
 * Handles the byte-order swap from the internal BGR(A) layout to the RGB(A)
 * layout that PNG expects. Throws std::runtime_error if the write fails.
 */
void writePng(const TGAImage &image, const char *filename);
