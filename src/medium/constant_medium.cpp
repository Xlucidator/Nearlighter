#include "constant_medium.h"
#include "material/material.h"


// ConstantMedium::ConstantMedium(shared_ptr<Shape> boundary, float density, shared_ptr<Material> phase_function)
//     : boundary(boundary), density(density), phase_function(phase_function) {
//     neg_inv_density = -1 / density;
// }

ConstantMedium::ConstantMedium(shared_ptr<Shape> boundary, float density, shared_ptr<Texture> tex) 
    : boundary(boundary), density(density) {
    neg_inv_density = -1 / density;
    phase_function = make_shared<Isotropic>(tex);
}

ConstantMedium::ConstantMedium(shared_ptr<Shape> boundary, float density, const Color& albedo) 
    : boundary(boundary), density(density) {
    neg_inv_density = -1 / density;
    phase_function = make_shared<Isotropic>(albedo);
}

/* Ray: r = o + t \vec{d}
 * Hit:
 * 1. Whether the ray will travel through the boundary
 *      - case 1:  r----|------|---->
 *      - case 2:       |  r---|---->
 *      - case 3:       |      | r--> 
 *                    enter  leave
 *      => so we need to check record_enter & record_leave
 *    [Attention] We Assume the boundary shape to be a convex, that is it won't re-enter the boundary after it leave it
 *    TODO: implement for non-convex boundary
 * 2. Whether the ray will be scattered when traveling through the boundary
 *      => Pr(Scatter) = density * distance_to_scattered
 *       - let Pr(Scatterd) = - log(random_float())
 *      => distance_to_scattered = Pr(Scattered) / density
 *                               = log(random_float()) * (-1 / density)
 * 3. Set hit record
 */
bool ConstantMedium::hit(const Ray& r, Interval ray_t, HitRecord& record) const {
    // std::cout << "[Hit Constant Medium] origin = " << r.origin() << " -> direction = " << r.direction() << "\n";
    HitRecord record_enter, record_leave;

    /* Check Boundary First: whether in the boundary */
    if (!boundary->hit(r, Interval::universe, record_enter)) return false;  // check whether the ray enter the boundary
    // std::cout << "[Hit CM] pass bcheck 1: enter_t = " << record_enter.t << std::endl;
    if (!boundary->hit(r, Interval(record_enter.t + 0.0001f, infinity), record_leave)) return false;  // check whether the ray leave the boundary
    // std::cout << "\t pass bcheck 2: leave_t = " << record_leave.t << std::endl;
    if (record_leave.t < 0) return false; // the ray has already leave the boundary
    record_enter.t = std::fmax(record_enter.t, ray_t.min); 
    record_leave.t = std::fmin(record_leave.t, ray_t.max);
    if (record_enter.t >= record_leave.t) return false;

#ifdef MEDIUM_DEBUG
    // std::cout << "\t pass bcheck: enter_t = " << record_enter.t << " leave_t = " << record_leave.t << std::endl;
#endif

    /* Check Volume Scattered */
    float ray_length = r.direction().length(); //  speed of t: | \vec{d} |
    float distance_to_scattered = std::log(random_float()) * neg_inv_density;   // ray spread distance before scattered (hit) when travel inside the boundary
    float distance_to_travel = (record_leave.t - record_enter.t) * ray_length;  // the distance that ray need to travel inside the boundary
    if (distance_to_scattered > distance_to_travel) return false; // ray will not be scatterd in this travel interval

#ifdef MEDIUM_DEBUG
    std::cout << "\t pass scheck: enter_t = " << record_enter.t << " leave_t = " << record_leave.t << std::endl;
    // std::cout << "[Hit Constant Medium] really hit:  distance_to_scattered= " << distance_to_scattered << " distance_to_travel = " << distance_to_travel << "\n";
#endif

    /* Than It Really Hits & Scattered */
    record.t = record_enter.t + distance_to_scattered / ray_length;  // std::cout << "[Hit CM] " << record.t << std::endl;
    record.point = r.at(record.t);
    record.normal = Vec3f::random_unit_vector(); // test
    record.front_face = true;
    record.material = phase_function;

    return true;
}