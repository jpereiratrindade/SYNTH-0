#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace synth::controller {

struct SystemDeclaration {
  std::string id;
  std::string authority;
  std::string host;
  int port{};
  std::string unit;
  bool required{};
  std::uint64_t max_memory_mb{};
  std::uint64_t max_queue_items{};
  std::uint64_t target_cpu_percent{};
};

struct RelationDeclaration {
  std::string id;
  bool required{};
  std::string observer;
  std::string probe_path;
  std::string requires_contract;
};

class Controller {
 public:
  explicit Controller(std::filesystem::path root);

  nlohmann::json validate() const;
  nlohmann::json observe(bool persist = true) const;
  nlohmann::json plan(const nlohmann::json& evidence) const;
  nlohmann::json apply(const nlohmann::json& plan) const;
  nlohmann::json verify() const;
  nlohmann::json run() const;
  nlohmann::json disturb_kill(const std::string& system_id) const;
  nlohmann::json latest_evidence() const;
  int serve(const std::string& host, int port) const;

 private:
  std::filesystem::path root_;
  std::vector<SystemDeclaration> systems() const;
  std::vector<RelationDeclaration> relations() const;
  std::string declaration_digest() const;
  std::string applied_digest() const;
  void write_applied_digest(const std::string& digest) const;
  std::filesystem::path evidence_path() const;
  std::filesystem::path state_digest_path() const;
  static int run_command(const std::string& command);
};

} // namespace synth::controller
