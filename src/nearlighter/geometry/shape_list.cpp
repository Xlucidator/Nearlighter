#include <nearlighter/geometry/shape_list.h>

#include <nearlighter/sampling/sampler.h>

ShapeList::ShapeList() {}

ShapeList::ShapeList(shared_ptr<Shape> object) {
    add(object);
}

bool ShapeList::hit(const Ray& r, Interval ray_t, HitRecord& closest_hitrec,
                    Sampler& sampler) const {
    HitRecord tmp_rec;
    bool hit_anything = false;
    float closest_t = ray_t.max;

    for (const auto& object : objects) {
        if (object->hit(r, Interval(ray_t.min, closest_t), tmp_rec, sampler)) {
            hit_anything = true;
            closest_t = tmp_rec.t;
            closest_hitrec = tmp_rec;
        }
    }

    return hit_anything;
}

bool ShapeList::hasPDF() const {
    for (const auto& object : objects) {
        if (object->hasPDF()) return true;
    }
    return false;
}

float ShapeList::getPDFValue(const Point3f& origin, const Vec3f& direction) const {
    if (objects.empty()) return 0.0f;

    float weight = 1.0f / objects.size();
    float sum = 0.0f;
    for (const auto& object : objects) {
        sum += weight * object->getPDFValue(origin, direction);
    }
    return sum;
}

Vec3f ShapeList::random(const Point3f& origin, Sampler& sampler) const {
    if (objects.empty()) return Vec3f(1, 0, 0);
    int index = sampler.nextInt(0, static_cast<int>(objects.size()) - 1);
    return objects[index]->random(origin, sampler);
}

void ShapeList::clear() {
    objects.clear();
}

void ShapeList::add(shared_ptr<Shape> object) {
    objects.push_back(object);
    bbox.uunion(object->getBoundingBox());
}
