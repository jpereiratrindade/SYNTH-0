#include <synth/controller/controller.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <thread>

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <toml++/toml.hpp>
#include <synth/contracts/types.hpp>

namespace synth::controller {
namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {
std::string iso_now() {
  const auto now = std::chrono::system_clock::now();
  const auto t = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
  gmtime_r(&t, &tm);
  char buf[32]{};
  std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
  return buf;
}

std::string slurp(const fs::path& p) {
  std::ifstream in(p, std::ios::binary);
  if (!in) throw std::runtime_error("cannot read " + p.string());
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

std::string sha256_hex(const std::string& input) {
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) throw std::runtime_error("EVP_MD_CTX_new failed");
  std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
  unsigned int len = 0;
  if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1 ||
      EVP_DigestUpdate(ctx, input.data(), input.size()) != 1 ||
      EVP_DigestFinal_ex(ctx, digest.data(), &len) != 1) {
    EVP_MD_CTX_free(ctx);
    throw std::runtime_error("SHA-256 failed");
  }
  EVP_MD_CTX_free(ctx);
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  for (unsigned int i = 0; i < len; ++i) out << std::setw(2) << static_cast<int>(digest[i]);
  return out.str();
}

std::string normalize_text(std::string value) {
  std::string out;
  out.reserve(value.size());
  for (std::size_t i = 0; i < value.size(); ++i) {
    if (value[i] == '\r' && i + 1 < value.size() && value[i + 1] == '\n') continue;
    out.push_back(value[i]);
  }
  return out;
}



std::string capture_command(const std::string& command) {
  std::array<char, 256> buffer{};
  std::string output;
  FILE* pipe = popen(command.c_str(), "r");
  if (!pipe) return {};
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) output += buffer.data();
  pclose(pipe);
  while (!output.empty() && (output.back() == '\n' || output.back() == '\r' || output.back() == ' ' || output.back() == '\t')) output.pop_back();
  return output;
}

json process_metrics(const std::string& unit, std::uint64_t max_memory_mb, std::uint64_t target_cpu_percent) {
  json out = {
    {"pid", 0},
    {"rss_kb", 0},
    {"cpu_ticks", 0},
    {"max_memory_mb", max_memory_mb},
    {"target_cpu_percent", target_cpu_percent},
    {"memory_budget_ok", true}
  };
  const auto pid_text = capture_command("systemctl --user show --property MainPID --value " + unit + " 2>/dev/null");
  if (pid_text.empty()) return out;
  long pid = 0;
  try { pid = std::stol(pid_text); } catch (...) { return out; }
  if (pid <= 0) return out;
  out["pid"] = pid;

  const fs::path status_path = fs::path{"/proc"} / std::to_string(pid) / "status";
  if (fs::exists(status_path)) {
    std::istringstream input(slurp(status_path));
    std::string key;
    while (input >> key) {
      if (key == "VmRSS:") {
        std::uint64_t rss_kb = 0;
        input >> rss_kb;
        out["rss_kb"] = rss_kb;
        out["memory_budget_ok"] = rss_kb <= max_memory_mb * 1024ULL;
        break;
      }
      std::string rest;
      std::getline(input, rest);
    }
  }

  const fs::path stat_path = fs::path{"/proc"} / std::to_string(pid) / "stat";
  if (fs::exists(stat_path)) {
    const auto raw = slurp(stat_path);
    const auto close = raw.rfind(')');
    if (close != std::string::npos && close + 2 < raw.size()) {
      std::istringstream fields(raw.substr(close + 2));
      std::string token;
      std::uint64_t user_ticks = 0, system_ticks = 0;
      // After comm, field 3 is first. utime/stime are fields 14/15 -> offsets 11/12 here.
      for (int field = 3; field <= 15 && fields >> token; ++field) {
        if (field == 14) user_ticks = static_cast<std::uint64_t>(std::stoull(token));
        if (field == 15) system_ticks = static_cast<std::uint64_t>(std::stoull(token));
      }
      out["cpu_ticks"] = user_ticks + system_ticks;
    }
  }
  return out;
}

json get_json(const std::string& host, int port, const std::string& path, bool& ok) {
  try {
    httplib::Client client(host, port);
    client.set_connection_timeout(0, 250000);
    client.set_read_timeout(0, 750000);
    const auto response = client.Get(path);
    if (!response) {
      ok = false;
      return json{{"error", "unreachable"}};
    }
    ok = response->status >= 200 && response->status < 300;
    auto body = json::parse(response->body, nullptr, false);
    if (body.is_discarded()) return json{{"error", "invalid_json"}, {"http_status", response->status}};
    body["http_status"] = response->status;
    return body;
  } catch (const std::exception& e) {
    ok = false;
    return json{{"error", e.what()}};
  }
}
}

Controller::Controller(fs::path root) : root_(std::move(root)) {}

std::vector<SystemDeclaration> Controller::systems() const {
  std::vector<SystemDeclaration> result;
  const auto dir = root_ / "declarations/systems";
  std::vector<fs::path> paths;
  for (const auto& entry : fs::directory_iterator(dir)) if (entry.path().extension() == ".toml") paths.push_back(entry.path());
  std::sort(paths.begin(), paths.end());
  for (const auto& path : paths) {
    const auto table = toml::parse_file(path.string());
    const auto* system = table["system"].as_table();
    const auto* resources = table["resources"].as_table();
    if (!system || !resources) throw std::runtime_error("invalid system declaration: " + path.string());
    result.push_back({
      system->get_as<std::string>("id")->get(),
      system->get_as<std::string>("authority")->get(),
      system->get_as<std::string>("host")->get(),
      static_cast<int>(system->get_as<std::int64_t>("port")->get()),
      system->get_as<std::string>("unit")->get(),
      system->get_as<bool>("required")->get(),
      static_cast<std::uint64_t>(resources->get_as<std::int64_t>("max_memory_mb")->get()),
      static_cast<std::uint64_t>(resources->get_as<std::int64_t>("max_queue_items")->get()),
      static_cast<std::uint64_t>(resources->get_as<std::int64_t>("target_cpu_percent")->get())
    });
  }
  return result;
}

std::vector<RelationDeclaration> Controller::relations() const {
  std::vector<RelationDeclaration> result;
  const auto dir = root_ / "declarations/relations";
  std::vector<fs::path> paths;
  for (const auto& entry : fs::directory_iterator(dir)) if (entry.path().extension() == ".toml") paths.push_back(entry.path());
  std::sort(paths.begin(), paths.end());
  for (const auto& path : paths) {
    const auto table = toml::parse_file(path.string());
    const auto* relation = table["relation"].as_table();
    if (!relation) throw std::runtime_error("invalid relation declaration: " + path.string());
    result.push_back({
      relation->get_as<std::string>("id")->get(),
      relation->get_as<bool>("required")->get(),
      relation->get_as<std::string>("observer")->get(),
      relation->get_as<std::string>("probe_path")->get(),
      relation->get_as<std::string>("requires_contract")->get()
    });
  }
  return result;
}

std::string Controller::declaration_digest() const {
  const auto dir = root_ / "declarations";
  std::vector<fs::path> paths;
  for (const auto& entry : fs::recursive_directory_iterator(dir)) if (entry.is_regular_file()) paths.push_back(entry.path());
  std::sort(paths.begin(), paths.end());
  std::string canonical;
  for (const auto& path : paths) {
    const auto relative = fs::relative(path, dir).generic_string();
    canonical += "--- " + relative + "\n";
    canonical += normalize_text(slurp(path));
    if (canonical.empty() || canonical.back() != '\n') canonical.push_back('\n');
  }
  return sha256_hex(canonical);
}

fs::path Controller::evidence_path() const { return root_ / ".evidence/latest.json"; }
fs::path Controller::state_digest_path() const { return root_ / ".state/generated/declaration.sha256"; }

std::string Controller::applied_digest() const {
  const auto path = state_digest_path();
  if (!fs::exists(path)) return {};
  auto value = slurp(path);
  value.erase(std::remove(value.begin(), value.end(), '\n'), value.end());
  return value;
}

void Controller::write_applied_digest(const std::string& digest) const {
  fs::create_directories(state_digest_path().parent_path());
  std::ofstream out(state_digest_path(), std::ios::trunc);
  out << digest << '\n';
}

json Controller::validate() const {
  json out = {
    {"schema", "synth.validation/1.0.0"},
    {"status", "PASS"},
    {"checked_at", iso_now()},
    {"errors", json::array()}
  };
  try {
    const auto ss = systems();
    const auto rs = relations();
    if (ss.size() != 3) out["errors"].push_back("expected exactly 3 synthetic systems");
    if (rs.size() != 3) out["errors"].push_back("expected exactly 3 declared relations");
    std::vector<std::string> authorities;
    std::vector<int> ports;
    for (const auto& s : ss) {
      if (s.authority.empty()) out["errors"].push_back("system " + s.id + " has empty authority");
      if (s.max_memory_mb == 0 || s.max_queue_items == 0) out["errors"].push_back("system " + s.id + " has an unbounded/zero resource declaration");
      authorities.push_back(s.authority);
      ports.push_back(s.port);
    }
    std::sort(authorities.begin(), authorities.end());
    if (std::adjacent_find(authorities.begin(), authorities.end()) != authorities.end()) out["errors"].push_back("duplicate authority");
    std::sort(ports.begin(), ports.end());
    if (std::adjacent_find(ports.begin(), ports.end()) != ports.end()) out["errors"].push_back("duplicate port");
    for (const auto& r : rs) {
      const auto found = std::find_if(ss.begin(), ss.end(), [&](const auto& s){ return s.id == r.observer; });
      if (found == ss.end()) out["errors"].push_back("relation " + r.id + " references unknown observer " + r.observer);
    }
    out["declaration_digest"] = declaration_digest();
  } catch (const std::exception& e) {
    out["errors"].push_back(e.what());
  }
  if (!out["errors"].empty()) out["status"] = "FAIL";
  return out;
}

json Controller::observe(bool persist) const {
  const auto ss = systems();
  const auto rs = relations();
  const auto digest = declaration_digest();
  json evidence = {
    {"schema", std::string(synth::contracts::evidence_contract)},
    {"observed_at", iso_now()},
    {"epistemic_class", "OBSERVED"},
    {"declaration_digest", digest},
    {"systems", json::array()},
    {"relations", json::array()}
  };

  for (const auto& s : ss) {
    bool ok = false;
    auto body = get_json(s.host, s.port, "/health", ok);
    auto resources = process_metrics(s.unit, s.max_memory_mb, s.target_cpu_percent);
    evidence["systems"].push_back({
      {"id", s.id}, {"authority", s.authority}, {"unit", s.unit},
      {"required", s.required}, {"reachable", ok},
      {"resource_ok", resources.value("memory_budget_ok", true)},
      {"resources", resources}, {"witness", body}
    });
  }

  for (const auto& r : rs) {
    const auto observer = std::find_if(ss.begin(), ss.end(), [&](const auto& s){ return s.id == r.observer; });
    bool ok = false;
    json body = {{"error", "observer_missing"}};
    if (observer != ss.end()) body = get_json(observer->host, observer->port, r.probe_path, ok);
    const auto observed_contract = body.value("observed_contract", body.value("schema", std::string{}));
    const bool contract_ok = observed_contract.empty() ? ok : observed_contract == r.requires_contract;
    evidence["relations"].push_back({
      {"id", r.id}, {"observer", r.observer}, {"required", r.required},
      {"reachable", ok}, {"contract_ok", contract_ok},
      {"requires_contract", r.requires_contract}, {"witness", body}
    });
  }

  if (persist) {
    fs::create_directories(evidence_path().parent_path());
    std::ofstream out(evidence_path(), std::ios::trunc);
    out << evidence.dump(2) << '\n';
  }
  return evidence;
}

json Controller::plan(const json& evidence) const {
  json out = {
    {"schema", std::string(synth::contracts::plan_contract)},
    {"action", "KEEP"},
    {"reason", "declaration and observation are coherent"},
    {"targets", json::array()},
    {"evidence", fs::relative(evidence_path(), root_).generic_string()},
    {"planned_at", iso_now()}
  };

  const auto desired = evidence.at("declaration_digest").get<std::string>();
  const auto applied = applied_digest();
  if (applied != desired) {
    out["action"] = "RECONFIGURE";
    out["reason"] = applied.empty() ? "no realized declaration digest" : "declared configuration changed";
    for (const auto& s : systems()) out["targets"].push_back(s.id);
    return out;
  }

  for (const auto& system : evidence.at("systems")) {
    if (system.at("required").get<bool>() && !system.at("reachable").get<bool>()) out["targets"].push_back(system.at("id"));
  }
  if (!out["targets"].empty()) {
    out["action"] = "REPAIR";
    out["reason"] = "one or more required systems are factually unreachable";
    return out;
  }

  for (const auto& system : evidence.at("systems")) {
    if (system.at("required").get<bool>() && !system.value("resource_ok", true)) {
      out["action"] = "DEGRADED";
      out["reason"] = "required system exceeded a declared runtime resource budget";
      out["targets"].push_back(system.at("id"));
      return out;
    }
  }

  for (const auto& relation : evidence.at("relations")) {
    if (relation.at("required").get<bool>() && !relation.at("contract_ok").get<bool>()) {
      out["action"] = "BLOCKED";
      out["reason"] = "required relation has incompatible contract";
      out["targets"].push_back(relation.at("id"));
      return out;
    }
  }
  for (const auto& relation : evidence.at("relations")) {
    if (relation.at("required").get<bool>() && !relation.at("reachable").get<bool>()) {
      out["action"] = "DEGRADED";
      out["reason"] = "systems are alive but a required relation is not satisfied";
      out["targets"].push_back(relation.at("id"));
      return out;
    }
  }
  return out;
}

int Controller::run_command(const std::string& command) {
  const int status = std::system(command.c_str());
  if (status == -1) return 127;
  return status;
}

json Controller::apply(const json& p) const {
  const auto action = p.at("action").get<std::string>();
  json out = {{"action", action}, {"status", "NO_OP"}, {"commands", json::array()}};
  if (action == "KEEP") return out;
  if (action == "BLOCKED" || action == "DEGRADED") {
    out["status"] = "NOT_APPLIED";
    return out;
  }

  const auto ss = systems();
  std::vector<std::string> targets;
  if (action == "RECONFIGURE") {
    for (const auto& s : ss) targets.push_back(s.id);
  } else {
    for (const auto& t : p.at("targets")) targets.push_back(t.get<std::string>());
  }

  bool ok = true;
  for (const auto& id : targets) {
    const auto it = std::find_if(ss.begin(), ss.end(), [&](const auto& s){ return s.id == id; });
    if (it == ss.end()) continue;
    const std::string verb = action == "RECONFIGURE" ? "restart" : "start";
    const std::string command = "systemctl --user " + verb + " " + it->unit;
    out["commands"].push_back(command);
    if (run_command(command) != 0) ok = false;
  }
  if (ok && action == "RECONFIGURE") write_applied_digest(declaration_digest());
  out["status"] = ok ? "APPLIED" : "FAILED";
  return out;
}

json Controller::verify() const {
  const auto evidence = observe(true);
  const auto p = plan(evidence);
  return {
    {"schema", "synth.verify/1.0.0"},
    {"status", p.at("action") == "KEEP" ? "PASS" : "FAIL"},
    {"plan", p},
    {"verified_at", iso_now()}
  };
}

json Controller::run() const {
  json report = {
    {"schema", "synth.lifecycle/1.0.0"},
    {"started_at", iso_now()},
    {"stages", json::array()}
  };
  const auto validation = validate();
  report["stages"].push_back({{"stage", "VALIDATE"}, {"result", validation}});
  if (validation.at("status") != "PASS") {
    report["status"] = "BLOCKED";
    return report;
  }

  const auto first_evidence = observe(true);
  report["stages"].push_back({{"stage", "OBSERVE"}, {"result", first_evidence}});
  const auto first_plan = plan(first_evidence);
  report["stages"].push_back({{"stage", "PLAN"}, {"result", first_plan}});
  const bool no_op = first_plan.at("action") == "KEEP";

  if (!no_op) {
    const auto applied = apply(first_plan);
    report["stages"].push_back({{"stage", "APPLY"}, {"result", applied}});
    if (applied.at("status") == "FAILED" || applied.at("status") == "NOT_APPLIED") {
      report["status"] = first_plan.at("action");
      return report;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
  }

  const auto second_evidence = observe(true);
  report["stages"].push_back({{"stage", "OBSERVE_AGAIN"}, {"result", second_evidence}});
  const auto second_plan = plan(second_evidence);
  report["stages"].push_back({{"stage", "PLAN_AGAIN"}, {"result", second_plan}});
  const auto verification = verify();
  report["stages"].push_back({{"stage", "VERIFY"}, {"result", verification}});
  report["status"] = verification.at("status") == "PASS" ? "READY" : "FAILED";
  report["no_op"] = no_op && second_plan.at("action") == "KEEP";
  report["completed_at"] = iso_now();
  return report;
}

json Controller::disturb_kill(const std::string& system_id) const {
  const auto ss = systems();
  const auto it = std::find_if(ss.begin(), ss.end(), [&](const auto& s){ return s.id == system_id; });
  if (it == ss.end()) return {{"status", "ERROR"}, {"error", "unknown system"}};
  const auto command = "systemctl --user kill -s SIGKILL " + it->unit;
  const int status = run_command(command);
  return {
    {"status", status == 0 ? "DISTURBED" : "ERROR"},
    {"system", system_id},
    {"command", command},
    {"at", iso_now()}
  };
}

json Controller::latest_evidence() const {
  if (!fs::exists(evidence_path())) return {{"status", "ABSENT"}};
  const auto parsed = json::parse(slurp(evidence_path()), nullptr, false);
  if (parsed.is_discarded()) return {{"status", "INVALID"}};
  return parsed;
}

int Controller::serve(const std::string& host, int port) const {
  httplib::Server server;
  server.Get("/api/v1/ecosystem", [&](const httplib::Request&, httplib::Response& res) {
    const auto evidence = observe(true);
    res.set_content(json{{"evidence", evidence}, {"plan", plan(evidence)}}.dump(), "application/json");
  });
  server.Get("/api/v1/evidence", [&](const httplib::Request&, httplib::Response& res) {
    res.set_content(latest_evidence().dump(), "application/json");
  });
  server.Post(R"(/api/v1/disturb/kill/([a-z0-9_-]+))", [&](const httplib::Request& req, httplib::Response& res) {
    const auto result = disturb_kill(req.matches[1]);
    if (result.value("status", "ERROR") == "ERROR") res.status = 400;
    res.set_content(result.dump(), "application/json");
  });
  server.Get("/events", [&](const httplib::Request&, httplib::Response& res) {
    res.set_header("Cache-Control", "no-cache");
    res.set_header("X-Accel-Buffering", "no");
    res.set_chunked_content_provider(
      "text/event-stream",
      [&](std::size_t, httplib::DataSink& sink) {
        const auto evidence = observe(true);
        const auto payload = std::string("event: ecosystem\ndata: ") + json{{"evidence", evidence}, {"plan", plan(evidence)}}.dump() + "\n\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return sink.write(payload.data(), payload.size());
      },
      [](bool) {}
    );
  });

  const auto web_dist = root_ / "web/dist";
  if (fs::exists(web_dist)) server.set_mount_point("/", web_dist.string());
  else {
    server.Get("/", [&](const httplib::Request&, httplib::Response& res) {
      res.set_content("SYNTH-0 controller is ready; build web/ for Living Topology.\n", "text/plain");
    });
  }

  std::cout << "SYNTH-0 controller serving on http://" << host << ':' << port << '\n';
  return server.listen(host, port) ? 0 : 1;
}

} // namespace synth::controller
