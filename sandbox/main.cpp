#include "Engine.h"
#include <filesystem>

int main() {
    const auto fixtures_dir = std::filesystem::path("tests/fixtures");
    Engine engine(
        (fixtures_dir / "hits_sample.csv").string(),
        (fixtures_dir / "scheme.csv").string(),
        "sandbox_hits.hub"
    );
    return 0;
}
