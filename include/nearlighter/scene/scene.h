
#ifndef SCENE_H
#define SCENE_H

#include <nearlighter/nearlighter.h>

#include <nearlighter/geometry/shape_list.h>
#include <nearlighter/render/camera.h>
#include <nearlighter/accel/bvh_node.h>

void set_scenery_boucingSpheres(ShapeList& world, Camera& camera, bool is_random = true);
void set_scenery_checkerSpheres(ShapeList& world, Camera& camera);
void set_scenery_earth(ShapeList& world, Camera& camera);
void set_scenery_perlinSphere(ShapeList& world, Camera& camera);
void set_scenery_quads(ShapeList& world, Camera& camera);
void set_scenery_simpleLight(ShapeList& world, Camera& camera);
void set_scenery_CornellBox(ShapeList& world, Camera& camera, ShapeList& lights);
void set_scenery_CornellBox(ShapeList& world, Camera& camera, ShapeList& lights, int width, int spp, int max_depth);
void set_scenery_CornellSmoke(ShapeList& world, Camera& camera);
void set_scenery_finalScene(ShapeList& world, Camera& camera, int width, int spp, int max_depth);

void set_scenery_CornellBall(ShapeList& world, Camera& camera, ShapeList& lights, int width, int spp, int max_depth);

#endif
