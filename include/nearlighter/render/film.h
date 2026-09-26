#ifndef NEARLIGHTER_RENDER_FILM_H
#define NEARLIGHTER_RENDER_FILM_H

#include <nearlighter/base/image.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

/** Optional in-memory outputs accumulated alongside the beauty image. */
enum class FilmAOV : std::uint32_t {
    None = 0,
    Beauty = 1U << 0U,
    Emission = 1U << 1U,
    Direct = 1U << 2U,
    Indirect = 1U << 3U,
    Variance = 1U << 4U,
    All = (1U << 0U) | (1U << 1U) | (1U << 2U) | (1U << 3U) |
          (1U << 4U),
};

constexpr FilmAOV operator|(FilmAOV left, FilmAOV right) {
    return static_cast<FilmAOV>(static_cast<std::uint32_t>(left) |
                                static_cast<std::uint32_t>(right));
}

constexpr FilmAOV operator&(FilmAOV left, FilmAOV right) {
    return static_cast<FilmAOV>(static_cast<std::uint32_t>(left) &
                                static_cast<std::uint32_t>(right));
}

constexpr bool hasAOV(FilmAOV value, FilmAOV requested) {
    return (value & requested) != FilmAOV::None;
}

/** One primary-path contribution classified for Film accumulation. */
struct FilmSample {
    Color beauty;
    Color emission;
    Color direct;
    Color indirect;

    /** Creates a sample whose beauty is the sum of classified components. */
    static FilmSample fromComponents(const Color& emission,
                                     const Color& direct,
                                     const Color& indirect) {
        return FilmSample{emission + direct + indirect, emission, direct,
                          indirect};
    }
};

/**
 * Accumulates typed render outputs and per-pixel sample statistics.
 *
 * Every exposed color layer stores the arithmetic mean of all samples added
 * to that pixel. Sample variance is the unbiased component-wise beauty
 * variance and is black until at least two samples are present.
 */
class Film {
public:
    /** Creates zero-initialized layers for a positive image extent. */
    Film(int width, int height, FilmAOV aovs = FilmAOV::All);

    int width() const { return beauty_.width(); }
    int height() const { return beauty_.height(); }
    FilmAOV enabledAOVs() const { return enabled_aovs_; }

    /** Adds one complete primary-path contribution to a pixel. */
    void addSample(int x, int y, const FilmSample& sample);

    /** Adds a beauty-only sample for integrators without AOV classification. */
    void addBeautySample(int x, int y, const Color& beauty) {
        addSample(x, y, FilmSample{beauty, Color(), Color(), Color()});
    }

    const Image& beauty() const { return beauty_; }
    /** @name Optional Color Layers
     * Returns an enabled layer or throws std::logic_error when its AOV was not
     * allocated.
     * @{ */
    const Image& emission() const;
    const Image& direct() const;
    const Image& indirect() const;
    const Image& variance() const;
    /** @} */

    /** Returns the exact number of primary samples accumulated at a pixel. */
    std::uint64_t sampleCountAt(int x, int y) const;

private:
    std::size_t index(int x, int y) const;

    FilmAOV enabled_aovs_ = FilmAOV::All;
    Image beauty_;
    Image beauty_sum_;
    std::optional<Image> emission_;
    std::optional<Image> emission_sum_;
    std::optional<Image> direct_;
    std::optional<Image> direct_sum_;
    std::optional<Image> indirect_;
    std::optional<Image> indirect_sum_;
    std::optional<Image> variance_;
    std::optional<Image> beauty_m2_;
    std::vector<std::uint64_t> sample_counts_;
};

#endif  // NEARLIGHTER_RENDER_FILM_H
