#ifndef BVHNODE_H
#define BVHNODE_H

#include "common.h"
#include "aabb.h"
#include "shape/shapelist.h"

class BVHNode : public Shape {
public:
    BVHNode(ShapeList& list): BVHNode(list.objects.begin(), list.objects.end()) {}
    BVHNode(std::vector<shared_ptr<Shape>>::iterator start, std::vector<shared_ptr<Shape>>::iterator end);
    
    bool hit(const Ray& ray, Interval ray_t, HitRecord& record) const;

    AABB getBoundingBox() const { return bbox; }
    bool isLeaf() const { return shapes != nullptr; }

private:
    /* Config */
    const size_t max_size = 2;
    int div_axis = 0;     // divide axis, x=0, y=1, z=2
    bool bbox_cmp(const shared_ptr<Shape>& a, const shared_ptr<Shape>& b);

    /* Content */
    shared_ptr<BVHNode> lchild, rchild;
    unique_ptr<ShapeList> shapes = nullptr;
    AABB bbox;
};

#endif // BVHNODE_H