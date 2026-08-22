layout(std140, binding = 0) uniform CameraUBO {

   mat4 viewProjectionMatrix;
   float time; //In seconds
   vec3 viewPos;
   vec3 viewDirection;
} cameraUBO;