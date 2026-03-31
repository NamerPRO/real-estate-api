-- ============================================================================
-- Data Generation: Real Estate Database
-- Description: Генерация 1M+ записей с правдоподобными данными
-- Время выполнения: ~10-30 секунд
-- ============================================================================

-- Очистка данных перед генерацией (CASCADE удалит зависимые записи)
TRUNCATE TABLE viewings RESTART IDENTITY CASCADE;
TRUNCATE TABLE properties RESTART IDENTITY CASCADE;
TRUNCATE TABLE users RESTART IDENTITY CASCADE;

-- Сброс статистики планировщика
ANALYZE;

-- ============================================================================
-- 1. Генерация пользователей (100.000 записей)
-- Используем массивы имен и фамилий для реалистичности
-- ============================================================================
DO $$
DECLARE
    first_names TEXT[] := ARRAY['Ivan', 'Dmitry', 'Alexey', 'Sergey', 'Andrey', 'Maxim', 'Alexander', 'Mikhail', 'Nikolay', 'Vladimir', 'Anna', 'Maria', 'Elena', 'Olga', 'Natalia', 'Tatiana', 'Ekaterina', 'Julia', 'Anastasia', 'Irina'];
    last_names TEXT[] := ARRAY['Ivanov', 'Smirnov', 'Kuznetsov', 'Popov', 'Vasiliev', 'Petrov', 'Sokolov', 'Mikhaylov', 'Novikov', 'Fedorov', 'Morozov', 'Volkov', 'Alekseev', 'Lebedev', 'Semenov', 'Egorov', 'Pavlov', 'Kozlov', 'Stepanov', 'Nikolaev'];
    name_len INT;
    lname_len INT;
BEGIN
    INSERT INTO users (login, password_hash, first_name, last_name, email, created_at)
    SELECT 
        'user_' || g.id,
        '$2b$12$' || substr(md5(random()::text), 1, 50), -- Псевдо-хэш
        first_names[(g.id % array_length(first_names, 1)) + 1],
        last_names[(g.id % array_length(last_names, 1)) + 1],
        'user_' || g.id || '@example.com',
        NOW() - (random() * INTERVAL '365 days')
    FROM generate_series(1, 100000) AS g(id);
    
    RAISE NOTICE 'Generated 100.000 users.';
END $$;

-- Обновляем статистику после вставки пользователей
ANALYZE users;

-- ============================================================================
-- 2. Генерация объектов недвижимости (1.000.000 записей)
-- Это самая тяжелая операция. Используем CROSS JOIN для комбинаторики.
-- ============================================================================
DO $$
DECLARE
    titles_prefix TEXT[] := ARRAY['Уютная', 'Светлая', 'Просторная', 'Современная', 'Элитная', 'Бюджетная', 'Роскошная', 'Тихая', 'Центральная', 'Новая'];
    titles_suffix TEXT[] := ARRAY['квартира', 'студия', 'апартаменты', 'лофт', 'дом', 'таунхаус', 'дача', 'комната', 'коттедж', 'пенхаус'];
    streets TEXT[] := ARRAY['ул. Ленина', 'пр. Мира', 'ул. Гагарина', 'пер. Советский', 'ул. Пушкина', 'бульвар Цветной', 'ул. Кирова', 'пр. Ленина', 'ул. Мира', 'пер. Тихий'];
    cities TEXT[] := ARRAY['Moscow', 'Saint Petersburg', 'Kazan', 'Novosibirsk', 'Yekaterinburg', 'Nizhny Novgorod', 'Chelyabinsk', 'Samara', 'Omsk', 'Rostov-on-Don'];
    statuses TEXT[] := ARRAY['active', 'active', 'active', 'active', 'sold', 'rented', 'archived']; -- active чаще всего
BEGIN
    INSERT INTO properties (owner_id, title, description, city, address, price, status, created_at)
    SELECT 
        (random() * 99999 + 1)::INT as owner_id, -- Случайный владелец из 1000
        -- Генерация заголовка: "Уютная квартира"
        titles_prefix[(g.id % array_length(titles_prefix, 1)) + 1] || ' ' || 
        titles_suffix[(g.id % array_length(titles_suffix, 1)) + 1] || ' #' || g.id,
        -- Описание
        'Объект #' || g.id || '. Отличный вариант в районе ' || (g.id % 100),
        -- Город (с весом: Москва и СПб чаще)
        CASE 
            WHEN random() < 0.3 THEN 'Moscow'
            WHEN random() < 0.5 THEN 'Saint Petersburg'
            ELSE cities[(g.id % array_length(cities, 1)) + 1]
        END,
        -- Адрес
        streets[(g.id % array_length(streets, 1)) + 1] || ', д. ' || (g.id % 200) || ', кв. ' || (g.id % 50),
        -- Цена: от 2 млн до 100 млн, с логарифмическим распределением (больше дешевых)
        (2000000 + (random() ^ 3) * 98000000)::DOUBLE PRECISION,
        -- Статус
        statuses[(g.id % array_length(statuses, 1)) + 1],
        -- Дата создания
        NOW() - (random() * INTERVAL '365 days')
    FROM generate_series(1, 1000000) AS g(id);
    
    RAISE NOTICE 'Generated 1.000.000 properties.';
END $$;

-- Обновляем статистику
ANALYZE properties;

-- ============================================================================
-- 3. Генерация просмотров (2.000.000 записей)
-- Много записей, чтобы нагрузить таблицу viewings
-- ============================================================================
DO $$
DECLARE
    viewing_statuses TEXT[] := ARRAY['scheduled', 'scheduled', 'completed', 'cancelled']; -- scheduled чаще
BEGIN
    INSERT INTO viewings (property_id, user_id, scheduled_time, status, comment, created_at)
    SELECT 
        (random() * 999999 + 1)::INT as property_id, -- Случайный объект (1..1000000)
        (random() * 99999 + 1)::INT as user_id,       -- Случайный пользователь (1..100000)
        ('2025-10-01'::timestamp + random() * INTERVAL '360 days') as scheduled_time,
        viewing_statuses[(g.id % array_length(viewing_statuses, 1)) + 1] as status,
        CASE WHEN random() > 0.7 THEN 'Комментарий к просмотру #' || g.id ELSE NULL END as comment,
        ('2025-10-01'::timestamp + random() * INTERVAL '270 days') as gen_created_at
    FROM generate_series(1, 2000000) AS g(id);
    
    RAISE NOTICE 'Generated 2.000.000 viewings.';
END $$;

-- Финальное обновление статистики по всей базе
ANALYZE;

-- Проверка итогов
SELECT 'users' as table_name, COUNT(*) as count FROM users
UNION ALL
SELECT 'properties', COUNT(*) FROM properties
UNION ALL
SELECT 'viewings', COUNT(*) FROM viewings;