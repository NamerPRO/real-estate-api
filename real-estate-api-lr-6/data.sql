-- ============================================================================
-- Data Generation: Real Estate Database
-- Description: Заполнение базы данных
-- ============================================================================

-- Очистка данных перед заполнением
TRUNCATE TABLE users RESTART IDENTITY CASCADE;

-- Сброс статистики планировщика
ANALYZE;

-- ============================================================================
-- Генерация пользователей (10 записей)
-- ============================================================================
INSERT INTO users (login, password_hash, first_name, last_name, email, created_at) VALUES
('user_1', '$2b$12$LJ3m4ys3Q5w6z7x8c9v0b.N1o2p3q4r5s6t7u8v9w0x1y2z3a4b5c', 'Ivan', 'Ivanov', 'ivan.ivanov@example.com', NOW() - INTERVAL '10 days'),
('user_2', '$2b$12$LJ3m4ys3Q5w6z7x8c9v0b.N1o2p3q4r5s6t7u8v9w0x1y2z3a4b5c', 'Maria', 'Smirnova', 'maria.smirnova@example.com', NOW() - INTERVAL '9 days'),
('user_3', '$2b$12$LJ3m4ys3Q5w6z7x8c9v0b.N1o2p3q4r5s6t7u8v9w0x1y2z3a4b5c', 'Alexey', 'Petrov', 'alexey.petrov@example.com', NOW() - INTERVAL '8 days'),
('user_4', '$2b$12$LJ3m4ys3Q5w6z7x8c9v0b.N1o2p3q4r5s6t7u8v9w0x1y2z3a4b5c', 'Elena', 'Sokolova', 'elena.sokolova@example.com', NOW() - INTERVAL '7 days'),
('user_5', '$2b$12$LJ3m4ys3Q5w6z7x8c9v0b.N1o2p3q4r5s6t7u8v9w0x1y2z3a4b5c', 'Dmitry', 'Kuznetsov', 'dmitry.kuznetsov@example.com', NOW() - INTERVAL '6 days'),
('user_6', '$2b$12$LJ3m4ys3Q5w6z7x8c9v0b.N1o2p3q4r5s6t7u8v9w0x1y2z3a4b5c', 'Anna', 'Vasilieva', 'anna.vasilieva@example.com', NOW() - INTERVAL '5 days'),
('user_7', '$2b$12$LJ3m4ys3Q5w6z7x8c9v0b.N1o2p3q4r5s6t7u8v9w0x1y2z3a4b5c', 'Sergey', 'Popov', 'sergey.popov@example.com', NOW() - INTERVAL '4 days'),
('user_8', '$2b$12$LJ3m4ys3Q5w6z7x8c9v0b.N1o2p3q4r5s6t7u8v9w0x1y2z3a4b5c', 'Olga', 'Novikova', 'olga.novikova@example.com', NOW() - INTERVAL '3 days'),
('user_9', '$2b$12$LJ3m4ys3Q5w6z7x8c9v0b.N1o2p3q4r5s6t7u8v9w0x1y2z3a4b5c', 'Maxim', 'Fedorov', 'maxim.fedorov@example.com', NOW() - INTERVAL '2 days'),
('user_10', '$2b$12$LJ3m4ys3Q5w6z7x8c9v0b.N1o2p3q4r5s6t7u8v9w0x1y2z3a4b5c', 'Natalia', 'Morozova', 'natalia.morozova@example.com', NOW() - INTERVAL '1 day');

-- Обновляем статистику после вставки пользователей
ANALYZE users;