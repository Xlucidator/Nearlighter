#include <nearlighter/material/material.h>

Color Material::emitted(const Ray&, const HitRecord&, float, float, const Point3f&) const {
    return Color(0, 0, 0);
}

bool Material::scatter(const Ray&, const HitRecord&, ScatterRecord&) const {
    return false;
}

float Material::getScatterPDF(const Ray&, const HitRecord&, Ray&) const {
    return 0.0f;
}
