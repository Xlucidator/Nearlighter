#ifndef BVHNODE_H
#define BVHNODE_H

#include "common.h"
#include "aabb.h"
#include "shape/shapelist.h"

#define OFFICIAL
// #define DEBUG_BVH

class BVHNode : public Shape {
public:
#ifndef OFFICIAL
    BVHNode(ShapeList& list): BVHNode(list.objects.begin(), list.objects.end()) {}
#else 
    BVHNode(ShapeList& list): BVHNode(list.objects, 0, list.objects.size()) {}
#endif
    BVHNode(std::vector<shared_ptr<Shape>>& objects, size_t start, size_t end);
    BVHNode(std::vector<shared_ptr<Shape>>::iterator start, std::vector<shared_ptr<Shape>>::iterator end);

    bool hit(const Ray& ray, Interval ray_t, HitRecord& record) const override;
    const AABB& getBoundingBox() const override { return bbox; }

    bool isLeaf() const { return shapes != nullptr; }

    /* Debug */
    void printNode(int level) const;

private:
    /* Config */
    const size_t max_size = 1;
    int div_axis = 0;     // divide axis, x=0, y=1, z=2
    bool bbox_cmp(const shared_ptr<Shape>& a, const shared_ptr<Shape>& b);

    /* Content */
    shared_ptr<BVHNode> lchild, rchild;
    shared_ptr<Shape> left, right; // Official
    unique_ptr<ShapeList> shapes = nullptr;
    size_t shape_size = 0;
    AABB bbox;
};

#endif // BVHNODE_H