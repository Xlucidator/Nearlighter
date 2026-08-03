#ifndef BVHNODE_H
#define BVHNODE_H

#include <nearlighter/geometry/aabb.h>
#include <nearlighter/geometry/shape_list.h>

#include <cstddef>
#include <memory>
#include <vector>

#define OFFICIAL
// #define DEBUG_BVH

class BVHNode : public Shape {
public:
#ifndef OFFICIAL
    BVHNode(ShapeList& list): BVHNode(list.objects.begin(), list.objects.end()) {}
#else 
    BVHNode(ShapeList& list): BVHNode(list.objects, 0, list.objects.size()) {}
#endif
    BVHNode(std::vector<std::shared_ptr<Shape>>& objects,
            std::size_t start, std::size_t end);
    BVHNode(std::vector<std::shared_ptr<Shape>>::iterator start, std::vector<std::shared_ptr<Shape>>::iterator end);

    bool hit(const Ray& ray, Interval ray_t, HitRecord& record,
             Sampler& sampler) const override;
    const AABB& getBoundingBox() const override { return bbox; }

    bool isLeaf() const { return shapes != nullptr; }

    /* Debug */
    void printNode(int level) const;

private:
    /* Config */
    const std::size_t max_size = 1;
    int div_axis = 0;     // divide axis, x=0, y=1, z=2
    bool bbox_cmp(const std::shared_ptr<Shape>& a, const std::shared_ptr<Shape>& b);

    /* Content */
    std::shared_ptr<BVHNode> lchild, rchild;
    std::shared_ptr<Shape> left, right; // Official
    std::unique_ptr<ShapeList> shapes = nullptr;
    std::size_t shape_size = 0;
    AABB bbox;
};

#endif // BVHNODE_H
