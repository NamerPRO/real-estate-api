-- ============================================================================
-- Queries: Real Estate Database
-- Description: SQL-запросы для всех операций из варианта задания
-- ============================================================================

-- ============================================================================
-- 1. Создание нового пользователя
-- API: POST /api/v1/register
-- ============================================================================
INSERT INTO users (login, password_hash, first_name, last_name, email)
VALUES ('new_user', '$2b$12$example_hash', 'Peter', 'Parker', 'peter@example.com')
RETURNING id, login, first_name, last_name, email, created_at;

-- ============================================================================
-- 2. Поиск пользователя по логину (точное совпадение)
-- API: GET /api/v1/users?login=john_doe
-- ============================================================================
SELECT id, login, first_name, last_name, email, created_at
FROM users
WHERE login = 'john_doe';

-- ============================================================================
-- 3. Поиск пользователя по маске имени и фамилии
-- API: GET /api/v1/users?name_mask=Ivan*
-- ============================================================================
-- Поддержка * в конце маски (заменяем на %)
SELECT id, login, first_name, last_name, email, created_at
FROM users
WHERE first_name LIKE 'Ivan%' OR last_name LIKE 'Ivan%';