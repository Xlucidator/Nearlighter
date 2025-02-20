#include "bvhnode.h"

#include <algorithm>

// Using Iterator Argument
BVHNode::BVHNode(std::vector<shared_ptr<Shape>>::iterator start, std::vector<shared_ptr<Shape>>::iterator end) {
    size_t size = end - start;

    /* Within Max Size: Stop Division */
    if (size <= max_size) {
        shapes = make_unique<ShapeList>();
        std::for_each(start, end, [this](auto& s){ shapes->add(s); });

        lchild = rchild = nullptr; 
        bbox = shapes->getBoundingBox();
        return;
    } 
    
    /* Recursively Divide */
    // Calculate the longest axis of total AABBs
    bbox = AABB::empty;
    std::for_each(start, end, [this](auto& s){ bbox.uunion(s->getBoundingBox()); });
    div_axis = bbox.longestAxis();

    auto mid = start + (size >> 1);
    std::nth_element(start, mid, end, [this](auto& a, auto& b){ return bbox_cmp(a, b); });
    
    lchild = make_shared<BVHNode>(start, mid);
    rchild = make_shared<BVHNode>( mid , end);
    bbox = AABB(lchild->getBoundingBox(), rchild->getBoundingBox());
}


bool BVHNode::hit(const Ray& ray, Interval ray_t, HitRecord& record) const {
    if (!bbox.hit(ray, ray_t)) return false;
    if (isLeaf()) return shapes->hit(ray, ray_t, record);
    
    bool hit_lchild = lchild->hit(ray, ray_t, record);
    if (hit_lchild) ray_t.max = record.t;
    bool hit_rchild = rchild->hit(ray, ray_t, record);
    
    return hit_lchild || hit_rchild;
}

bool BVHNode::bbox_cmp(const shared_ptr<Shape>& a, const shared_ptr<Shape>& b) {
    const Interval& a_axis_interval = a->getBoundingBox().getAxisInterval(div_axis);
    const Interval& b_axis_interval = b->getBoundingBox().getAxisInterval(div_axis);
    return a_axis_interval.min < b_axis_interval.min; // TODO: use centroid, compare effects
}

void BVHNode::printNode(int level) const {
    for (int i = 0; i < level; ++i) printf("\t");
    std::cout << bbox << ", divided by axis" << div_axis;
    if (isLeaf()) {
        // std::cout << " | shapes size = " << shapes->size() << " [LEAF]" << std::endl;
        std::cout << std::endl;
        return;
    }
    std::cout << std::endl;

    lchild->printNode(level+1);
    rchild->printNode(level+1); 
}
