-- ============================================================================
-- Queries: Real Estate Database (Variant 24)
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

-- ============================================================================
-- 4. Добавление объекта недвижимости
-- API: POST /api/v1/properties
-- ============================================================================
INSERT INTO properties (owner_id, title, description, city, address, price, status)
VALUES (1, 'Новая квартира', 'Светлая квартира в центре', 'Moscow', 'ул. Новая, 10', 12000000.00, 'active')
RETURNING id, owner_id, title, city, price, status, created_at;

-- ============================================================================
-- 5. Поиск объектов по городу
-- API: GET /api/v1/properties?city=Moscow
-- ============================================================================
SELECT id, owner_id, title, city, price, status, created_at
FROM properties
WHERE city = 'Moscow'
  AND status != 'archived'
ORDER BY created_at DESC;

-- ============================================================================
-- 6. Поиск объектов по цене (диапазон)
-- API: GET /api/v1/properties?min_price=5000000&max_price=20000000
-- ============================================================================
SELECT id, owner_id, title, city, price, status, created_at
FROM properties
WHERE price BETWEEN 5000000 AND 20000000
  AND status != 'archived'
ORDER BY price ASC;

-- ============================================================================
-- 7. Поиск объектов по городу и цене (комбинированный)
-- API: GET /api/v1/properties?city=Moscow&min_price=5000000&max_price=20000000
-- ============================================================================
SELECT id, owner_id, title, city, price, status, created_at
FROM properties
WHERE city = 'Moscow'
  AND price BETWEEN 5000000 AND 20000000
  AND status != 'archived'
ORDER BY price ASC;

-- ============================================================================
-- 8. Запись на просмотр объекта
-- API: POST /api/v1/properties/{property_id}/viewings
-- ============================================================================
INSERT INTO viewings (property_id, user_id, scheduled_time, status, comment)
VALUES (1, 2, '2024-03-15T14:00:00Z', 'scheduled', 'Хочу посмотреть кухню')
RETURNING id, property_id, user_id, scheduled_time, status, comment;

-- ============================================================================
-- 9. Получение записей на просмотр объекта
-- API: GET /api/v1/properties/{property_id}/viewings
-- ============================================================================
SELECT v.id, v.property_id, v.user_id, v.scheduled_time, v.status, v.comment,
       u.first_name, u.last_name, u.email
FROM viewings v
JOIN users u ON v.user_id = u.id
WHERE v.property_id = 1
ORDER BY v.scheduled_time ASC;

-- ============================================================================
-- 10. Получение объектов пользователя (владельца)
-- API: GET /api/v1/properties?owner_id=1
-- ============================================================================
SELECT id, owner_id, title, city, price, status, created_at
FROM properties
WHERE owner_id = 1
  AND status != 'archived'
ORDER BY created_at DESC;

-- ============================================================================
-- 11. Изменение статуса объекта
-- API: PATCH /api/v1/properties/{property_id}
-- ============================================================================
UPDATE properties
SET status = 'sold',
    updated_at = NOW()
WHERE id = 1
  AND owner_id = 1  -- Проверка прав владельца
RETURNING id, status, updated_at;

-- ============================================================================
-- 12. Получение всех просмотров пользователя (кто смотрел)
-- API: GET /api/v1/viewings?user_id=2
-- ============================================================================
SELECT v.id, v.property_id, v.scheduled_time, v.status, v.comment,
       p.title, p.city, p.price
FROM viewings v
JOIN properties p ON v.property_id = p.id
WHERE v.user_id = 2
ORDER BY v.scheduled_time DESC;

-- ============================================================================
-- 13. Отмена просмотра
-- API: PATCH /api/v1/viewings/{viewing_id}
-- ============================================================================
UPDATE viewings
SET status = 'cancelled'
WHERE id = 1
  AND user_id = 2  -- Проверка прав пользователя
RETURNING id, status;

-- ============================================================================
-- 14. Удаление объекта (мягкое удаление через archived)
-- API: DELETE /api/v1/properties/{property_id}
-- ============================================================================
UPDATE properties
SET status = 'archived'
WHERE id = 1
  AND owner_id = 1  -- Проверка прав владельца
RETURNING id, status;

-- ============================================================================
-- 15. Статистика: количество объектов по городу
-- (Дополнительный запрос для аналитики)
-- ============================================================================
SELECT city, COUNT(*) as property_count, AVG(price) as avg_price
FROM properties
WHERE status != 'archived'
GROUP BY city
ORDER BY property_count DESC;

-- ============================================================================
-- 16. Статистика: количество просмотров по объекту
-- (Дополнительный запрос для аналитики)
-- ============================================================================
SELECT p.id, p.title, COUNT(v.id) as viewing_count
FROM properties p
LEFT JOIN viewings v ON p.id = v.property_id
WHERE p.status != 'archived'
GROUP BY p.id, p.title
ORDER BY viewing_count DESC;