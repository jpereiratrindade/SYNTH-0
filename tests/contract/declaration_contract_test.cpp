#include <algorithm>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <toml++/toml.hpp>

#ifndef SYNTH_SOURCE_DIR
#define SYNTH_SOURCE_DIR "."
#endif

int main() {
  namespace fs = std::filesystem;
  const fs::path root = SYNTH_SOURCE_DIR;
  const fs::path dir = root / "declarations/systems";
  std::set<std::string> authorities;
  std::set<std::int64_t> ports;
  std::size_t count = 0;
  try {
    for (const auto& entry : fs::directory_iterator(dir)) {
      if (entry.path().extension() != ".toml") continue;
      const auto table = toml::parse_file(entry.path().string());
      const auto* system = table["system"].as_table();
      const auto* resources = table["resources"].as_table();
      if (!system || !resources) return 1;
      const auto authority = system->get_as<std::string>("authority")->get();
      const auto port = system->get_as<std::int64_t>("port")->get();
      const auto queue = resources->get_as<std::int64_t>("max_queue_items")->get();
      const auto memory = resources->get_as<std::int64_t>("max_memory_mb")->get();
      if (!authorities.insert(authority).second) return 2;
      if (!ports.insert(port).second) return 3;
      if (queue <= 0 || memory <= 0) return 4;
      ++count;
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 5;
  }
  return count == 3 ? 0 : 6;
}
