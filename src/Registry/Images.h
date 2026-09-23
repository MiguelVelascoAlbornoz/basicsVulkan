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
#define MISSING_IMAGE_ID "missing_image_id"

class Images {
public:
    static Image* missingImage;


    static std::unordered_map<std::string, Image*> images; /**< @brief Map to store menu rendering functions. */

    static Image* registerImages(const std::string& id, Image* image) {
        return registerObject(id, image, images);
    };


    static void freeImages();
};


#endif //BASICSVULKAN_MESHES_H

