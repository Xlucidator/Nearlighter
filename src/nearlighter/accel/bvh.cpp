#include <nearlighter/accel/bvh.h>

#include <nearlighter/scene/linear_aggregate.h>

#include <algorithm>
#include <stdexcept>
#include <utility>

struct BVH::Node {
    AABB bounds;
    std::shared_ptr<const Intersectable> object;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;

    /** Builds one binary subtree over the non-empty half-open object span. */
    Node(std::vector<std::shared_ptr<const Intersectable>>& objects,
         std::size_t begin, std::size_t end) {
        /* ----- Collect Node bounds ----- */
        for (std::size_t index = begin; index < end; ++index) {
            bounds.uunion(objects[index]->getBoundingBox());
        }

        /* ----- Endcase: Single-object leaf ----- */
        const std::size_t count = end - begin;
        if (count == 1) {
            object = objects[begin]; // not null object denote it's leaf
            return;
        }

        /* ----- Recursive: Centroid median partition ----- */
        const int axis = bounds.longestAxis();
        const std::size_t middle = begin + count / 2;
        std::nth_element(
            objects.begin() + begin, objects.begin() + middle,
            objects.begin() + end,
            [axis](const auto& a, const auto& b) {
                return a->getBoundingBox().centroid()[axis] <
                       b->getBoundingBox().centroid()[axis];
            }
        ); // nth_element is sufficient: don't need fully sorted span
        left = std::make_unique<Node>(objects, begin, middle);
        right = std::make_unique<Node>(objects, middle, end);
    }

    /** Returns the closest descendant interaction within ray_t. */
    bool hit(const Ray& ray, Interval ray_t, HitRecord& record,
             Sampler& sampler) const {
        /* ----- Endcase: Test hit bound & single object ----- */
        if (!bounds.hit(ray, ray_t)) return false;
        if (object) return object->hit(ray, ray_t, record, sampler);

        /* ----- Recursive: Closest-child selection ----- */
        HitRecord left_record;
        const bool hit_left = left->hit(ray, ray_t, left_record, sampler);
        HitRecord right_record;
        const bool hit_right = right->hit(
            ray, Interval(ray_t.min, hit_left ? left_record.t : ray_t.max),
            right_record, sampler);  // only search on a closer bound
        if (hit_right) {
            record = right_record;
        } else if (hit_left) {
            record = left_record;
        }
        return hit_left || hit_right;
    }
};


BVH::BVH(const LinearAggregate& aggregate)
    : BVH(aggregate.objects()) {}

BVH::BVH(std::vector<std::shared_ptr<const Intersectable>> objects) {
    if (objects.empty()) {
        throw std::invalid_argument("BVH requires at least one object");
    }
    if (std::any_of(objects.begin(), objects.end(),
                    [](const auto& object) { return !object; })) {
        throw std::invalid_argument("BVH cannot contain null objects");
    }
    root_ = std::make_unique<Node>(objects, 0, objects.size());
    bounds_ = root_->bounds;
}

BVH::~BVH() = default;
BVH::BVH(BVH&&) noexcept = default;
BVH& BVH::operator=(BVH&&) noexcept = default;

bool BVH::hit(const Ray& ray, Interval ray_t, HitRecord& record,
              Sampler& sampler) const {
    return root_->hit(ray, ray_t, record, sampler);
}
