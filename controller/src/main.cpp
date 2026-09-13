#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include <nlohmann/json.hpp>
#include <synth/controller/controller.hpp>

#ifndef SYNTH_SOURCE_DIR
#define SYNTH_SOURCE_DIR "."
#endif

namespace {
void print_usage() {
  std::cerr << "usage: synth-controller <validate|observe|plan|apply|verify|status|evidence|run|disturb|serve>\n";
}
}

int main(int argc, char** argv) {
  if (argc < 2) { print_usage(); return 2; }
  synth::controller::Controller controller{std::filesystem::path{SYNTH_SOURCE_DIR}};
  const std::string cmd = argv[1];
  try {
    if (cmd == "validate") {
      const auto r = controller.validate(); std::cout << r.dump(2) << '\n'; return r.at("status") == "PASS" ? 0 : 1;
    }
    if (cmd == "observe") { std::cout << controller.observe(true).dump(2) << '\n'; return 0; }
    if (cmd == "plan") {
      const auto e = controller.observe(true); const auto p = controller.plan(e); std::cout << p.dump(2) << '\n'; return p.at("action") == "BLOCKED" ? 1 : 0;
    }
    if (cmd == "apply") {
      const auto e = controller.observe(true); const auto p = controller.plan(e); const auto a = controller.apply(p); std::cout << a.dump(2) << '\n'; return a.at("status") == "FAILED" ? 1 : 0;
    }
    if (cmd == "verify") {
      const auto r = controller.verify(); std::cout << r.dump(2) << '\n'; return r.at("status") == "PASS" ? 0 : 1;
    }
    if (cmd == "status") {
      const auto e = controller.observe(true); std::cout << nlohmann::json{{"evidence", e}, {"plan", controller.plan(e)}}.dump(2) << '\n'; return 0;
    }
    if (cmd == "evidence") { std::cout << controller.latest_evidence().dump(2) << '\n'; return 0; }
    if (cmd == "run") {
      if (argc > 2 && (argc != 4 || std::string(argv[2]) != "--target" || std::string(argv[3]) != "dev")) {
        std::cerr << "only --target dev exists in SYNTH-BOOTSTRAP-001\n";
        return 2;
      }
      const auto r = controller.run(); std::cout << r.dump(2) << '\n'; return r.at("status") == "READY" ? 0 : 1;
    }
    if (cmd == "disturb") {
      if (argc != 4 || std::string(argv[2]) != "kill") { print_usage(); return 2; }
      const auto r = controller.disturb_kill(argv[3]); std::cout << r.dump(2) << '\n'; return r.at("status") == "DISTURBED" ? 0 : 1;
    }
    if (cmd == "serve") return controller.serve("127.0.0.1", 9180);
    print_usage();
    return 2;
  } catch (const std::exception& e) {
    std::cerr << "synth-controller: " << e.what() << '\n';
    return 1;
  }
}
