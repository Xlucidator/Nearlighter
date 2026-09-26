#include <nearlighter/render/film.h>

#include <stdexcept>

namespace {

int validatedDimension(int value) {
    if (value <= 0) {
        throw std::invalid_argument("Film dimensions must be positive");
    }
    return value;
}

}  // namespace

Film::Film(int width, int height, FilmAOV aovs)
    : enabled_aovs_(aovs | FilmAOV::Beauty),
      beauty_(validatedDimension(width), validatedDimension(height)),
      beauty_sum_(width, height),
      sample_counts_(static_cast<std::size_t>(width) *
                     static_cast<std::size_t>(height)) {
    if (hasAOV(enabled_aovs_, FilmAOV::Emission)) {
        emission_.emplace(width, height);
        emission_sum_.emplace(width, height);
    }
    if (hasAOV(enabled_aovs_, FilmAOV::Direct)) {
        direct_.emplace(width, height);
        direct_sum_.emplace(width, height);
    }
    if (hasAOV(enabled_aovs_, FilmAOV::Indirect)) {
        indirect_.emplace(width, height);
        indirect_sum_.emplace(width, height);
    }
    if (hasAOV(enabled_aovs_, FilmAOV::Variance)) {
        variance_.emplace(width, height);
        beauty_m2_.emplace(width, height);
    }
}

/**
 * @par Implementation
 * Beauty variance uses Welford's online update, avoiding cancellation from
 * subtracting two accumulated squared terms. The other layers only require a
 * running mean and therefore share the new sample count as their weight.
 */
void Film::addSample(int x, int y, const FilmSample& sample) {
    const std::size_t pixel_index = index(x, y);
    const std::uint64_t count = ++sample_counts_[pixel_index];
    const float inverse_count = 1.0f / static_cast<float>(count);

    Color& beauty = beauty_.at(x, y);
    const Color delta = sample.beauty - beauty;
    beauty_sum_.at(x, y) += sample.beauty;
    beauty = beauty_sum_.at(x, y) * inverse_count;
    if (hasAOV(enabled_aovs_, FilmAOV::Variance)) {
        Color& m2 = beauty_m2_->at(x, y);
        m2 += delta * (sample.beauty - beauty);
        variance_->at(x, y) = count > 1
                                  ? m2 / static_cast<float>(count - 1)
                                  : Color();
    }

    if (hasAOV(enabled_aovs_, FilmAOV::Emission)) {
        emission_sum_->at(x, y) += sample.emission;
        emission_->at(x, y) = emission_sum_->at(x, y) * inverse_count;
    }
    if (hasAOV(enabled_aovs_, FilmAOV::Direct)) {
        direct_sum_->at(x, y) += sample.direct;
        direct_->at(x, y) = direct_sum_->at(x, y) * inverse_count;
    }
    if (hasAOV(enabled_aovs_, FilmAOV::Indirect)) {
        indirect_sum_->at(x, y) += sample.indirect;
        indirect_->at(x, y) = indirect_sum_->at(x, y) * inverse_count;
    }
}

const Image& Film::emission() const {
    if (!emission_) throw std::logic_error("Film emission AOV is disabled");
    return *emission_;
}

const Image& Film::direct() const {
    if (!direct_) throw std::logic_error("Film direct AOV is disabled");
    return *direct_;
}

const Image& Film::indirect() const {
    if (!indirect_) throw std::logic_error("Film indirect AOV is disabled");
    return *indirect_;
}

const Image& Film::variance() const {
    if (!variance_) throw std::logic_error("Film variance AOV is disabled");
    return *variance_;
}

std::uint64_t Film::sampleCountAt(int x, int y) const {
    return sample_counts_[index(x, y)];
}

std::size_t Film::index(int x, int y) const {
    if (x < 0 || x >= width() || y < 0 || y >= height()) {
        throw std::out_of_range("Film pixel coordinates are outside the film");
    }
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width()) +
           static_cast<std::size_t>(x);
}
