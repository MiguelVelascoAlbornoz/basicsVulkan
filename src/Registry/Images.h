//
// Created by migue on 23/07/2026.
//

//#include "Meshes.h"
//
// Created by migue on 23/07/2026.
//

#ifndef BASICSVULKAN_IMAGES_H
#define BASICSVULKAN_IMAGES_H

#include "../App/Utilitys.h"

class Image;
#define MISSING_IMAGE_ID "missing_image"
#define IFFT_IN_IMAGE_ID "ifft_in_image"
#define IFFT_OUT_IMAGE_ID "ifft_out_image"

#define IFFT_DERIVATES_TEMP_IMAGE_ID "ifft_derivates_temp_image"
#define GRASS_IMAGE_ID "grass"
class Images {
public:
    static Image* missingImage;
    static Image* ifftInImage;

    static std::unordered_map<std::string, Image*> images; /**< @brief Map to store menu rendering functions. */

    static Image* registerImages(const std::string& id, Image* image) {
        return registerObject(id, image, images);
    };


    static void freeImages();
};


#endif //BASICSVULKAN_MESHES_H

