#include <nearlighter/io/mesh_io.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

// ==================================================
// Shared Utilities
// ==================================================

[[noreturn]] void fail(const std::filesystem::path& path,
                       const std::string& reason) {
    throw std::runtime_error("Failed to load mesh '" + path.string() +
                             "': " + reason);
}

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::tolower(character));
                   });
    return value;
}

void triangulate(const std::vector<std::uint32_t>& polygon,
                 std::vector<std::array<std::uint32_t, 3>>& triangles) {
    for (std::size_t index = 1; index + 1 < polygon.size(); ++index) {
        triangles.push_back({polygon[0], polygon[index], polygon[index + 1]});
    }
}

// ==================================================
// OBJ Decoder
// ==================================================

struct OBJVertexReference {
    int position = 0;
    int texture_coordinate = 0;
    int normal = 0;

    bool operator==(const OBJVertexReference& other) const {
        return position == other.position &&
               texture_coordinate == other.texture_coordinate &&
               normal == other.normal;
    }
};

struct OBJVertexReferenceHash {
    std::size_t operator()(const OBJVertexReference& value) const {
        std::size_t seed = std::hash<int>{}(value.position);
        seed ^= std::hash<int>{}(value.texture_coordinate) +
                0x9e3779b9U + (seed << 6U) + (seed >> 2U);
        seed ^= std::hash<int>{}(value.normal) +
                0x9e3779b9U + (seed << 6U) + (seed >> 2U);
        return seed;
    }
};

int parseOBJIndex(const std::string& text, std::size_t count,
                  const std::filesystem::path& path,
                  std::size_t line_number) {
    if (text.empty()) return 0;
    int raw_index = 0;
    try {
        std::size_t consumed = 0;
        raw_index = std::stoi(text, &consumed);
        if (consumed != text.size()) throw std::invalid_argument("suffix");
    } catch (const std::exception&) {
        fail(path, "invalid OBJ index at line " +
                       std::to_string(line_number));
    }
    if (raw_index == 0) {
        fail(path, "OBJ indices are one-based at line " +
                       std::to_string(line_number));
    }

    const long long resolved = raw_index > 0
                                   ? static_cast<long long>(raw_index - 1)
                                   : static_cast<long long>(count) + raw_index;
    if (resolved < 0 || resolved >= static_cast<long long>(count)) {
        fail(path, "OBJ index is out of range at line " +
                       std::to_string(line_number));
    }
    return static_cast<int>(resolved + 1);
}

OBJVertexReference parseOBJReference(
    const std::string& token, std::size_t position_count,
    std::size_t texture_coordinate_count, std::size_t normal_count,
    const std::filesystem::path& path, std::size_t line_number) {
    std::array<std::string, 3> fields;
    std::size_t field_index = 0;
    std::size_t field_start = 0;
    while (field_index < fields.size()) {
        const std::size_t slash = token.find('/', field_start);
        fields[field_index++] = token.substr(
            field_start, slash == std::string::npos
                             ? std::string::npos
                             : slash - field_start);
        if (slash == std::string::npos) break;
        if (field_index == fields.size()) {
            fail(path, "too many OBJ face index fields at line " +
                           std::to_string(line_number));
        }
        field_start = slash + 1;
    }

    OBJVertexReference reference;
    reference.position = parseOBJIndex(fields[0], position_count, path,
                                       line_number);
    if (reference.position == 0) {
        fail(path, "OBJ face is missing a position at line " +
                       std::to_string(line_number));
    }
    reference.texture_coordinate = parseOBJIndex(
        fields[1], texture_coordinate_count, path, line_number);
    reference.normal = parseOBJIndex(fields[2], normal_count, path,
                                     line_number);
    return reference;
}

// ==================================================
// PLY Decoder
// ==================================================

enum class PLYFormat {
    ASCII,
    BinaryLittleEndian,
};

enum class PLYScalarType {
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Float32,
    Float64,
};

struct PLYProperty {
    std::string name;
    bool is_list = false;
    PLYScalarType scalar_type = PLYScalarType::Float32;
    PLYScalarType count_type = PLYScalarType::UInt8;
};

struct PLYElement {
    std::string name;
    std::size_t count = 0;
    std::vector<PLYProperty> properties;
};

struct PLYHeader {
    PLYFormat format = PLYFormat::ASCII;
    std::vector<PLYElement> elements;
};

PLYScalarType parsePLYType(const std::string& name,
                           const std::filesystem::path& path) {
    if (name == "char" || name == "int8") return PLYScalarType::Int8;
    if (name == "uchar" || name == "uint8") return PLYScalarType::UInt8;
    if (name == "short" || name == "int16") return PLYScalarType::Int16;
    if (name == "ushort" || name == "uint16") return PLYScalarType::UInt16;
    if (name == "int" || name == "int32") return PLYScalarType::Int32;
    if (name == "uint" || name == "uint32") return PLYScalarType::UInt32;
    if (name == "float" || name == "float32") return PLYScalarType::Float32;
    if (name == "double" || name == "float64") return PLYScalarType::Float64;
    fail(path, "unsupported PLY scalar type '" + name + "'");
}

PLYHeader readPLYHeader(std::istream& input,
                        const std::filesystem::path& path) {
    std::string line;
    if (!std::getline(input, line)) {
        fail(path, "missing PLY magic header");
    }
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != "ply") {
        fail(path, "missing PLY magic header");
    }

    PLYHeader header;
    PLYElement* current_element = nullptr;
    bool has_format = false;
    bool has_end_header = false;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream parser(line);
        std::string keyword;
        parser >> keyword;
        if (keyword.empty() || keyword == "comment" || keyword == "obj_info") {
            continue;
        }
        if (keyword == "format") {
            std::string format;
            std::string version;
            parser >> format >> version;
            if (version != "1.0") fail(path, "unsupported PLY version");
            if (format == "ascii") {
                header.format = PLYFormat::ASCII;
            } else if (format == "binary_little_endian") {
                header.format = PLYFormat::BinaryLittleEndian;
            } else if (format == "binary_big_endian") {
                fail(path, "binary big-endian PLY is not supported");
            } else {
                fail(path, "unsupported PLY format '" + format + "'");
            }
            has_format = true;
        } else if (keyword == "element") {
            PLYElement element;
            parser >> element.name >> element.count;
            if (!parser) fail(path, "invalid PLY element declaration");
            header.elements.push_back(std::move(element));
            current_element = &header.elements.back();
        } else if (keyword == "property") {
            if (!current_element) {
                fail(path, "PLY property appears before an element");
            }
            std::string type;
            parser >> type;
            PLYProperty property;
            if (type == "list") {
                std::string count_type;
                std::string scalar_type;
                parser >> count_type >> scalar_type >> property.name;
                property.is_list = true;
                property.count_type = parsePLYType(count_type, path);
                property.scalar_type = parsePLYType(scalar_type, path);
            } else {
                parser >> property.name;
                property.scalar_type = parsePLYType(type, path);
            }
            if (!parser) fail(path, "invalid PLY property declaration");
            current_element->properties.push_back(std::move(property));
        } else if (keyword == "end_header") {
            has_end_header = true;
            break;
        } else {
            fail(path, "unsupported PLY header directive '" + keyword + "'");
        }
    }
    if (!has_format || !has_end_header) {
        fail(path, "incomplete PLY header");
    }
    return header;
}

template <typename UInt>
UInt readLittleUnsigned(std::istream& input,
                        const std::filesystem::path& path) {
    std::array<unsigned char, sizeof(UInt)> bytes{};
    input.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    if (!input) fail(path, "truncated binary PLY payload");
    UInt value = 0;
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        value |= static_cast<UInt>(bytes[index]) << (index * 8U);
    }
    return value;
}

template <typename Signed, typename Unsigned>
Signed readLittleSigned(std::istream& input,
                        const std::filesystem::path& path) {
    static_assert(sizeof(Signed) == sizeof(Unsigned),
                  "PLY signed and unsigned scalar sizes must match");
    const Unsigned bits = readLittleUnsigned<Unsigned>(input, path);
    Signed value = 0;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

double readBinaryScalar(std::istream& input, PLYScalarType type,
                        const std::filesystem::path& path) {
    switch (type) {
        case PLYScalarType::Int8:
            return readLittleSigned<std::int8_t, std::uint8_t>(input, path);
        case PLYScalarType::UInt8:
            return readLittleUnsigned<std::uint8_t>(input, path);
        case PLYScalarType::Int16:
            return readLittleSigned<std::int16_t, std::uint16_t>(input, path);
        case PLYScalarType::UInt16:
            return readLittleUnsigned<std::uint16_t>(input, path);
        case PLYScalarType::Int32:
            return readLittleSigned<std::int32_t, std::uint32_t>(input, path);
        case PLYScalarType::UInt32:
            return readLittleUnsigned<std::uint32_t>(input, path);
        case PLYScalarType::Float32: {
            const std::uint32_t bits =
                readLittleUnsigned<std::uint32_t>(input, path);
            float value = 0.0f;
            std::memcpy(&value, &bits, sizeof(value));
            return value;
        }
        case PLYScalarType::Float64: {
            const std::uint64_t bits =
                readLittleUnsigned<std::uint64_t>(input, path);
            double value = 0.0;
            std::memcpy(&value, &bits, sizeof(value));
            return value;
        }
    }
    fail(path, "invalid binary PLY scalar type");
}

std::size_t checkedCount(double value, const std::filesystem::path& path) {
    if (value < 0.0 || value > static_cast<double>(
                                  std::numeric_limits<std::uint32_t>::max()) ||
        value != static_cast<double>(static_cast<std::uint32_t>(value))) {
        fail(path, "invalid PLY list count");
    }
    return static_cast<std::size_t>(value);
}

std::uint32_t checkedIndex(double value, const std::filesystem::path& path) {
    if (value < 0.0 || value > static_cast<double>(
                                  std::numeric_limits<std::uint32_t>::max()) ||
        value != static_cast<double>(static_cast<std::uint32_t>(value))) {
        fail(path, "invalid PLY vertex index");
    }
    return static_cast<std::uint32_t>(value);
}

bool isUProperty(const std::string& name) {
    return name == "u" || name == "s" || name == "texture_u";
}

bool isVProperty(const std::string& name) {
    return name == "v" || name == "t" || name == "texture_v";
}

}  // namespace

MeshData loadOBJ(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) fail(path, "file could not be opened");

    std::vector<Point3f> source_positions;
    std::vector<std::array<float, 2>> source_texture_coordinates;
    std::vector<Vec3f> source_normals;
    std::vector<OBJVertexReference> expanded_references;
    std::unordered_map<OBJVertexReference, std::uint32_t,
                       OBJVertexReferenceHash> expanded_indices;
    MeshData data;

    /* ----- Source records ----- */
    std::string line;
    std::size_t line_number = 0;
    bool any_texture_coordinates = false;
    bool all_texture_coordinates = true;
    bool any_normals = false;
    bool all_normals = true;
    while (std::getline(input, line)) {
        ++line_number;
        std::istringstream parser(line);
        std::string keyword;
        parser >> keyword;
        if (keyword.empty() || keyword[0] == '#') continue;

        if (keyword == "v") {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            parser >> x >> y >> z;
            if (!parser) fail(path, "invalid vertex at line " +
                                        std::to_string(line_number));
            source_positions.emplace_back(x, y, z);
        } else if (keyword == "vt") {
            float u = 0.0f;
            float v = 0.0f;
            parser >> u >> v;
            if (!parser) fail(path, "invalid texture coordinate at line " +
                                        std::to_string(line_number));
            source_texture_coordinates.push_back({u, v});
        } else if (keyword == "vn") {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            parser >> x >> y >> z;
            if (!parser) fail(path, "invalid normal at line " +
                                        std::to_string(line_number));
            source_normals.emplace_back(x, y, z);
        } else if (keyword == "f") {
            std::vector<std::uint32_t> polygon;
            std::string token;
            while (parser >> token) {
                const OBJVertexReference reference = parseOBJReference(
                    token, source_positions.size(),
                    source_texture_coordinates.size(), source_normals.size(),
                    path, line_number);
                any_texture_coordinates |= reference.texture_coordinate != 0;
                all_texture_coordinates &= reference.texture_coordinate != 0;
                any_normals |= reference.normal != 0;
                all_normals &= reference.normal != 0;

                const auto inserted = expanded_indices.emplace(
                    reference,
                    static_cast<std::uint32_t>(expanded_references.size()));
                if (inserted.second) expanded_references.push_back(reference);
                polygon.push_back(inserted.first->second);
            }
            if (polygon.size() < 3) {
                fail(path, "OBJ face has fewer than three vertices at line " +
                               std::to_string(line_number));
            }
            triangulate(polygon, data.triangles);
        }
        /* Object, group, smoothing, and material-library records do not alter
           geometry. The Scene owns one material for the complete Mesh. */
    }

    /* ----- Indexed output ----- */
    if (any_texture_coordinates && !all_texture_coordinates) {
        fail(path, "OBJ faces mix vertices with and without texture coordinates");
    }
    if (any_normals && !all_normals) {
        fail(path, "OBJ faces mix vertices with and without normals");
    }
    data.positions.reserve(expanded_references.size());
    if (any_texture_coordinates) {
        data.texture_coordinates.reserve(expanded_references.size());
    }
    if (any_normals) data.normals.reserve(expanded_references.size());
    for (const OBJVertexReference& reference : expanded_references) {
        data.positions.push_back(source_positions[reference.position - 1]);
        if (any_texture_coordinates) {
            data.texture_coordinates.push_back(
                source_texture_coordinates[reference.texture_coordinate - 1]);
        }
        if (any_normals) {
            data.normals.push_back(source_normals[reference.normal - 1]);
        }
    }
    if (data.positions.empty() || data.triangles.empty()) {
        fail(path, "OBJ contains no renderable faces");
    }
    return data;
}

MeshData loadPLY(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) fail(path, "file could not be opened");
    const PLYHeader header = readPLYHeader(input, path);
    MeshData data;

    /* ----- Element payload ----- */
    for (const PLYElement& element : header.elements) {
        for (std::size_t record_index = 0; record_index < element.count;
             ++record_index) {
            std::optional<std::istringstream> ascii_record;
            if (header.format == PLYFormat::ASCII) {
                std::string line;
                if (!std::getline(input, line)) {
                    fail(path, "truncated ASCII PLY payload");
                }
                ascii_record.emplace(line);
            }
            const auto read_scalar = [&](PLYScalarType type) {
                if (header.format == PLYFormat::BinaryLittleEndian) {
                    return readBinaryScalar(input, type, path);
                }
                double value = 0.0;
                *ascii_record >> value;
                if (!*ascii_record) fail(path, "invalid ASCII PLY scalar");
                return value;
            };

            std::optional<float> x;
            std::optional<float> y;
            std::optional<float> z;
            std::optional<float> nx;
            std::optional<float> ny;
            std::optional<float> nz;
            std::optional<float> u;
            std::optional<float> v;
            std::vector<std::uint32_t> polygon;
            for (const PLYProperty& property : element.properties) {
                if (property.is_list) {
                    const std::size_t count = checkedCount(
                        read_scalar(property.count_type), path);
                    std::vector<std::uint32_t> values;
                    if (element.name == "face" &&
                        (property.name == "vertex_indices" ||
                         property.name == "vertex_index")) {
                        values.reserve(count);
                    }
                    for (std::size_t index = 0; index < count; ++index) {
                        const double value = read_scalar(property.scalar_type);
                        if (element.name == "face" &&
                            (property.name == "vertex_indices" ||
                             property.name == "vertex_index")) {
                            values.push_back(checkedIndex(value, path));
                        }
                    }
                    if (!values.empty()) polygon = std::move(values);
                } else {
                    const float value =
                        static_cast<float>(read_scalar(property.scalar_type));
                    if (element.name == "vertex") {
                        if (property.name == "x") x = value;
                        else if (property.name == "y") y = value;
                        else if (property.name == "z") z = value;
                        else if (property.name == "nx") nx = value;
                        else if (property.name == "ny") ny = value;
                        else if (property.name == "nz") nz = value;
                        else if (isUProperty(property.name)) u = value;
                        else if (isVProperty(property.name)) v = value;
                    }
                }
            }

            if (element.name == "vertex") {
                if (!x || !y || !z) fail(path, "PLY vertex is missing x, y, or z");
                data.positions.emplace_back(*x, *y, *z);
                const bool has_any_normal = nx || ny || nz;
                if (has_any_normal && !(nx && ny && nz)) {
                    fail(path, "PLY vertex has an incomplete normal");
                }
                if (has_any_normal) data.normals.emplace_back(*nx, *ny, *nz);
                const bool has_any_uv = u || v;
                if (has_any_uv && !(u && v)) {
                    fail(path, "PLY vertex has incomplete texture coordinates");
                }
                if (has_any_uv) data.texture_coordinates.push_back({*u, *v});
            } else if (element.name == "face") {
                if (polygon.size() < 3) {
                    fail(path, "PLY face has fewer than three vertices");
                }
                triangulate(polygon, data.triangles);
            }
        }
    }

    /* ----- Mesh consistency ----- */
    if (data.positions.empty() || data.triangles.empty()) {
        fail(path, "PLY contains no renderable faces");
    }
    if (!data.normals.empty() && data.normals.size() != data.positions.size()) {
        fail(path, "PLY mixes vertices with and without normals");
    }
    if (!data.texture_coordinates.empty() &&
        data.texture_coordinates.size() != data.positions.size()) {
        fail(path, "PLY mixes vertices with and without texture coordinates");
    }
    for (const auto& triangle : data.triangles) {
        for (const std::uint32_t index : triangle) {
            if (index >= data.positions.size()) {
                fail(path, "PLY face index is out of range");
            }
        }
    }
    return data;
}

MeshData loadMeshData(const std::filesystem::path& path) {
    const std::string extension = lowercase(path.extension().string());
    if (extension == ".obj") return loadOBJ(path);
    if (extension == ".ply") return loadPLY(path);
    fail(path, "unsupported mesh extension '" + path.extension().string() + "'");
}
