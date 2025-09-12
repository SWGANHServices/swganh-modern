// File: src/core/account_manager.hpp
#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include "types.hpp"

namespace swganh {

struct Account {
    u32 account_id;
    std::string username;
    std::string password;
    bool is_active;
    std::string created_date;
    u32 login_count;
    
    Account(u32 id, const std::string& user, const std::string& pass) 
        : account_id(id), username(user), password(pass), is_active(true), 
          created_date("2025-09-12"), login_count(0) {}
};

enum class LoginResult {
    SUCCESS = 0,
    INVALID_CREDENTIALS = 1,
    ACCOUNT_DISABLED = 2,
    SERVER_FULL = 3,
    MAINTENANCE = 4
};

class AccountManager {
public:
    static AccountManager& instance() {
        static AccountManager inst;
        return inst;
    }
    
    LoginResult authenticate(const std::string& username, const std::string& password) {
        auto it = accounts_.find(username);
        
        if (it != accounts_.end()) {
            // Account exists - check password
            if (it->second->password == password) {
                if (it->second->is_active) {
                    it->second->login_count++;
                    return LoginResult::SUCCESS;
                } else {
                    return LoginResult::ACCOUNT_DISABLED;
                }
            } else {
                return LoginResult::INVALID_CREDENTIALS;
            }
        } else {
            // Account doesn't exist - check if we should auto-create
            return try_auto_create_account(username, password);
        }
    }
    
    std::shared_ptr<Account> get_account(const std::string& username) {
        auto it = accounts_.find(username);
        return (it != accounts_.end()) ? it->second : nullptr;
    }
    
    void create_test_accounts() {
        // Create some test accounts for development
        create_account("test", "test");
        create_account("admin", "admin");
        create_account("dev", "dev");
    }
    
    size_t get_account_count() const {
        return accounts_.size();
    }

private:
    AccountManager() = default;
    
    u32 create_account(const std::string& username, const std::string& password) {
        u32 new_id = next_account_id_++;
        auto account = std::make_shared<Account>(new_id, username, password);
        accounts_[username] = account;
        return new_id;
    }
    
    LoginResult try_auto_create_account(const std::string& username, const std::string& password);
    
    std::unordered_map<std::string, std::shared_ptr<Account>> accounts_;
    u32 next_account_id_ = 1000;
};

} // namespace swganh