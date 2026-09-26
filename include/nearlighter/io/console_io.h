#ifndef NEARLIGHTER_IO_CONSOLE_IO_H
#define NEARLIGHTER_IO_CONSOLE_IO_H

#include <chrono>
#include <iosfwd>
#include <string>

struct RenderProgress;
struct RenderStats;
struct SceneBuildStats;

/** Render status output for a text console */
class ConsoleOutput {
public:
    ConsoleOutput(std::ostream& output, bool show_progress = true,
                  double update_interval_seconds = 1.0);

    /** Starts a render status section */
    void beginRender(const std::string& scene_name);

    /** Reports load time and its Scene assembly/build components. */
    void reportSceneLoad(std::chrono::duration<double> load_time,
                         const SceneBuildStats& build_stats);

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
