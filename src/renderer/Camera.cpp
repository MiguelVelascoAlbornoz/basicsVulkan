#include "Camera.h"


static void normalizeAngle(float& angle) {
	 if (angle > 180) {
		 angle /= 180;
		 angle = glm::fract(angle);
		 angle *= 180;
		 angle = -180 + angle;
	 }
	 else if (angle < -180) {
		 angle /= 180;
		 angle = -glm::fract(-angle);
		 angle *= 180;
		 angle = 180 + angle;

	 }

 }
 /**
  * @brief updated de rotationMatrix and consequently all the dependent matrix and vector of the rotationMatrix (viewMatrix, viewProjectionMatrix, front, right and up vectors)
  * @details 
  * This function normalize each angle in a [-180;180] range except of the pitch.
  * The pitch is clamped to the range [-89;89].
  * The yawToRoll is used to set the angle (in Up axis) of rotation in which the roll is applied. Ex 0rad meeans that roll is alined with the plane perpendicular to front vector.
  * The direction vector are calculated by multiplying the rotationMatrix with the default direction vectors.
  * The rotationMatrix is calculated by multiplying the rotation matrices of each angle in the order: pitch * yaw * yawToRoll * roll * -yawToRoll.
  */
 void Camera::setRotation(float yaw, float pitch, float roll,float yawToRoll) {
	 normalizeAngle(yaw);
	 normalizeAngle(roll);
	 normalizeAngle(yawToRoll);
	 if (pitch > 89) {
		 pitch = 89;
	 }
	 else if (pitch < -89) {
		 pitch = -89;
	 }
	 this->pitch = pitch;
	 this->yaw = yaw;
	 this->roll = roll;
	 this->yawToRoll = yawToRoll;
	 const float rPitch = glm::radians(pitch);
	 const float rYaw = glm::radians(yaw);
	 const float rRoll = glm::radians(roll);
	 const float rYawToRoll = glm::radians(yawToRoll);
	 rotationMatrix =  glm::rotate(glm::mat4(1.0f), rPitch, glm::vec3(1.0f, 0.0f, 0.0f))* glm::rotate(glm::mat4(1.0f), rYaw, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), (rYawToRoll), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), rRoll, glm::vec3(0.0f, 0.0f, 1.0f)) * glm::rotate(glm::mat4(1.0f), -rYawToRoll, glm::vec3(0.0f, 1.0f, 0.0f)) ;
	 
     front = vec3(vec4(DEFAULT_WORLD_FRONT,1.0f) * rotationMatrix);
     right = vec3(vec4(DEFAULT_WORLD_RIGHT,1.0f) * rotationMatrix);
     up = vec3(vec4(DEFAULT_WORLD_UP,1.0f) * rotationMatrix);
     updateViewMatrix();
	 updateViewProjectionMatrix();
	
 }
 /**
  * @brief Updates the view matrix and consequently the matrix dependent on this.
  * @details The view matrix is calculated using the glm::lookAt function, which creates a view matrix based on the camera's position, the target point (position + front), and the up vector.
  */
void Camera::updateViewMatrix() {
	  viewMatrix = glm::lookAt(position,position+front,up);
 }
 /**
  * @brief Sets the perspective projection parameters and updates the projection matrix and the view-projection matrix.
  * @details The projection matrix is calculated using the glm::perspective function, which creates a perspectiveProjectio.
  */
void Camera::setPerspective(float fov, float aspectRatio, float nearPlane, float farPlane) {
    this->fov = fov;
    this->aspectRatio = aspectRatio;
    this->nearPlane = nearPlane;
    this->farPlane = farPlane;
    projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
	projectionMatrix[1][1] *=-1;
    updateViewProjectionMatrix();

}
/**
 * @brief Sets the orthogonal projection parameters and updates the projection matrix and the view-projection matrix.
 * @details The projection matrix is calculated using the glm::ortho function, which creates an orthographic projection matrix based on the specified left, right, bottom, top, near, and far clipping planes.
 */
void Camera::setOrtho(float left, float right, float up, float down, float mfar, float mnear) {
	projectionMatrix = glm::ortho(left,right,down,up,mnear,mfar);

	updateViewProjectionMatrix();

}
