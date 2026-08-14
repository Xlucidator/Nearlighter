#ifndef NEARLIGHTER_IO_MESH_IO_H
#define NEARLIGHTER_IO_MESH_IO_H

#include <nearlighter/shape/mesh.h>

#include <filesystem>

/** Loads indexed geometry from a Wavefront OBJ file. */
MeshData loadOBJ(const std::filesystem::path& path);

/** Loads indexed geometry from an ASCII or little-endian PLY file. */
MeshData loadPLY(const std::filesystem::path& path);

/** Dispatches mesh loading according to the lowercase file extension. */
MeshData loadMeshData(const std::filesystem::path& path);

#endif  // NEARLIGHTER_IO_MESH_IO_H
