#include <nearlighter/render/hit/surface_intersection.h>
#include <nearlighter/material/material.h>
#include <nearlighter/shape/shape.h>

bool SurfaceIntersection::supports(const Material& material) const {
    return material.kind() == MaterialKind::Surface;
}

bool SurfaceIntersection::hit(const Shape& shape, const Material&,
                              const Ray& local_ray, Interval ray_t, float,
                              HitRecord& record, Sampler&) const {
    ShapeHit geometry;
    if (!shape.hit(local_ray, ray_t, geometry)) return false;
    SurfaceRecord surface;
    surface.point = geometry.point;
    surface.t = geometry.t;
    surface.u = geometry.u;
    surface.v = geometry.v;
    surface.setFaceNormals(local_ray, geometry.geometric_normal, geometry.shading_normal);
    record = surface;
    return true;
}
