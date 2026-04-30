> Настоящий документ актуален только для лабораторной работы №2. В ходе выполнения лабораторной работы №3 базы данных properties и viewings были перенесены в MongoDB. В связи с этим соответствующие таблицы, индексы и механизмы партиционирования в PostgreSQL были удалены, а часть ранее использовавшихся запросов стала неактуальной или технически невыполнимой в новой архитектуре.

# Отчет по созданию индексов БД

**Цель:** Обосновать выбор стратегий индексации для оптимизации работы системы управления недвижимостью.
**СУБД:** PostgreSQL 16

---

## 1. Индексы первичных ключей (Автоматические)

Эти индексы создаются автоматически при объявлении ограничения `PRIMARY KEY`.

- **Объекты:** `users.id`, `properties.id`, `viewings.id`.
- **Тип:** B-Tree (Unique).
- **Обоснование:**
  - Гарантируют уникальность записей.
  - Обеспечивают доступ к записи за O(1) по идентификатору.
  - Необходимы для работы внешних ключей (`FOREIGN KEY`) и механизмов ссылочной целостности.
  - В таблице `viewings` составной первичный ключ `(id, created_at)` также служит кластеризующим ключом для партиционирования.

---

## 2. Индексы внешних ключей (Ручное создание)

В PostgreSQL индексы для внешних ключей **не создаются автоматически**. Их отсутствие приводит к блокировкам всей таблицы (`LOCK`) при удалении или обновлении родительской записи, так как СУБД вынуждена делать полный скан таблицы для проверки связей.

- **Созданные индексы:**
  - `idx_properties_owner_id` (на `properties.owner_id`)
  - `idx_viewings_property_id` (на `viewings.property_id`)
  - `idx_viewings_user_id` (на `viewings.user_id`)
  
- **Обоснование:**
  1. **Ускорение JOIN:** Критически важны для запросов №9 и №12, где происходит соединение таблиц (`JOIN users`, `JOIN properties`). Без индексов время выполнения растет линейно от размера таблицы.
  2. **Оптимизация каскадного удаления:** При удалении пользователя (`ON DELETE CASCADE`) база данных должна мгновенно найти все его объекты и просмотры. Индексы позволяют сделать это без блокировки таблицы на долгое время.
  3. **Фильтрация по владельцу:** Запрос №10 («Мои объекты») использует поле `owner_id`. Индекс `idx_properties_owner_created` (составной) дополнительно оптимизирует сортировку по дате создания.

---

## 3. Индексы для поиска и фильтрации (WHERE, ORDER BY)

### 3.1. Аутентификация и поиск пользователей
- **Индекс:** `idx_users_login` (Unique, B-Tree)
  - **Запрос:** №2 (Поиск по логину).
  - **Зачем:** Мгновенный поиск точного совпадения. Ограничение `UNIQUE` также гарантирует, что два пользователя не смогут зарегистрироваться с одинаковым логином на уровне БД.

- **Индексы:** `idx_users_first_trgm`, `idx_users_last_trgm` (GIN, pg_trgm)
  - **Запрос:** №3 (Поиск по маске имени/фамилии).
  - **Зачем:** Стандартный индекс B-Tree работает только для поиска по префиксу (`'Ivan%'`). Индексы на основе триграмм (`pg_trgm`) позволяют эффективно искать подстроку в любом месте (`'%van%'`) и поддерживать нечеткий поиск (исправление опечаток), что критично для пользовательского поиска.

### 3.2. Каталог недвижимости (Properties)
- **Индекс:** `idx_properties_city_status_created` (B-Tree, составной)
  - **Запрос:** №5 (Фильтр по городу и статусу, сортировка по дате).
  - **Структура:** `(city, status, created_at DESC)`
  - **Зачем:** Позволяет базе данных сразу отсканировать только нужный город и статус, а данные внутри уже будут идти в нужном порядке. Это исключает операцию дорогостоящей сортировки (`Sort`) в памяти или на диске.

- **Индекс:** `idx_properties_price_status_include` (B-Tree, покрывающий)
  - **Запрос:** №6, №7 (Фильтр по цене, вывод списка).
  - **Структура:** `(price, status) INCLUDE (id, owner_id, title, city, created_at)`
  - **Зачем:** Это **покрывающий индекс** (Covering Index). Все данные, необходимые для вывода списка в запросе (название, город, цена), хранятся непосредственно в индексе. Базе данных не нужно обращаться к основной таблице (Heap), что реализует стратегию **Index Only Scan**. Это максимально возможная скорость чтения для списков.

- **Индекс:** `idx_properties_owner_created` (B-Tree, составной)
  - **Запрос:** №10 (Мои объекты).
  - **Зачем:** Оптимизирует выборку объектов конкретного владельца с сортировкой по новизне.

### 3.3. Расписание просмотров (Viewings)
- **Индекс:** `idx_viewings_property_scheduled` (B-Tree, составной)
  - **Запрос:** №9 (График просмотров для объекта).
  - **Структура:** `(property_id, scheduled_time ASC)`
  - **Зачем:** Позволяет быстро получить хронологический список всех просмотров для конкретной квартиры. Критично для проверки занятости слотов перед записью нового просмотра.

---

Примененный набор индексов покрывает все основные сценарии использования API. Такая конфигурация обеспечивает отклик системы в пределах миллисекунд даже при росте объема данных до миллионов записей.

# Отчет по оптимизации запросов

**Цель:** Продемонстрировать влияние индексов и структуры запросов на план выполнения SQL-запросов в PostgreSQL с использованием команды EXPLAIN. В данном разделе описаны 5 экспериментов, наглядно демонстрирующие ускорение при использовании индексов.

**Инструментарий:**
- СУБД: PostgreSQL 16
- Команда анализа: EXPLAIN (ANALYZE)
- Тестовые данные: 100000 пользователей, 1000000 объектов недвижимости, 2000000 записей на просмотр.

## Эксперимент 1: Поиск пользователя по маске имени

**Запрос:** Поиск по префиксу имени или фамилии (маска Ivan* -> LIKE 'Ivan%').

```sql
EXPLAIN ANALYZE
SELECT id, login, first_name, last_name, email, created_at
FROM users
WHERE first_name LIKE 'Ivan%' OR last_name LIKE 'Ivan%';
```

### 1.1. До создания индексов

План выполнения:
```
                                                QUERY PLAN
----------------------------------------------------------------------------------------------------------
 Seq Scan on users  (cost=0.00..3192.00 rows=9801 width=63) (actual time=0.517..23.854 rows=5000 loops=1)
   Filter: (((first_name)::text ~~ 'Ivan%'::text) OR ((last_name)::text ~~ 'Ivan%'::text))
   Rows Removed by Filter: 95000
 Planning Time: 4.296 ms
 Execution Time: 23.967 ms
(5 rows)
```

### 1.2. После создания индексов

Созданные индексы:
```sql
CREATE EXTENSION IF NOT EXISTS pg_trgm;

CREATE INDEX idx_users_first_trgm ON users USING gin (first_name gin_trgm_ops);
CREATE INDEX idx_users_last_trgm ON users USING gin (last_name gin_trgm_ops);
```

План выполнения:
```
                                                                QUERY PLAN
------------------------------------------------------------------------------------------------------------------------------------------
 Bitmap Heap Scan on users  (cost=139.97..1982.76 rows=9801 width=63) (actual time=1.772..3.651 rows=5000 loops=1)
   Recheck Cond: (((first_name)::text ~~ 'Ivan%'::text) OR ((last_name)::text ~~ 'Ivan%'::text))
   Heap Blocks: exact=1692
   ->  BitmapOr  (cost=139.97..139.97 rows=10053 width=0) (actual time=1.591..1.592 rows=0 loops=1)
         ->  Bitmap Index Scan on idx_users_first_trgm  (cost=0.00..67.53 rows=5027 width=0) (actual time=0.853..0.853 rows=5000 loops=1)
               Index Cond: ((first_name)::text ~~ 'Ivan%'::text)
         ->  Bitmap Index Scan on idx_users_last_trgm  (cost=0.00..67.53 rows=5027 width=0) (actual time=0.738..0.738 rows=5000 loops=1)
               Index Cond: ((last_name)::text ~~ 'Ivan%'::text)
 Planning Time: 0.294 ms
 Execution Time: 3.844 ms
(10 rows)
```

### 1.3 Описание оптимизации:
В исходном варианте выполнялся полный последовательный скан таблицы (Seq Scan). Для поиска по префиксу стандартные B-Tree индексы неэффективны, поэтому было подключено расширение pg_trgm и созданы GIN-индексы для first_name и last_name. Планировщик использовал Bitmap Index Scan с объединением результатов через BitmapOr.
**Результат:** Время выполнения сократилось с ~24 мс до ~3.8 мс (ускорение ~6.3 раза). Индексы триграмм позволили избежать полного сканирования таблицы.

## Эксперимент 2: Поиск объектов по городу

**Запрос:** Получение пятидесяти активных объектов по городу с сортировкой по дате.

```sql
EXPLAIN ANALYZE
SELECT id, owner_id, title, city, price, status, created_at
FROM properties
WHERE status <> 'archived'
  AND city = 'Moscow'
ORDER BY created_at DESC
LIMIT 50;
```

### 2.1 До создания индексов

План выполнения:
```
                                                                  QUERY PLAN           
----------------------------------------------------------------------------------------------------------------------------------------------
 Limit  (cost=40269.14..40274.97 rows=50 width=88) (actual time=49.370..51.566 rows=50 loops=1)
   ->  Gather Merge  (cost=40269.14..68339.93 rows=240590 width=88) (actual time=49.368..51.559 rows=50 loops=1)
         Workers Planned: 2
         Workers Launched: 2
         ->  Sort  (cost=39269.11..39569.85 rows=120295 width=88) (actual time=46.444..46.446 rows=38 loops=3)
               Sort Key: created_at DESC
               Sort Method: top-N heapsort  Memory: 36kB
               Worker 0:  Sort Method: top-N heapsort  Memory: 36kB
               Worker 1:  Sort Method: top-N heapsort  Memory: 36kB
               ->  Parallel Seq Scan on properties  (cost=0.00..35273.00 rows=120295 width=88) (actual time=0.028..34.823 rows=95984 loops=3)
                     Filter: (((status)::text <> 'archived'::text) AND ((city)::text = 'Moscow'::text))
                     Rows Removed by Filter: 237349
 Planning Time: 0.165 ms
 Execution Time: 51.682 ms
(14 rows)
```

### 2.2 После создания индексов

Созданные индексы:
```sql
CREATE INDEX idx_properties_fast
ON properties (city, created_at DESC)
WHERE status <> 'archived';
```

План выполнения:
```
                                                                    QUERY PLAN         
--------------------------------------------------------------------------------------------------------------------------------------------------
 Limit  (cost=0.42..21.67 rows=50 width=88) (actual time=0.022..0.184 rows=50 loops=1)
   ->  Index Scan using idx_properties_fast on properties  (cost=0.42..122673.66 rows=288709 width=88) (actual time=0.021..0.182 rows=50 loops=1)
         Index Cond: ((city)::text = 'Moscow'::text)
 Planning Time: 3.326 ms
 Execution Time: 0.206 ms
(5 rows)
```

### 2.3 Описание оптимизации:
Создан составной индекс (city, created_at) с фильтром по status, что позволило использовать Index Scan вместо сортировки после Seq Scan. Планировщик сразу выбирает только необходимые строки по индексу, минимизируя чтение таблицы.
**Результат:** Время выполнения сократилось с ~51 мс до ~0.2 мс (ускорение ~250 раз). Основное улучшение достигнуто за счет прямого доступа к нужным страницам таблицы через индекс.

## Эксперимент 3: Поиск объектов по диапазону цен

**Запрос:** Фильтрация по цене с исключением архивных записей.

```sql
EXPLAIN ANALYZE
SELECT id, owner_id, title, city, price, status, created_at
FROM properties
WHERE price BETWEEN 5000000 AND 20000000
  AND status != 'archived'
ORDER BY price ASC;
```

### 3.1 До создания индексов

План выполнения:
```
                                                              QUERY PLAN               
---------------------------------------------------------------------------------------------------------------------------------------
 Gather Merge  (cost=49264.27..70660.79 rows=183386 width=87) (actual time=88.932..167.778 rows=219063 loops=1)
   Workers Planned: 2
   Workers Launched: 2
   ->  Sort  (cost=48264.24..48493.48 rows=91693 width=87) (actual time=83.677..99.931 rows=73021 loops=3)
         Sort Key: price
         Sort Method: external merge  Disk: 8072kB
         Worker 0:  Sort Method: external merge  Disk: 7104kB
         Worker 1:  Sort Method: external merge  Disk: 7192kB
         ->  Parallel Seq Scan on properties  (cost=0.00..36317.67 rows=91693 width=87) (actual time=0.013..55.913 rows=73021 loops=3)
               Filter: ((price >= '5000000'::numeric) AND (price <= '20000000'::numeric) AND ((status)::text <> 'archived'::text))
               Rows Removed by Filter: 260312
 Planning Time: 0.576 ms
 Execution Time: 172.582 ms
(13 rows)
```

### 3.2 После создания индексов

Созданные индексы:
```sql
CREATE INDEX idx_properties_price_status 
ON properties(price, status) 
INCLUDE (id, owner_id, title, city, created_at);
```

План выполнения:
```
                                                                         QUERY PLAN    
-------------------------------------------------------------------------------------------------------------------------------------------------------------
 Index Only Scan using idx_properties_price_status on properties  (cost=0.42..20938.78 rows=220064 width=87) (actual time=0.037..42.320 rows=219063 loops=1)
   Index Cond: ((price >= '5000000'::numeric) AND (price <= '20000000'::numeric))
   Filter: ((status)::text <> 'archived'::text)
   Rows Removed by Filter: 36449
   Heap Fetches: 10
 Planning Time: 0.087 ms
 Execution Time: 45.670 ms
(7 rows)
```

### 3.3 Описание оптимизации:
Создан покрывающий индекс (price, status) с INCLUDE для остальных полей. Планировщик выбрал Index Only Scan, что минимизирует обращения к основной таблице.
**Результат:** Время выполнения снизилось с ~173 мс до ~46 мс (ускорение ~3.8 раза). Почти все данные получены из индекса, Heap Fetches минимальны.

## Эксперимент 4: Поиск объектов по диапазону цен

**Запрос:** Множественная фильтрация с сортировкой.

```sql
EXPLAIN ANALYZE
SELECT id, owner_id, title, city, price, status, created_at
FROM properties
WHERE city = 'Moscow'
  AND price BETWEEN 5000000 AND 20000000
  AND status != 'archived'
ORDER BY price ASC;
```

### 4.1 До создания индексов

План выполнения:
```
                                                                              QUERY PLAN
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
 Gather Merge  (cost=40657.50..47858.67 rows=61720 width=88) (actual time=67.787..90.160 rows=73705 loops=1)
   Workers Planned: 2
   Workers Launched: 2
   ->  Sort  (cost=39657.48..39734.63 rows=30860 width=88) (actual time=64.687..66.341 rows=24568 loops=3)
         Sort Key: price
         Sort Method: quicksort  Memory: 3552kB
         Worker 0:  Sort Method: quicksort  Memory: 3393kB
         Worker 1:  Sort Method: quicksort  Memory: 3386kB
         ->  Parallel Seq Scan on properties  (cost=0.00..37356.33 rows=30860 width=88) (actual time=0.029..57.650 rows=24568 loops=3)
               Filter: ((price >= '5000000'::numeric) AND (price <= '20000000'::numeric) AND ((status)::text <> 'archived'::text) AND ((city)::text = 'Moscow'::text))
               Rows Removed by Filter: 308765
 Planning Time: 0.208 ms
 Execution Time: 91.648 ms
(13 rows)
```

### 4.2 После создания индексов

Созданные индексы:
```sql
CREATE INDEX idx_properties_city_price_covering 
ON properties(city, price, status) 
INCLUDE (id, owner_id, title, created_at);
```

План выполнения:
```
                                                                           QUERY PLAN  
-----------------------------------------------------------------------------------------------------------------------------------------------------------------
 Index Only Scan using idx_properties_city_price_covering on properties  (cost=0.42..7440.97 rows=74065 width=88) (actual time=0.051..19.545 rows=73705 loops=1)
   Index Cond: ((city = 'Moscow'::text) AND (price >= '5000000'::numeric) AND (price <= '20000000'::numeric))
   Filter: ((status)::text <> 'archived'::text)
   Rows Removed by Filter: 12258
   Heap Fetches: 0
 Planning Time: 0.566 ms
 Execution Time: 20.932 ms
(7 rows)
```

### 4.3 Описание оптимизации:
Создан составной покрывающий индекс (city, price, status) INCLUDE (...). Порядок колонок совпадает с условиями WHERE и сортировкой ORDER BY, что позволяет планировщику выполнять Index Only Scan с минимальными Heap Fetches.
**Результат:** Время выполнения сократилось с ~92 мс до ~21 мс (ускорение ~4.4 раза).

## Эксперимент 5: Статистика — количество просмотров по объекту

**Запрос:** Подсчет просмотров для каждого активного объекта.

```sql
EXPLAIN ANALYZE SELECT p.id, p.title, v.viewing_count
FROM properties p
LEFT JOIN (
    SELECT property_id, COUNT(*) AS viewing_count
    FROM viewings
    GROUP BY property_id
) v ON p.id = v.property_id
WHERE p.status != 'archived'
ORDER BY v.viewing_count DESC;
```

### 5.1 До создания индексов

План выполнения:
```
                                                                       QUERY PLAN
--------------------------------------------------------------------------------------------------------------------------------------------------------
 Gather Merge  (cost=285369.63..368542.44 rows=712860 width=54) (actual time=1502.419..1628.200 rows=857143 loops=1)
   Workers Planned: 2
   Workers Launched: 2
   ->  Sort  (cost=284369.61..285260.68 rows=356430 width=54) (actual time=1474.004..1493.632 rows=285714 loops=3)
         Sort Key: v.viewing_count DESC
         Sort Method: external merge  Disk: 19664kB
         Worker 0:  Sort Method: external merge  Disk: 18384kB
         Worker 1:  Sort Method: external merge  Disk: 18136kB
         ->  Hash Left Join  (cost=194705.49..239317.45 rows=356430 width=54) (actual time=1056.773..1426.681 rows=285714 loops=3)
               Hash Cond: (p.id = v.property_id)
               ->  Parallel Seq Scan on properties p  (cost=0.00..34231.33 rows=356430 width=46) (actual time=6.742..230.856 rows=285714 loops=3)
                     Filter: ((status)::text <> 'archived'::text)
                     Rows Removed by Filter: 47619
               ->  Hash  (cost=183389.38..183389.38 rows=650969 width=16) (actual time=1049.352..1049.353 rows=864744 loops=3)
                     Buckets: 262144  Batches: 8  Memory Usage: 7131kB
                     ->  Subquery Scan on v  (cost=154745.00..183389.38 rows=650969 width=16) (actual time=592.653..967.423 rows=864744 loops=3)
                           ->  HashAggregate  (cost=154745.00..176879.69 rows=650969 width=16) (actual time=592.650..929.107 rows=864744 loops=3)
                                 Group Key: viewings.property_id
                                 Planned Partitions: 8  Batches: 41  Memory Usage: 8273kB  Disk Usage: 48600kB
                                 Worker 0:  Batches: 41  Memory Usage: 8273kB  Disk Usage: 49672kB
                                 Worker 1:  Batches: 41  Memory Usage: 8273kB  Disk Usage: 49672kB
                                 ->  Seq Scan on viewings  (cost=0.00..42245.00 rows=2000000 width=8) (actual time=0.258..240.955 rows=2000000 loops=3)
 Planning Time: 0.336 ms
 JIT:
   Functions: 54
   Options: Inlining false, Optimization false, Expressions true, Deforming true
   Timing: Generation 2.491 ms, Inlining 0.000 ms, Optimization 1.677 ms, Emission 21.037 ms, Total 25.206 ms
 Execution Time: 1651.628 ms
(28 rows)
```

### 5.2 После создания индексов

Созданные индексы:
```sql
CREATE INDEX idx_properties_status_id ON properties(status, id) INCLUDE (title);
CREATE INDEX idx_viewings_property_id ON viewings(property_id);
```

План выполнения:
```
                                                                                   QUERY PLAN
---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 Gather Merge  (cost=170565.49..253738.30 rows=712860 width=54) (actual time=407.945..511.614 rows=857143 loops=1)
   Workers Planned: 2
   Workers Launched: 2
   ->  Sort  (cost=169565.47..170456.54 rows=356430 width=54) (actual time=387.150..404.677 rows=285714 loops=3)
         Sort Key: (count(*)) DESC
         Sort Method: external merge  Disk: 22000kB
         Worker 0:  Sort Method: external merge  Disk: 17152kB
         Worker 1:  Sort Method: external merge  Disk: 16992kB
         ->  Merge Left Join  (cost=0.85..124513.31 rows=356430 width=54) (actual time=12.056..339.688 rows=285714 loops=3)
               Merge Cond: (p.id = viewings.property_id)
               ->  Parallel Index Scan using properties_pkey on properties p  (cost=0.42..50214.76 rows=356430 width=46) (actual time=4.615..61.774 rows=285714 loops=3)
                     Filter: ((status)::text <> 'archived'::text)
                     Rows Removed by Filter: 47619
               ->  GroupAggregate  (cost=0.43..62950.12 rows=650969 width=16) (actual time=0.045..231.510 rows=864584 loops=3)
                     Group Key: viewings.property_id
                     ->  Index Only Scan using idx_viewings_property_id on viewings  (cost=0.43..46440.43 rows=2000000 width=8) (actual time=0.037..88.079 rows=1999630 loops=3)
                           Heap Fetches: 0
 Planning Time: 0.273 ms
 JIT:
   Functions: 30
   Options: Inlining false, Optimization false, Expressions true, Deforming true
   Timing: Generation 2.548 ms, Inlining 0.000 ms, Optimization 0.876 ms, Emission 12.802 ms, Total 16.225 ms
 Execution Time: 528.299 ms
(23 rows)
```

### 5.3 Описание оптимизации:
Добавление индекса на viewings(property_id) ускорило агрегирование просмотров для каждого объекта, а индекс на properties(status, id) позволил быстро отфильтровать активные объекты перед соединением. Планировщик выбрал Merge Left Join и Index Only Scan, что уменьшило обращения к таблицам.
**Результат:** Время выполнения снизилось с ~1652 мс до ~528 мс (ускорение ~3.1 раза). Основное ускорение достигнуто за счет индексов, уменьшения Seq Scan и минимизации Heap Fetches.

## Отчет по партиционированию таблицы `viewings`

Таблица `viewings` разделена на партиции по диапазону дат (`created_at`).

- **Стратегия:** Range Partitioning по месяцам.
- **Партиции:** `viewings_2025_10` ... `viewings_2026_06`, а также `viewings_default` для защиты от ошибок ввода дат.
- **Обоснование:**
  1. **Производительность:** При запросах с фильтром по дате (например, «просмотры за март»), сканируются только соответствующие файлы данных, а не вся таблица целиком (Partition Pruning).
  2. **Обслуживание:** Старые данные (например, за 2025 год) можно быстро архивировать или удалять простой командой `DROP TABLE` для конкретной партиции, не нагружая базу операциями `DELETE`, которые вызывают фрагментацию и рост логов транзакций.
  3. **Масштабируемость:** Позволяет таблице расти практически бесконечно без деградации скорости вставки новых записей.

