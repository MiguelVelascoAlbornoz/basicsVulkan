#include "Images.h"
#include "../Renderer/Image.h"


std::unordered_map<std::string, Image*> Images::images;

void Images::freeImages() {
    for ( std::pair<std::string,Image*> image: images) {
        delete image.second;
    }
    images.clear();
}

/** @brief Mesh del cubo, cada vertice esta duplicado y contiene la normal de la cara a la que pertenece. El lado del cubo es 1 por default y esta centrado en el origen**/
Image* Images::missingImage;

