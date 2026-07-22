#ifndef NEARLIGHTER_IO_CONSOLE_IO_H
#define NEARLIGHTER_IO_CONSOLE_IO_H

#include <iosfwd>
#include <string>

struct RenderProgress;
struct RenderStats;

/** Render status output for a text console */
class ConsoleOutput {
public:
    ConsoleOutput(std::ostream& output, bool show_progress = true,
                  double update_interval_seconds = 1.0);

    /** Starts a render status section */
    void beginRender(const std::string& scene_name);

    /** Publishes a rate-limited progress update */
    void updateRender(const RenderProgress& progress);

    /** Completes progress output and prints final metrics */
    void finishRender(const RenderStats& stats);

private:
    std::ostream& output_;
    bool show_progress_ = true;
    double update_interval_seconds_ = 1.0;
    double last_update_seconds_ = 0.0;
};

#endif  // NEARLIGHTER_IO_CONSOLE_IO_H
