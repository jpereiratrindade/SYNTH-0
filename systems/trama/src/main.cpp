#include <algorithm>
#include <chrono>
#include <ctime>
#include <deque>
#include <iostream>
#include <mutex>
#include <numeric>
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

class Window {
 public:
  explicit Window(std::size_t capacity) : capacity_(std::max<std::size_t>(1, capacity)) {}
  double push(double v) {
    std::scoped_lock lock(mutex_);
    if (values_.size() == capacity_) values_.pop_front();
    values_.push_back(v);
    return std::accumulate(values_.begin(), values_.end(), 0.0) / static_cast<double>(values_.size());
  }
  std::size_t size() const {
    std::scoped_lock lock(mutex_);
    return values_.size();
  }
 private:
  std::size_t capacity_;
  mutable std::mutex mutex_;
  std::deque<double> values_;
};

json fetch_pattern(Window& window) {
  httplib::Client upstream("127.0.0.1", 9201);
  upstream.set_connection_timeout(0, 250000);
  upstream.set_read_timeout(0, 500000);
  const auto response = upstream.Get("/api/v1/signal");
  if (!response || response->status != 200) {
    return {
      {"schema", std::string(synth::contracts::pattern_contract)},
      {"authority", "synth.trama"},
      {"status", "degraded"},
      {"reason", "pulso_unreachable"},
      {"observed_at", iso_now()}
    };
  }
  const auto signal = json::parse(response->body);
  const auto value = signal.at("value").get<double>();
  const auto mean = window.push(value);
  return {
    {"schema", std::string(synth::contracts::pattern_contract)},
    {"authority", "synth.trama"},
    {"status", "ready"},
    {"epistemic_class", "DERIVED"},
    {"source", signal.at("ref")},
    {"window_size", window.size()},
    {"mean", mean},
    {"observed_at", iso_now()}
  };
}
}

int main() {
  constexpr int port = 9202;
  Window window{32};
  httplib::Server server;

  server.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
    res.set_content(json{
      {"schema", std::string(synth::contracts::health_contract)},
      {"authority", "synth.trama"},
      {"system", "trama"},
      {"status", "ready"},
      {"observed_at", iso_now()},
      {"queue_depth", window.size()},
      {"queue_capacity", 32}
    }.dump(), "application/json");
  });

  server.Get("/api/v1/pattern", [&](const httplib::Request&, httplib::Response& res) {
    const auto body = fetch_pattern(window);
    if (body.value("status", "degraded") != "ready") res.status = 503;
    res.set_content(body.dump(), "application/json");
  });

  server.Get("/api/v1/relations/pulso", [&](const httplib::Request&, httplib::Response& res) {
    httplib::Client upstream("127.0.0.1", 9201);
    upstream.set_connection_timeout(0, 250000);
    upstream.set_read_timeout(0, 500000);
    const auto response = upstream.Get("/health");
    const bool ok = response && response->status == 200;
    json body = {
      {"relation", "pulso-to-trama"},
      {"observer", "synth.trama"},
      {"status", ok ? "ready" : "degraded"},
      {"requires_contract", std::string(synth::contracts::signal_contract)},
      {"observed_contract", std::string(synth::contracts::signal_contract)},
      {"observed_at", iso_now()}
    };
    if (!ok) res.status = 503;
    res.set_content(body.dump(), "application/json");
  });

  std::cout << "synth.trama listening on 127.0.0.1:" << port << '\n';
  if (!server.listen("127.0.0.1", port)) {
    std::cerr << "trama failed to bind port " << port << '\n';
    return 1;
  }
}
