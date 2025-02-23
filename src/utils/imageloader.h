#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <iostream>
#include <string>

class ImageLoader {
public:
    ImageLoader() {}
    ImageLoader(const std::string& filename);
    ~ImageLoader();
    
    bool load(const std::string& filename);

    bool hasData() const { return _data != nullptr; }
    int width() const { return _data == nullptr ? 0 : _width; }
    int height() const { return _data == nullptr ? 0 : _height; }

    const unsigned char* getPixelData(int x, int y) const;

private:
    const int desired_channels = 3;  // <=> bytes per pixel
    int bytes_per_pixel, bytes_per_scanline;
    
    int _width = 0, _height = 0;
    unsigned char* _data = nullptr;
};



#endif // IMAGELOADER_H
