#include "imageloader.h"
#include "math.h"

#define STB_IMAGE_IMPLEMENTATION
// #define STBI_FAILURE_USERMSG
#include <stb_image.h>

ImageLoader::ImageLoader(const std::string& file_path) {
    // stbi_ldr_to_hdr_gamma(1.0f);

    // auto image_dir = getenv("IMAGE_DIR");
    // if (image_dir && load(std::string(image_dir) + "/" + file_path)) return;
    if (load(file_path)) return;
    std::cerr << "ERROR: Could not load image file '" << file_path << "'." << std::endl;
}
    
ImageLoader::~ImageLoader() {
    if (_data != nullptr) stbi_image_free(_data);
}

bool ImageLoader::load(const std::string& file_path) {
    _data = stbi_load(file_path.c_str(), &_width, &_height, &bytes_per_pixel, desired_channels);
    bytes_per_scanline = _width * bytes_per_pixel;
    return _data != nullptr;
}

const unsigned char* ImageLoader::getPixelData(int x, int y) const {
    // Default: magenta
    static unsigned char magenta[] = { 255, 0, 255 };
    if (_data == nullptr) return magenta;

    // Clamp x and y to the image boundaries
    x = clamp_idx(x, 0, _width);
    y = clamp_idx(y, 0, _height);

    return _data + y * bytes_per_scanline + x * bytes_per_pixel;
}
