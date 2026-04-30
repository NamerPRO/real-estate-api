-- ============================================================================
-- Schema: Real Estate Database
-- Description: Система управления недвижимостью (аналог Zillow)
-- Database: PostgreSQL 16
-- ============================================================================

-- Удаляем таблицы в правильном порядке (из-за внешних ключей)
DROP TABLE IF EXISTS viewings CASCADE;
DROP TABLE IF EXISTS properties CASCADE;
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
-- Table: properties
-- Description: Объекты недвижимости (квартиры, дома, коммерческие помещения)
-- ============================================================================
CREATE TABLE IF NOT EXISTS properties (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    owner_id BIGINT NOT NULL,
    title VARCHAR(200) NOT NULL,
    description TEXT,
    city VARCHAR(100) NOT NULL,
    address TEXT,
    price DOUBLE PRECISION NOT NULL,
    status VARCHAR(20) DEFAULT 'active' NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW() NOT NULL,

    -- Внешний ключ на пользователей
    CONSTRAINT fk_properties_owner 
        FOREIGN KEY (owner_id) REFERENCES users(id) ON DELETE CASCADE,
    
    -- Ограничения на данные
    CONSTRAINT chk_properties_price CHECK (price >= 0),
    CONSTRAINT chk_properties_title_length CHECK (LENGTH(title) >= 1 AND LENGTH(title) <= 200),
    CONSTRAINT chk_properties_status CHECK (status IN ('active', 'sold', 'rented', 'archived'))
);

COMMENT ON TABLE properties IS 'Объекты недвижимости (квартиры, дома, коммерческие помещения)';
COMMENT ON COLUMN properties.id IS 'Уникальный идентификатор объекта';
COMMENT ON COLUMN properties.owner_id IS 'ID владельца объекта (ссылка на users.id)';
COMMENT ON COLUMN properties.title IS 'Заголовок объявления (1-200 символов)';
COMMENT ON COLUMN properties.description IS 'Подробное описание объекта';
COMMENT ON COLUMN properties.city IS 'Город расположения объекта';
COMMENT ON COLUMN properties.address IS 'Полный адрес объекта';
COMMENT ON COLUMN properties.price IS 'Стоимость объекта в рублях (NUMERIC для точности)';
COMMENT ON COLUMN properties.status IS 'Статус объекта: active, sold, rented, archived';
COMMENT ON COLUMN properties.created_at IS 'Дата создания объявления';

-- ============================================================================
-- Table: viewings
-- Description: Записи на просмотр объектов недвижимости
-- ============================================================================
CREATE TABLE IF NOT EXISTS viewings (
    id BIGINT GENERATED ALWAYS AS IDENTITY,
    property_id BIGINT NOT NULL,
    user_id BIGINT NOT NULL,
    scheduled_time TIMESTAMP WITH TIME ZONE NOT NULL,
    status VARCHAR(20) DEFAULT 'scheduled' NOT NULL,
    comment TEXT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW() NOT NULL,
    
    PRIMARY KEY (id, created_at),

    -- Внешние ключи
    CONSTRAINT fk_viewings_property 
        FOREIGN KEY (property_id) REFERENCES properties(id) ON DELETE CASCADE,
    CONSTRAINT fk_viewings_user 
        FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    
    -- Ограничения на данные
    CONSTRAINT chk_viewings_status CHECK (status IN ('scheduled', 'completed', 'cancelled')),
    CONSTRAINT chk_viewings_comment_length CHECK (comment IS NULL OR LENGTH(comment) <= 500)
) PARTITION BY RANGE (created_at);

-- 2025 год
CREATE TABLE viewings_2025_10 PARTITION OF viewings
    FOR VALUES FROM ('2025-10-01') TO ('2025-11-01');
CREATE TABLE viewings_2025_11 PARTITION OF viewings
    FOR VALUES FROM ('2025-11-01') TO ('2025-12-01');
CREATE TABLE viewings_2025_12 PARTITION OF viewings
    FOR VALUES FROM ('2025-12-01') TO ('2026-01-01');

-- 2026 год
CREATE TABLE viewings_2026_01 PARTITION OF viewings
    FOR VALUES FROM ('2026-01-01') TO ('2026-02-01');
CREATE TABLE viewings_2026_02 PARTITION OF viewings
    FOR VALUES FROM ('2026-02-01') TO ('2026-03-01');
CREATE TABLE viewings_2026_03 PARTITION OF viewings
    FOR VALUES FROM ('2026-03-01') TO ('2026-04-01');
CREATE TABLE viewings_2026_04 PARTITION OF viewings
    FOR VALUES FROM ('2026-04-01') TO ('2026-05-01');
CREATE TABLE viewings_2026_05 PARTITION OF viewings
    FOR VALUES FROM ('2026-05-01') TO ('2026-06-01');
CREATE TABLE viewings_2026_06 PARTITION OF viewings
    FOR VALUES FROM ('2026-06-01') TO ('2026-07-01');

-- Резервная партиция для данных вне диапазона (защита от ошибок)
CREATE TABLE viewings_default PARTITION OF viewings DEFAULT;

COMMENT ON TABLE viewings IS 'Записи на просмотр объектов недвижимости';
COMMENT ON COLUMN viewings.id IS 'Уникальный идентификатор записи на просмотр';
COMMENT ON COLUMN viewings.property_id IS 'ID объекта для просмотра';
COMMENT ON COLUMN viewings.user_id IS 'ID пользователя, который записался на просмотр';
COMMENT ON COLUMN viewings.scheduled_time IS 'Запланированное время просмотра';
COMMENT ON COLUMN viewings.status IS 'Статус просмотра: scheduled, completed, cancelled';
COMMENT ON COLUMN viewings.comment IS 'Комментарий к просмотру (до 500 символов)';

-- ============================================================================
-- Создание индексов
-- ============================================================================

CREATE UNIQUE INDEX idx_users_login ON users(login);

CREATE EXTENSION IF NOT EXISTS pg_trgm;
CREATE INDEX idx_users_first_trgm ON users USING gin (first_name gin_trgm_ops);
CREATE INDEX idx_users_last_trgm ON users USING gin (last_name gin_trgm_ops);

CREATE INDEX idx_properties_city_status_created 
ON properties(city, status, created_at DESC);

CREATE INDEX idx_properties_price_status_include
ON properties(price, status) INCLUDE (id, owner_id, title, city, created_at);

CREATE INDEX idx_viewings_property_id ON viewings(property_id);
CREATE INDEX idx_viewings_user_id ON viewings(user_id);

CREATE INDEX idx_viewings_property_scheduled 
ON viewings(property_id, scheduled_time ASC);

CREATE INDEX idx_properties_owner_id ON properties(owner_id);

CREATE INDEX idx_properties_owner_created 
ON properties(owner_id, created_at DESC);