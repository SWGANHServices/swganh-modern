// File: src/core/account_manager.cpp
#include "account_manager.hpp"
#include "config.hpp"
#include "logger.hpp"

namespace swganh {

LoginResult AccountManager::try_auto_create_account(const std::string& username, const std::string& password) {
    Config& config = Config::instance();
    
    if (config.get_bool("auto_create_accounts")) {
        LOG_INFO_F("Auto-creating account for user: {}", username);
        
        u32 new_id = create_account(username, password);
        
        LOG_INFO_F("Created account ID {} for user '{}' (development mode)", new_id, username);
        return LoginResult::SUCCESS;
        
    } else {
        LOG_WARNING_F("Login failed - account '{}' does not exist (production mode)", username);
        return LoginResult::INVALID_CREDENTIALS;
    }
}

} // namespace swganh