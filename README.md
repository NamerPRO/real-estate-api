# 🏠 Real Estate API

> REST API сервис для управления недвижимостью (аналог Zillow)

---

## 🏠 О проекте

Real Estate API — учебный backend-сервис, разработанный в рамках курса «Программная инженерия» студентом группы М8О-106СВ-25 Авдиянцем Петром.

Функционал:
```
- 👤 Пользователи — регистрация, логин, поиск
- 🏢 Недвижимость — добавление, фильтрация
- 📅 Просмотры — запись на просмотр
- 🔐 JWT-аутентификация
```

Вариант задания: №24 — Система управления недвижимостью (Zillow)

---

## ⚙️ Технологии

При выполнении задания лабораторной работы были использованы следующие технологии

```
- C++20
- Yandex Userver
- JWT (jwt-cpp, HS256)
- PostgreSQL 16
- MongoDB 7
- Docker, docker-compose
- OpenAPI 3.0 + Swagger UI
- pytest + requests
```

---

## 🚀 Быстрый старт

Требования
- Docker + Docker Compose 

Первый старт
```
docker-compose up --build 
```

Последующие старты
```
docker-compose up
```

Открыть Swagger UI
```
http://localhost:8080/api/v1/docs
```

---

## 🗄️ Архитектура хранения данных

Проект использует гибридный подход к хранению данных, сочетая реляционную СУБД PostgreSQL 16 для структурированных данных пользователей и документоориентированную базу данных MongoDB 7 для гибких сущностей недвижимости и истории просмотров.

### PostgreSQL: Таблица users
Таблица предназначена для хранения данных пользователей.

**Таблица `users`**: Хранит данные пользователей.
- `id`: Уникальный идентификатор (BIGINT, автоинкремент).
- `login`: Уникальный логин (3-50 символов).
- `password_hash`: Хэш пароля.
- `first_name`, `last_name`: Имя и фамилия.
- `email`: Уникальный email.
- `created_at`: Дата регистрации.

> 💡 Примечание по миграции: В рамках лабораторной работы №3 была реализована стратегия частичного перехода на MongoDB. Сущности properties (объекты недвижимости) и viewings (просмотры) были мигрированы из PostgreSQL в MongoDB для использования преимуществ гибкой схемы. Подробное описание стратегии индексации, сравнение планов выполнения запросов до и после оптимизации, а также обоснование стратегии партиционирования таблицы viewings применительно к лабораторной работе №2 доступно в документе: [optimization.md](optimization.md).

### MongoDB: Коллекции
Использование MongoDB обусловлено необходимостью хранения динамических структур данных (различные типы объектов недвижимости) и эффективной работы с массивами вложенных данных.

**Коллекция `properties`**
Хранит информацию об объектах недвижимости. Благодаря схеме NoSQL, структура документа может варьироваться в зависимости от типа объекта (квартира, дом, коммерция).

- `_id: String` - Уникальный идентификатор документа.
- `owner_id: Int32/Int64` - **Обязательное**. ID владельца (ссылка на таблицу пользователей в PostgreSQL).
- `type: String` - **Обязательное**. Тип недвижимости. Допустимые значения: "apartment", "house", "land", "commercial".
- `title: String` - **Обязательное**. Заголовок объявления (минимум 5 символов).
- `city: String` - **Обязательное**. Город (только буквы и пробелы).
- `address: Object` - **Обязательное**. Динамическое поле для описания адреса (структура зависит от типа недвижимости).
- `details: Object` - **Обязательное**. Динамическое поле для специфических характеристик (площадь, этаж и т.д.).
- `price: Number` - **Обязательное**. Стоимость объекта (неотрицательное число).
- `status: String` - **Обязательное**. Статус объявления. Допустимые значения: "active", "sold", "rented", "archived".
- `features: Array[String]` - **Опциональное**. Массив строк с особенностями объекта.
- `created_at: Date` - Дата создания.

**Коллекция `viewings`**
Хранит информацию о запланированных просмотрах недвижимости.
- `_id: String` - Уникальный идентификатор записи о просмотре.
- `property_id: ObjectId` - **Обязательное**. Ссылка на документ в коллекции properties.
- `user_id: Int32/Int64` - **Обязательное**. ID пользователя, который записался на просмотр (ссылка на PostgreSQL).
- `scheduled_time: Date` - **Обязательное**. Дата и время запланированного просмотра.
- `comments: Array[Object]` - **Обязательное**. Массив комментариев/заметок по просмотру.
  - Структура элемента массива:
    - `text: String` - Текст комментария.
    - `author: String` - Автор комментария (например, агент или клиент).
    - `timestamp: Date` - Время добавления комментария.
- `status: String` - **Опциональное**. Статус просмотра. Допустимые значения: "pending", "confirmed", "cancelled", "completed".

## 💾 Инициализация баз данных

Для заполнения PostgreSQL и MongoDB тестовыми данными выполните команды, приведенные ниже. Скрипт создаст по 10 записей в каждой из баз данных.

Для PowerShell (Windows)
```powershell
Get-Content data.sql | docker exec -i postgres_real_estate psql -U api_user -d real_estate_db
Get-Content data.js | docker exec -i mongodb_real_estate mongosh -u admin -p password
```

Для Linux / macOS
```bash
cat data.sql | docker exec -i postgres_real_estate psql -U api_user -d real_estate_db
cat data.js | docker exec -i mongodb_real_estate mongosh -u admin -p password
```
Или используя перенаправление ввода:
```bash
docker exec -i postgres_real_estate psql -U api_user -d real_estate_db < data.sql
docker exec -i mongodb_real_estate mongosh -u admin -p password < data.js
```

## 📡 API Endpoints

Базовый URL
```
http://localhost:8080/api/v1
```

---

#### Аутентификация и Пользователи

| Метод | Путь      | Краткое описание | Требуется аутентификация |
| :---- | :-------- | :--------------- | :-----------------------: |
| `POST` | `/users`               | Создание пользователя (регистрация) | Не требуется JWT |
| `POST`  | `/login`    | Получение JWT токена        | Не требуется JWT |
| `GET`  | `/users?login=...`     | Поиск по логину    | Не требуется JWT |
| `GET`  | `/users?name_mask=...` | Поиск имени/фамилии    | Не требуется JWT |

#### Недвижимость

| Метод  | Путь                                      | Краткое описание   | Аутентификация |
| :----- | :---------------------------------------- | :--------- | :------------: |
| `POST` | `/properties`                             | Добавление недвижимости | Требуется JWT |
| `GET`  | `/properties?city=...`                    | Поиск по городу     | Не требуется JWT |
| `GET`  | `/properties?min_price=...&max_price=...` | Поиск по диапазону цен     | Не требуется JWT |

#### Просмотры

| Метод  | Путь                        | Краткое описание | Аутентификация |
| :----- | :-------------------------- | :------- | :------------: |
| `POST` | `/properties/{id}/viewings` | Запись на просмотр недвижимости   | Требуется JWT |

#### Документация

| Метод | Путь                 | Краткое описание   |
| :---- | :------------------- | :---------: |
| `GET` | `/docs`              | Swagger UI |
| `GET` | `/docs/openapi.yaml` | OpenAPI    |

<i><br/>Подробное описание смотри в Swagger UI.</i>

---

## 🔐 Аутентификация и Регистрация

Получение токена состоит из двух этапов.

1. Регистрация
```
curl -X POST http://localhost:8080/api/v1/users \
  -H "Content-Type: application/json" \
  -d '{
    "login": "john_doe",
    "password": "secure123",
    "first_name": "John",
    "last_name": "Doe",
    "email": "john@example.com"
  }'
```

2. Аутентификация
```
curl -X POST http://localhost:8080/api/v1/login \
  -H "Content-Type: application/json" \
  -d '{
    "login": "john_doe",
    "password": "secure123"
  }'
```

3. Результатом шага 2 станет ответ с JWT токеном
```
{
  "access_token": "JWT_TOKEN",
  "token_type": "Bearer",
  "user_id": 1,
  "login": "john_doe"
}
```

---

## 🔑 Использование токена

Пример использования токена
```
curl -X POST http://localhost:8080/api/v1/properties \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <TOKEN>" \
  -d '{
    "owner_id": 1,
    "type": "apartment",
    "title": "Уютная двушка в центре",
    "city": "Moscow",
    "address": {
      "street": "Tverskaya",
      "house": "10",
      "flat": "5"
    },
    "price": 15000000,
    "details": {
      "rooms": 2,
      "floor": 5,
      "total_floors": 9,
      "has_elevator": true
    },
    "features": [
      "balcony",
      "internet"
    ],
    "status": "active" 
  }'
```

---

## 📝 Примеры запросов

#### Регистрация
```
curl -X POST http://localhost:8080/api/v1/users \
  -d '{
    "login": "alice_wonder",
    "password": "wonderland123",
    "first_name": "Alice",
    "last_name": "Wonder",
    "email": "alice@example.com"
  }'
```

---

#### Поиск недвижимости
```
curl "http://localhost:8080/api/v1/properties?city=Moscow"
```

---

#### Запись на просмотр
```
curl -X POST http://localhost:8080/api/v1/properties/101/viewings \
  -H "Authorization: Bearer <TOKEN>" \
  -d '{
    "property_id": "19936beec3de4b099bc3017f27cd8013",
    "user_id": 1,
    "scheduled_time": "2026-04-15T10:00:00Z",
    "comments": [
      {
        "text": "Буду с риелтором",
        "author": "user",
        "timestamp": "2026-04-15T12:30:00Z"
      }
    ]
  }'
```

---

#### Некоторые ошибки

404
```
{
  "code": "NOT_FOUND",
  "message": "User not found"
}
```

401
```
{
  "code": "UNAUTHORIZED",
  "message": "Invalid credentials"
}
```

---

## 🧪 Тестирование

Для запуска тестов выполните следующие команды
```
pip install -r tests/requirements.txt  
pytest tests/test_api.py -v  
```
<br/>

Для запуска aggregation pipeline выполните следующую команду:
```
Get-Content aggregation.js | docker exec -i mongodb_real_estate mongosh -u admin -p password
```
<br/>

Для тестирования взаимодействия с MongoDB выполните следующие команды:
```
docker exec -i mongodb_real_estate mongosh -u admin -p password
load("./queries.js");
```
Далее вы сможете писать, например:
```
findPropertiesByCity("Moscow");
```

---

## 📚 Дополнительная документация

- Swagger UI: `http://localhost:8080/api/v1/docs`
- OpenAPI: `http://localhost:8080/api/v1/docs/openapi.yaml`
- Оптимизация PostgreSQL БД (актуален для лр №2): См. файл [optimization.md](optimization.md)
- Описание проектирования MongoDB: См. файл [schema_design.md](schema_design.md)