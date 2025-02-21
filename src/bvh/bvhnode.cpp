#include "bvhnode.h"

#include <algorithm>

// Debug
void printBboxSequence(std::vector<shared_ptr<Shape>>::iterator start, std::vector<shared_ptr<Shape>>::iterator end, int axis) {
    std::cout << "[Check bbox sequence] axis = " << axis << std::endl;
    std::for_each(start, end, [axis](auto& s) {
        std::cout << s->getBoundingBox().getAxisInterval(axis).min << " ";
    });
    std::cout << std::endl;
}

// Official
BVHNode::BVHNode(std::vector<shared_ptr<Shape>>& objects, size_t start, size_t end) {
    // Build the bounding box of the span of source objects.
    bbox = AABB::empty;
    for (size_t object_index=start; object_index < end; object_index++)
        bbox = AABB(bbox, objects[object_index]->getBoundingBox());

    int axis = bbox.longestAxis();
    div_axis = axis;

    size_t object_span = end - start;
    shape_size = object_span;

    if (object_span == 1) {
        left = right = objects[start];
    } else if (object_span == 2) {
        left = objects[start];
        right = objects[start+1];
    } else {
        std::sort(std::begin(objects) + start, std::begin(objects) + end, [this](auto& a, auto& b){ return bbox_cmp(a, b); });
        
        // after sort check
        // printBboxSequence(std::begin(objects) + start, std::begin(objects) + end, div_axis);

        auto mid = start + object_span/2;
        left = make_shared<BVHNode>(objects, start, mid);
        right = make_shared<BVHNode>(objects, mid, end);
    }
}

// Using Iterator Argument
BVHNode::BVHNode(std::vector<shared_ptr<Shape>>::iterator start, std::vector<shared_ptr<Shape>>::iterator end) {
    size_t size = end - start;
    shape_size = size;

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
    // std::nth_element(start, mid, end, [this](auto& a, auto& b){ return bbox_cmp(a, b); });
    std::sort(start, end, [this](auto& a, auto& b){ return bbox_cmp(a, b); });
    printBboxSequence(start, end, div_axis);
    lchild = make_shared<BVHNode>(start, mid);
    rchild = make_shared<BVHNode>( mid , end);
}

#ifndef OFFICIAL
bool BVHNode::hit(const Ray& ray, Interval ray_t, HitRecord& record) const {
    if (!bbox.hit(ray, ray_t)) return false;
    if (isLeaf()) return shapes->hit(ray, ray_t, record);
    
    bool hit_lchild = lchild->hit(ray, ray_t, record);
    if (hit_lchild) ray_t.max = record.t;
    bool hit_rchild = rchild->hit(ray, ray_t, record);
    
    return hit_lchild || hit_rchild;
}
#else
bool BVHNode::hit(const Ray& r, Interval ray_t, HitRecord& rec) const {
    if (!bbox.hit(r, ray_t))
        return false;

    bool hit_left = left->hit(r, ray_t, rec);
    bool hit_right = right->hit(r, Interval(ray_t.min, hit_left ? rec.t : ray_t.max), rec);

    return hit_left || hit_right;
}
#endif

bool BVHNode::bbox_cmp(const shared_ptr<Shape>& a, const shared_ptr<Shape>& b) {
    const Interval& a_axis_interval = a->getBoundingBox().getAxisInterval(div_axis);
    const Interval& b_axis_interval = b->getBoundingBox().getAxisInterval(div_axis);
    return a_axis_interval.min < b_axis_interval.min; // TODO: use centroid, compare effects
}

void BVHNode::printNode(int level) const {
    if (isLeaf()) return;

    for (int i = 0; i < level; ++i) printf("\t");
    std::cout << bbox << ", longest axis: " << div_axis << ", contain shapes: " << shape_size;
    // if (isLeaf()) {
    //     // std::cout << " | shapes size = " << shapes->size() << " [LEAF]" << std::endl;
    //     std::cout << " | [LEAF] Object = " << shapes->objects[0] << std::endl;
    //     return;
    // }
    std::cout << std::endl;
#ifndef OFFICIAL
    lchild->printNode(level+1);
    rchild->printNode(level+1); 
#else
    left->printNode(level+1);
    right->printNode(level+1); 
#endif
}
