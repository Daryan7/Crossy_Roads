#ifndef EULERCAMERA_H
#define EULERCAMERA_H
#include "basecamera.h"

class EulerCamera : public virtual BaseCamera {
public:
    float phi, theta, psi, d;

	EulerCamera();
    EulerCamera(float phi, float theta, float psi, float d);
    virtual ~EulerCamera();

	glm::mat4 viewMatrixInverse();
    void updateVM();
};

#endif // EULERCAMERA_H
