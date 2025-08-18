
#ifndef SCENE_H
#define SCENE_H

#include "common.h"

#include "shape/shapelist.h"
#include "camera/camera.h"
#include "bvh/bvhnode.h"

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


#endif
