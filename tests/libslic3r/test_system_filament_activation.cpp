#include "libslic3r/PresetBundle.hpp"

#include <stdexcept>
#include <iostream>
#ifndef NEO_STANDALONE_NATIVE_TEST
#include <catch2/catch_test_macros.hpp>
#endif

namespace {
void verify_system_filament_activation()
{
    using Slic3r::PresetBundle;
    using Slic3r::VendorProfile;
    using Vendors = std::map<std::string, std::map<std::string, std::set<std::string>>>;
    using Filaments = std::map<std::string, std::string>;
    PresetBundle bundle;
    bundle.vendors.emplace("A", VendorProfile("A"));
    bundle.vendors.at("A").default_filaments = {"Generic PLA vendor override"};
    bundle.vendors.emplace("B", VendorProfile("B"));
    bundle.vendors.at("B").default_filaments = {"Only B"};
    const auto metadata_before = bundle.vendors.at("A").default_filaments;
    const Vendors selected_a = {{"A", {{"printer", {"0.4"}}}}};
    auto check = [&](const char* label, const Vendors& selected, const Filaments& input, const Filaments& expected) {
        if (bundle.normalize_system_filament_activation(selected, input) != expected)
            throw std::runtime_error(label);
    };
    check("substring replacement preserves values and exact suffix", selected_a,
          {{"Generic PLA @System", "false"}, {"Generic PLA@System", "one"}, {"Generic PLA @System extra", "two"}, {" @System", "three"}},
          {{"Generic PLA", "false"}, {"Generic PLA@System", "one"}, {"Generic PLA @System extra", "two"}, {" @System", "three"}});
    check("no match stays unchanged", selected_a, {{"Unmatched @System", "true"}}, {{"Unmatched @System", "true"}});
    check("Custom only has no non-Custom printer", {{"Custom", {{"custom", {"0.4"}}}}},
          {{"Generic PLA @System", "true"}}, {{"Generic PLA @System", "true"}});
    check("empty non-Custom models do not trigger replacement", {{"A", {}}},
          {{"Generic PLA @System", "true"}}, {{"Generic PLA @System", "true"}});
    auto custom_and_a = selected_a;
    custom_and_a["Custom"] = {{"custom", {"0.4"}}};
    check("enabled Custom retains both names", custom_and_a, {{"Generic PLA @System", "enabled"}},
          {{"Generic PLA", "enabled"}, {"Generic PLA @System", "enabled"}});
    auto both = selected_a;
    both["B"] = {{"printer", {"0.4"}}};
    check("first loaded vendor stops search even without match", both, {{"Only B @System", "true"}}, {{"Only B @System", "true"}});
    auto a_unloaded = both;
    a_unloaded["0-Unloaded"] = {{"printer", {"0.4"}}};
    check("unloaded vendors are skipped before first loaded match", a_unloaded,
          {{"Generic PLA @System", "true"}}, {{"Generic PLA", "true"}});
    check("target vendor not already loaded stays unchanged", {{"NewVendor", {{"printer", {"0.4"}}}}},
          {{"Generic PLA @System", "true"}}, {{"Generic PLA @System", "true"}});
    bundle.vendors.at("A").default_filaments.clear();
    check("JSON-style empty metadata does not match model materials", selected_a,
          {{"Generic PLA @System", "true"}}, {{"Generic PLA @System", "true"}});
    bundle.vendors.at("A").default_filaments = metadata_before;
    check("repeat uses same metadata without mutation", selected_a,
          {{"Generic PLA @System", "true"}}, {{"Generic PLA", "true"}});
    if (bundle.vendors.at("A").default_filaments != metadata_before)
        throw std::runtime_error("normalization mutated metadata");
}
}

#ifdef NEO_STANDALONE_NATIVE_TEST
int main()
{
    try {
        verify_system_filament_activation();
        std::cout << "system filament activation native rule PASS (10 cases)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
#else
TEST_CASE("System filament activation follows loaded vendor metadata", "[PresetBundle]")
{
    REQUIRE_NOTHROW(verify_system_filament_activation());
}
#endif
