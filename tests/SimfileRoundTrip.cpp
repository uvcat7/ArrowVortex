#include <System/File.h>

#include <Managers/StyleMan.h>

#include <Simfile/Parsing.h>
#include <Simfile/SegmentGroup.h>
#include <Simfile/Segments.h>
#include <Simfile/Tempo.h>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace Vortex {
namespace {

struct StyleManagerScope {
    StyleManagerScope() { StyleMan::create(); }
    ~StyleManagerScope() { StyleMan::destroy(); }
};

bool RunRoundTripTest(const fs::path& fixture) {
    StyleManagerScope styles;
    const fs::path test_root = fs::temp_directory_path() /
                               "ArrowVortex-ユニコード-roundtrip" /
                               fixture.extension().string().substr(1);
    const fs::path input_dir = test_root / "input-路径";
    const fs::path output_dir = test_root / "output-路径";
    const fs::path unicode_fixture = input_dir / fixture.filename();

    std::error_code error;
    fs::remove_all(test_root, error);
    error.clear();
    fs::create_directories(input_dir, error);
    fs::create_directories(output_dir, error);
    fs::copy_file(fixture, unicode_fixture,
                  fs::copy_options::overwrite_existing, error);
    if (error) {
        std::cerr << "Unable to prepare Unicode test path: " << error.message()
                  << '\n';
        return false;
    }

    Simfile original;
    std::cerr << "Loading " << pathToUtf8(unicode_fixture) << '\n';
    if (!LoadSimfile(original, unicode_fixture)) return false;

    const std::string title = original.title;
    const std::string artist = original.artist;
    const size_t chart_count = original.charts.size();
    const size_t bpm_count =
        original.tempo->segments->getList<BpmChange>().size();
    const size_t stop_count = original.tempo->segments->getList<Stop>().size();
    const size_t warp_count = original.tempo->segments->getList<Warp>().size();

    original.dir = pathToUtf8(output_dir / fs::path{});
    original.file = pathToUtf8(fixture.filename());
    std::cerr << "Saving " << pathToUtf8(fixture) << '\n';
    if (!SaveSimfile(original, original.format, false)) return false;

    fs::path saved = output_dir / fixture.filename();
    if (original.format == SIM_OSU) {
        saved.clear();
        for (const auto& entry : fs::directory_iterator(output_dir)) {
            if (entry.path().extension() == ".osu") {
                saved = entry.path();
                break;
            }
        }
    }
    if (saved.empty() || !fs::exists(saved)) return false;

    Simfile reloaded;
    std::cerr << "Reloading " << pathToUtf8(saved) << '\n';
    if (!LoadSimfile(reloaded, saved)) return false;
    const size_t reloaded_warp_count =
        reloaded.tempo->segments->getList<Warp>().size();
    const bool warps_match =
        original.format != SIM_SSC || reloaded_warp_count == warp_count;
    const bool matches =
        reloaded.title == title && reloaded.artist == artist &&
        reloaded.charts.size() == chart_count &&
        reloaded.tempo->segments->getList<BpmChange>().size() == bpm_count &&
        reloaded.tempo->segments->getList<Stop>().size() == stop_count &&
        warps_match;

    std::cout << "Round-trip test " << pathToUtf8(fixture) << ": "
              << (matches ? "PASS" : "FAIL") << '\n';
    if (!matches) {
        std::cerr << "Expected charts=" << chart_count << " bpms=" << bpm_count
                  << " stops=" << stop_count << " warps=" << warp_count
                  << "; actual charts=" << reloaded.charts.size() << " bpms="
                  << reloaded.tempo->segments->getList<BpmChange>().size()
                  << " stops="
                  << reloaded.tempo->segments->getList<Stop>().size()
                  << " warps=" << reloaded_warp_count << '\n';
    }
    fs::remove_all(test_root, error);
    return matches;
}

}  // namespace
}  // namespace Vortex

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: ArrowVortexSimfileRoundTrip <fixture>\n";
        return 2;
    }
    return Vortex::RunRoundTripTest(Vortex::utf8ToPath(argv[1])) ? 0 : 1;
}
