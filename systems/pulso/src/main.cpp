#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <iostream>
#include <mutex>
#include <string>

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <synth/contracts/types.hpp>

namespace {
using json = nlohmann::json;
using clock_type = std::chrono::system_clock;

std::string iso_now() {
  const auto now = clock_type::now();
  const auto t = clock_type::to_time_t(now);
  std::tm tm{};
  gmtime_r(&t, &tm);
  char buf[32]{};
  std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
  return buf;
}

struct Sample {
  std::uint64_t sequence{};
  double value{};
  std::string observed_at;
};

class BoundedHistory {
 public:
  explicit BoundedHistory(std::size_t capacity) : capacity_(std::max<std::size_t>(1, capacity)) {}
  void push(Sample value) {
    std::scoped_lock lock(mutex_);
    if (items_.size() == capacity_) items_.pop_front();
    items_.push_back(std::move(value));
  }
  std::size_t size() const {
    std::scoped_lock lock(mutex_);
    return items_.size();
  }
 private:
  std::size_t capacity_;
  mutable std::mutex mutex_;
  std::deque<Sample> items_;
};
}

int main() {
  constexpr int port = 9201;
  std::atomic<std::uint64_t> sequence{0};
  std::atomic<double> rate_hz{1.0};
  BoundedHistory history{4096};
  httplib::Server server;

  server.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
    json body = {
      {"schema", std::string(synth::contracts::health_contract)},
      {"authority", "synth.pulso"},
      {"system", "pulso"},
      {"status", "ready"},
      {"observed_at", iso_now()},
      {"queue_depth", history.size()},
      {"queue_capacity", 4096}
    };
    res.set_content(body.dump(), "application/json");
  });

  server.Get("/api/v1/signal", [&](const httplib::Request&, httplib::Response& res) {
    const auto seq = ++sequence;
    Sample sample{seq, std::sin(static_cast<double>(seq) / 10.0), iso_now()};
    history.push(sample);
    json body = {
      {"schema", std::string(synth::contracts::signal_contract)},
      {"epistemic_class", "OBSERVED"},
      {"ref", synth::contracts::Ref{"synth.pulso", "signal", "primary", "1"}},
      {"sequence", sample.sequence},
      {"value", sample.value},
      {"observed_at", sample.observed_at},
      {"rate_hz", rate_hz.load()}
    };
    res.set_content(body.dump(), "application/json");
  });

  server.Get("/api/v1/rate", [&](const httplib::Request&, httplib::Response& res) {
    json body = {
      {"authority", "synth.pulso"},
      {"kind", "rate"},
      {"rate_hz", rate_hz.load()},
      {"epistemic_class", "OBSERVED"}
    };
    res.set_content(body.dump(), "application/json");
  });

  server.Put("/api/v1/rate", [&](const httplib::Request& req, httplib::Response& res) {
    try {
      const auto body = json::parse(req.body);
      const auto requested = body.at("rate_hz").get<double>();
      if (!(requested > 0.0 && requested <= 100.0)) {
        res.status = 422;
        res.set_content(R"({"error":"rate_hz must be > 0 and <= 100"})", "application/json");
        return;
      }
      rate_hz.store(requested);
      res.set_content(json{{"status", "accepted"}, {"rate_hz", requested}}.dump(), "application/json");
    } catch (const std::exception& e) {
      res.status = 400;
      res.set_content(json{{"error", e.what()}}.dump(), "application/json");
    }
  });

  std::cout << "synth.pulso listening on 127.0.0.1:" << port << '\n';
  if (!server.listen("127.0.0.1", port)) {
    std::cerr << "pulso failed to bind port " << port << '\n';
    return 1;
  }
}
