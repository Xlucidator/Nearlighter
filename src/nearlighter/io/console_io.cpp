#include <nearlighter/io/console_io.h>

#include <nearlighter/render/renderer.h>

#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>

ConsoleOutput::ConsoleOutput(std::ostream& output, bool show_progress,
                             double update_interval_seconds)
    : output_(output),
      show_progress_(show_progress),
      update_interval_seconds_(update_interval_seconds) {
    if (update_interval_seconds <= 0.0) {
        throw std::invalid_argument(
            "Console update interval must be positive");
    }
}

void ConsoleOutput::beginRender(const std::string& scene_name) {
    last_update_seconds_ = 0.0;
    output_ << "Rendering '" << scene_name << "'...\n" << std::flush;
}

void ConsoleOutput::updateRender(const RenderProgress& progress) {
    if (!show_progress_) return;

    const double integration_seconds = progress.integration_time.count();
    const bool complete = progress.completed_rows == progress.total_rows;
    if (!complete && integration_seconds - last_update_seconds_ <
                         update_interval_seconds_) {
        return;
    }

    const double completion =
        static_cast<double>(progress.completed_rows) /
        static_cast<double>(progress.total_rows);
    const double remaining_seconds =
        completion > 0.0
            ? integration_seconds * (1.0 - completion) / completion
            : 0.0;

    std::ostringstream message;
    message << std::fixed << std::setprecision(1)
            << "Rendering: " << completion * 100.0 << "% ("
            << progress.completed_rows << '/' << progress.total_rows
            << " rows), elapsed " << integration_seconds << " s, ETA "
            << remaining_seconds << " s";
    output_ << '\r' << message.str() << "    " << std::flush;
    last_update_seconds_ = integration_seconds;
}

void ConsoleOutput::finishRender(const RenderStats& stats) {
    if (show_progress_) output_ << '\n';
    output_ << "Render: " << stats.integration_time.count() << " s, "
            << stats.samplesPerSecond() << " samples/s\n"
            << std::flush;
}
