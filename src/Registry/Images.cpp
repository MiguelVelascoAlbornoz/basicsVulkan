#include "Images.h"
#include "../Renderer/Image.h"


std::unordered_map<std::string, Image*> Images::images;

void Images::freeImages() {
    for ( std::pair<std::string,Image*> image: images) {
        delete image.second;
    }
    images.clear();
}

Image* Images::missingImage;
Image* Images::ifftImage;

