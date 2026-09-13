#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

namespace synth::contracts {

inline constexpr std::string_view signal_contract = "synth.contract.signal/1.0.0";
inline constexpr std::string_view pattern_contract = "synth.contract.pattern/1.0.0";
inline constexpr std::string_view intent_contract = "synth.contract.intent/1.0.0";
inline constexpr std::string_view health_contract = "synth.contract.health/1.0.0";
inline constexpr std::string_view evidence_contract = "synth.evidence/1.0.0";
inline constexpr std::string_view plan_contract = "synth.plan/1.0.0";

struct Ref {
  std::string authority;
  std::string kind;
  std::string id;
  std::string version;
};

inline void to_json(nlohmann::json& j, const Ref& r) {
  j = {
    {"authority", r.authority},
    {"kind", r.kind},
    {"id", r.id},
    {"version", r.version}
  };
}

} // namespace synth::contracts
