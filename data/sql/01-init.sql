-- SWG:ANH Modern Database Initialization
-- This script sets up the basic database structure for development

-- Enable necessary PostgreSQL extensions
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
CREATE EXTENSION IF NOT EXISTS "pgcrypto";

-- Create schemas for organization
CREATE SCHEMA IF NOT EXISTS account;
CREATE SCHEMA IF NOT EXISTS game;
CREATE SCHEMA IF NOT EXISTS world;
CREATE SCHEMA IF NOT EXISTS admin;

-- Account Management Tables
CREATE TABLE IF NOT EXISTS account.accounts (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    username VARCHAR(32) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    status INTEGER DEFAULT 1, -- 0=banned, 1=active, 2=suspended
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    last_login TIMESTAMP WITH TIME ZONE,
    login_count INTEGER DEFAULT 0
);

CREATE TABLE IF NOT EXISTS account.sessions (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    account_id UUID REFERENCES account.accounts(id) ON DELETE CASCADE,
    session_token VARCHAR(255) UNIQUE NOT NULL,
    ip_address INET,
    user_agent TEXT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    expires_at TIMESTAMP WITH TIME ZONE NOT NULL,
    is_active BOOLEAN DEFAULT TRUE
);

-- Character Management
CREATE TABLE IF NOT EXISTS game.characters (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    account_id UUID REFERENCES account.accounts(id) ON DELETE CASCADE,
    name VARCHAR(32) UNIQUE NOT NULL,
    species VARCHAR(32) NOT NULL,
    profession VARCHAR(32) NOT NULL,
    gender INTEGER NOT NULL, -- 0=male, 1=female
    appearance_data JSONB,
    position_x FLOAT DEFAULT 0,
    position_y FLOAT DEFAULT 0, 
    position_z FLOAT DEFAULT 0,
    orientation_x FLOAT DEFAULT 0,
    orientation_y FLOAT DEFAULT 0,
    orientation_z FLOAT DEFAULT 0,
    orientation_w FLOAT DEFAULT 1,
    planet VARCHAR(32) DEFAULT 'tatooine',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    last_played TIMESTAMP WITH TIME ZONE,
    play_time INTEGER DEFAULT 0 -- seconds
);

-- Game Objects (unified object system)
CREATE TABLE IF NOT EXISTS world.objects (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    object_id BIGINT UNIQUE NOT NULL, -- SWG object ID
    template_string VARCHAR(255) NOT NULL,
    type INTEGER NOT NULL, -- object type enum
    parent_id UUID REFERENCES world.objects(id),
    container_type INTEGER DEFAULT 0,
    position_x FLOAT DEFAULT 0,
    position_y FLOAT DEFAULT 0,
    position_z FLOAT DEFAULT 0,
    orientation_x FLOAT DEFAULT 0,
    orientation_y FLOAT DEFAULT 0,
    orientation_z FLOAT DEFAULT 0,
    orientation_w FLOAT DEFAULT 1,
    planet VARCHAR(32),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Object Attributes (key-value storage for flexibility)
CREATE TABLE IF NOT EXISTS world.object_attributes (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    object_id UUID REFERENCES world.objects(id) ON DELETE CASCADE,
    attribute_name VARCHAR(64) NOT NULL,
    attribute_value TEXT,
    data_type VARCHAR(16) DEFAULT 'string', -- string, int, float, bool, json
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    UNIQUE(object_id, attribute_name)
);

-- Server Configuration
CREATE TABLE IF NOT EXISTS admin.server_config (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    config_key VARCHAR(64) UNIQUE NOT NULL,
    config_value TEXT,
    description TEXT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Audit Log
CREATE TABLE IF NOT EXISTS admin.audit_log (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    event_type VARCHAR(32) NOT NULL,
    entity_type VARCHAR(32),
    entity_id UUID,
    user_id UUID,
    details JSONB,
    ip_address INET,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Indexes for performance
CREATE INDEX IF NOT EXISTS idx_accounts_username ON account.accounts(username);
CREATE INDEX IF NOT EXISTS idx_accounts_email ON account.accounts(email);
CREATE INDEX IF NOT EXISTS idx_sessions_token ON account.sessions(session_token);
CREATE INDEX IF NOT EXISTS idx_sessions_account ON account.sessions(account_id);
CREATE INDEX IF NOT EXISTS idx_characters_account ON game.characters(account_id);
CREATE INDEX IF NOT EXISTS idx_characters_name ON game.characters(name);
CREATE INDEX IF NOT EXISTS idx_objects_object_id ON world.objects(object_id);
CREATE INDEX IF NOT EXISTS idx_objects_parent ON world.objects(parent_id);
CREATE INDEX IF NOT EXISTS idx_objects_planet ON world.objects(planet);
CREATE INDEX IF NOT EXISTS idx_attributes_object ON world.object_attributes(object_id);
CREATE INDEX IF NOT EXISTS idx_attributes_name ON world.object_attributes(attribute_name);

-- Insert default server configuration
INSERT INTO admin.server_config (config_key, config_value, description) VALUES
('server.name', 'SWG:ANH Modern Dev', 'Server display name'),
('server.version', '14.1.0', 'Target SWG client version'),
('server.max_players', '100', 'Maximum concurrent players'),
('server.galaxy_name', 'Ahazi', 'Galaxy name'),
('login.port', '44453', 'Login server port'),
('zone.port', '44463', 'Zone server port'),
('features.jedi_enabled', 'false', 'Enable Jedi profession'),
('features.vehicle_enabled', 'true', 'Enable vehicles'),
('features.space_enabled', 'false', 'Enable space content')
ON CONFLICT (config_key) DO NOTHING;

-- Create a test account for development
INSERT INTO account.accounts (username, password_hash, email) VALUES
('admin', crypt('admin123', gen_salt('bf')), 'admin@swganh.dev'),
('testuser', crypt('test123', gen_salt('bf')), 'test@swganh.dev')
ON CONFLICT (username) DO NOTHING;