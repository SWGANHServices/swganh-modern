// File: src/core/config.hpp
#pragma once

#include <string>
#include <unordered_map>

namespace swganh {

class Config {
public:
    static Config& instance() {
        static Config inst;
        return inst;
    }
    
    void load_defaults() {
        // Development settings
        settings_["auto_create_accounts"] = "true";
        settings_["default_password"] = "test";
        settings_["debug_login"] = "true";
        settings_["server_name"] = "SWG:ANH Modern Dev Server";
        settings_["server_population"] = "Light";
        
        // Network settings
        settings_["login_port"] = "44453";
        settings_["max_connections"] = "1000";
    }
    
    std::string get(const std::string& key, const std::string& default_value = "") const {
        auto it = settings_.find(key);
        return (it != settings_.end()) ? it->second : default_value;
    }
    
    bool get_bool(const std::string& key, bool default_value = false) const {
        std::string value = get(key);
        return value == "true" || value == "1" || value == "yes";
    }
    
    int get_int(const std::string& key, int default_value = 0) const {
        std::string value = get(key);
        return value.empty() ? default_value : std::stoi(value);
    }
    
    void set(const std::string& key, const std::string& value) {
        settings_[key] = value;
    }

private:
    Config() { load_defaults(); }
    std::unordered_map<std::string, std::string> settings_;
};

} // namespace swganh