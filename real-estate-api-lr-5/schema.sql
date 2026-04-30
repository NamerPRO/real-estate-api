-- ============================================================================
-- Schema: Real Estate Database
-- Description: Система управления недвижимостью (аналог Zillow)
-- Database: PostgreSQL 16
-- ============================================================================

DROP TABLE IF EXISTS users CASCADE;

-- ============================================================================
-- Table: users
-- Description: Пользователи системы (владельцы и покупатели)
-- ============================================================================
CREATE TABLE IF NOT EXISTS users (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    login VARCHAR(50) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    first_name VARCHAR(100) NOT NULL,
    last_name VARCHAR(100) NOT NULL,
    email VARCHAR(255) NOT NULL UNIQUE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW() NOT NULL,
    
    -- Ограничения на данные
    CONSTRAINT chk_users_login_length CHECK (LENGTH(login) >= 3 AND LENGTH(login) <= 50),
    CONSTRAINT chk_users_password_length CHECK (LENGTH(password_hash) >= 6)
);

COMMENT ON TABLE users IS 'Пользователи системы (владельцы и покупатели недвижимости)';
COMMENT ON COLUMN users.id IS 'Уникальный идентификатор пользователя';
COMMENT ON COLUMN users.login IS 'Уникальный логин для входа (3-50 символов)';
COMMENT ON COLUMN users.password_hash IS 'Хэш пароля пользователя';
COMMENT ON COLUMN users.first_name IS 'Имя пользователя';
COMMENT ON COLUMN users.last_name IS 'Фамилия пользователя';
COMMENT ON COLUMN users.email IS 'Уникальный email адрес';
COMMENT ON COLUMN users.created_at IS 'Дата и время регистрации пользователя';

-- ============================================================================
-- Создание индексов
-- ============================================================================

CREATE UNIQUE INDEX idx_users_login ON users(login);

CREATE EXTENSION IF NOT EXISTS pg_trgm;
CREATE INDEX idx_users_first_trgm ON users USING gin (first_name gin_trgm_ops);
CREATE INDEX idx_users_last_trgm ON users USING gin (last_name gin_trgm_ops);