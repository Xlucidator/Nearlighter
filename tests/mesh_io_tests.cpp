#include "test_support.h"

#include <nearlighter/io/mesh_io.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

template <typename UInt>
void writeLittleUnsigned(std::ostream& output, UInt value) {
    for (std::size_t index = 0; index < sizeof(UInt); ++index) {
        output.put(static_cast<char>((value >> (index * 8U)) & 0xffU));
    }
}

void writeLittleFloat(std::ostream& output, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(value));
    writeLittleUnsigned(output, bits);
}

void writeOBJ(const std::filesystem::path& path) {
    std::ofstream output(path);
    output << "v -1 -1 -1\n"
           << "v 1 -1 -1\n"
           << "v 1 1 -1\n"
           << "v -1 1 -1\n"
           << "vt 0 0\n"
           << "vt 1 0\n"
           << "vt 1 1\n"
           << "vt 0 1\n"
           << "vn 0 0 1\n"
           << "f -4/-4/1 -3/-3/1 -2/-2/1 -1/-1/1\n";
}

void writeASCIIPLY(const std::filesystem::path& path) {
    std::ofstream output(path);
    output << "ply\n"
           << "format ascii 1.0\n"
           << "element vertex 4\n"
           << "property float x\n"
           << "property float y\n"
           << "property float z\n"
           << "element face 1\n"
           << "property list uchar int vertex_indices\n"
           << "end_header\n"
           << "-1 -1 -1\n"
           << "1 -1 -1\n"
           << "1 1 -1\n"
           << "-1 1 -1\n"
           << "4 0 1 2 3\n";
}

void writeBinaryPLY(const std::filesystem::path& path) {
    std::ofstream output(path, std::ios::binary);
    output << "ply\n"
           << "format binary_little_endian 1.0\n"
           << "element vertex 3\n"
           << "property float x\n"
           << "property float y\n"
           << "property float z\n"
           << "element face 1\n"
           << "property list uchar int vertex_indices\n"
           << "end_header\n";
    const std::array<std::array<float, 3>, 3> positions = {{
        {{0.0f, 0.0f, -1.0f}},
        {{1.0f, 0.0f, -1.0f}},
        {{0.0f, 1.0f, -1.0f}},
    }};
    for (const auto& position : positions) {
        for (float component : position) writeLittleFloat(output, component);
    }
    output.put(3);
    writeLittleUnsigned(output, std::uint32_t{0});
    writeLittleUnsigned(output, std::uint32_t{1});
    writeLittleUnsigned(output, std::uint32_t{2});
}

void expectQuad(nearlighter::test::Context& context, const MeshData& data,
                std::string_view label) {
    const std::array<Point3f, 4> positions{
        Point3f(-1, -1, -1), Point3f(1, -1, -1),
        Point3f(1, 1, -1), Point3f(-1, 1, -1)};
    const std::vector<std::array<std::uint32_t, 3>> triangles{{0, 1, 2}, {0, 2, 3}};
    context.expectTrue(data.positions.size() == 4,
                       std::string(label) + " vertex count");
    context.expectTrue(data.triangles == triangles,
                       std::string(label) + " fan triangulation and index mapping");
    if (data.positions.size() != positions.size()) return;
    for (std::size_t i = 0; i < positions.size(); ++i) {
        context.expectVecNear(data.positions[i], positions[i], 0,
                              std::string(label) + " vertex coordinates");
    }
}

}  // namespace

int main([[maybe_unused]] int argc, char* argv[]) {
    nearlighter::test::Context context;
    const std::filesystem::path directory =
        std::filesystem::absolute(argv[0]).parent_path() / "mesh_io_fixture";
    std::filesystem::create_directories(directory);

    const std::filesystem::path obj_path = directory / "quad.obj";
    const std::filesystem::path ascii_ply_path = directory / "quad_ascii.ply";
    const std::filesystem::path binary_ply_path = directory / "triangle_binary.ply";
    writeOBJ(obj_path);
    writeASCIIPLY(ascii_ply_path);
    writeBinaryPLY(binary_ply_path);

    const MeshData obj = loadOBJ(obj_path);
    expectQuad(context, obj, "OBJ");
    context.expectTrue(obj.normals.size() == obj.positions.size(),
                       "OBJ aligned normal count");
    context.expectTrue(obj.texture_coordinates.size() == obj.positions.size(),
                       "OBJ aligned texture-coordinate count");
    const std::vector<std::array<float, 2>> expected_uv{{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    context.expectTrue(obj.texture_coordinates == expected_uv,
                       "OBJ texture values remain aligned after negative-index remapping");
    for (const auto& normal : obj.normals) {
        context.expectVecNear(normal, Vec3f(0, 0, 1), 0,
                              "OBJ normal values survive import");
    }

    expectQuad(context, loadPLY(ascii_ply_path), "ASCII PLY");
    const MeshData binary_ply = loadPLY(binary_ply_path);
    context.expectTrue(binary_ply.positions.size() == 3,
                       "binary PLY vertex count");
    context.expectTrue(binary_ply.triangles.size() == 1,
                       "binary PLY face count");
    const std::array<Point3f, 3> binary_positions{
        Point3f(0, 0, -1), Point3f(1, 0, -1), Point3f(0, 1, -1)};
    if (binary_ply.positions.size() == binary_positions.size()) {
        for (std::size_t i = 0; i < binary_positions.size(); ++i) {
            context.expectVecNear(binary_ply.positions[i], binary_positions[i], 0,
                                  "binary PLY preserves little-endian float coordinates");
        }
    }

    std::filesystem::remove_all(directory);
    return context.finish("mesh I/O tests");
}
