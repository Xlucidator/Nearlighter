#ifndef MAT4F_H
#define MAT4F_H

#include "vec3f.h"

class Mat4f {
public:
    float data[16]; // Column First: uniform to OpenGL

    Mat4f(float diagonal = 1.0f) {
        for (int i = 0; i < 16; i++) data[i] = 0.0f;
        data[0] = data[5] = data[10] = data[15] = diagonal;
    }

    Mat4f operator*(const Mat4f& other) const {
        Mat4f result;
        for (int col = 0; col < 4; col++) {
            for (int row = 0; row < 4; row++) {
                float sum = 0.0f;
                for (int k = 0; k < 4; k++) {
                    sum += data[k * 4 + row] * other.data[col * 4 + k];
                }
                result.data[col * 4 + row] = sum;
            }
        }
        return result;
    }

    static Mat4f translate(const Vec3f& translation) {
        Mat4f result;
        result.data[12] = translation.x();
        result.data[13] = translation.y();
        result.data[14] = translation.z();
        return result;
    }

    static Mat4f rotate(float angle_degrees, const Vec3f& axis) {
        // TODO: Quaternion
        // TDOO: Euler Angles
    }


    static Mat4f scale(const Vec3f& scaling) {
        Mat4f result;
        result.data[0] = scaling.x();
        result.data[5] = scaling.y();
        result.data[10] = scaling.z();
        return result;
    }
};

#endif // MAT4F_H
