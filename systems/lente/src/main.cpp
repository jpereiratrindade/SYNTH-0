#include <chrono>
#include <ctime>
#include <iostream>
#include <string>

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <synth/contracts/types.hpp>

namespace {
using json = nlohmann::json;

std::string iso_now() {
  const auto now = std::chrono::system_clock::now();
  const auto t = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
  gmtime_r(&t, &tm);
  char buf[32]{};
  std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
  return buf;
}

json fetch_pattern() {
  httplib::Client upstream("127.0.0.1", 9202);
  upstream.set_connection_timeout(0, 250000);
  upstream.set_read_timeout(0, 750000);
  const auto response = upstream.Get("/api/v1/pattern");
  if (!response || response->status != 200) return {};
  return json::parse(response->body);
}
}

int main() {
  constexpr int port = 9203;
  httplib::Server server;

  server.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
    res.set_content(json{
      {"schema", std::string(synth::contracts::health_contract)},
      {"authority", "synth.lente"},
      {"system", "lente"},
      {"status", "ready"},
      {"observed_at", iso_now()}
    }.dump(), "application/json");
  });

  server.Get("/api/v1/relations/trama", [&](const httplib::Request&, httplib::Response& res) {
    httplib::Client upstream("127.0.0.1", 9202);
    upstream.set_connection_timeout(0, 250000);
    upstream.set_read_timeout(0, 500000);
    const auto response = upstream.Get("/health");
    const bool ok = response && response->status == 200;
    json body = {
      {"relation", "trama-to-lente"},
      {"observer", "synth.lente"},
      {"status", ok ? "ready" : "degraded"},
      {"requires_contract", std::string(synth::contracts::pattern_contract)},
      {"observed_contract", std::string(synth::contracts::pattern_contract)},
      {"observed_at", iso_now()}
    };
    if (!ok) res.status = 503;
    res.set_content(body.dump(), "application/json");
  });

  server.Get("/api/v1/projection", [&](const httplib::Request&, httplib::Response& res) {
    const auto pattern = fetch_pattern();
    if (pattern.empty()) {
      res.status = 503;
      res.set_content(json{{"authority", "synth.lente"}, {"status", "degraded"}, {"reason", "trama_unreachable"}}.dump(), "application/json");
      return;
    }
    const auto mean = pattern.at("mean").get<double>();
    res.set_content(json{
      {"authority", "synth.lente"},
      {"kind", "projection"},
      {"epistemic_class", "DERIVED"},
      {"source_contract", std::string(synth::contracts::pattern_contract)},
      {"mean", mean},
      {"band", mean > 0.25 ? "high" : (mean < -0.25 ? "low" : "nominal")},
      {"observed_at", iso_now()}
    }.dump(), "application/json");
  });

  server.Get("/api/v1/intent", [&](const httplib::Request&, httplib::Response& res) {
    const auto pattern = fetch_pattern();
    if (pattern.empty()) {
      res.status = 503;
      res.set_content(json{
        {"schema", std::string(synth::contracts::intent_contract)},
        {"authority", "synth.lente"},
        {"status", "degraded"},
        {"reason", "trama_unreachable"}
      }.dump(), "application/json");
      return;
    }
    const auto mean = pattern.at("mean").get<double>();
    const double proposed_rate = mean > 0.25 ? 0.5 : 1.0;
    res.set_content(json{
      {"schema", std::string(synth::contracts::intent_contract)},
      {"authority", "synth.lente"},
      {"status", "ready"},
      {"epistemic_class", "DERIVED"},
      {"target", synth::contracts::Ref{"synth.pulso", "rate", "primary", "1"}},
      {"proposal", {{"rate_hz", proposed_rate}}},
      {"applied", false},
      {"observed_at", iso_now()}
    }.dump(), "application/json");
  });

  std::cout << "synth.lente listening on 127.0.0.1:" << port << '\n';
  if (!server.listen("127.0.0.1", port)) {
    std::cerr << "lente failed to bind port " << port << '\n';
    return 1;
  }
}
