#include "test_support.h"

#include <nearlighter/io/mesh_io.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

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
    context.expectTrue(data.positions.size() == 4,
                       std::string(label) + " vertex count");
    context.expectTrue(data.triangles.size() == 2,
                       std::string(label) + " triangulated face count");
}

}  // namespace

int main() {
    nearlighter::test::Context context;
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "nearlighter_mesh_io_tests";
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

    expectQuad(context, loadPLY(ascii_ply_path), "ASCII PLY");
    const MeshData binary_ply = loadPLY(binary_ply_path);
    context.expectTrue(binary_ply.positions.size() == 3,
                       "binary PLY vertex count");
    context.expectTrue(binary_ply.triangles.size() == 1,
                       "binary PLY face count");

    std::filesystem::remove_all(directory);
    return context.finish("mesh I/O tests");
}
