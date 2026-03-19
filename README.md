# 🏠 Real Estate API

> REST API сервис для управления недвижимостью (аналог Zillow)

---

## 🏠 О проекте

Real Estate API — учебный backend-сервис, разработанный в рамках курса «Программная инженерия» студентом группы М8О-106СВ-25 Авдиянцем Петром.

```Функционал:
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
- In-memory (std::map)
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

## 📡 API Endpoints

Базовый URL
```
http://localhost:8080/api/v1
```

---

#### Аутентификация

| Метод | Путь      | Краткое описание | Требуется аутентификация |
| :---- | :-------- | :--------------- | :----------------------- |
| `POST`  | /register | Регистрация      | ❌ Не требуется JWT                       |
| `POST`  | /login    | Получение JWT токена        | ❌ Не требуется JWT                       |

#### Пользователи

| Метод  | Путь                   | Краткое описание | Аутентификация |
| :----- | :--------------------- | :------- | :------------: |
| `POST` | `/users`               | Создание пользователя |       ✅ Требуется JWT       |
| `GET`  | `/users?login=...`     | Поиск по логину    |       ❌ Не требуется JWT       |
| `GET`  | `/users?name_mask=...` | Поиск имени/фамилии    |       ❌ Не требуется JWT       |

#### Недвижимость

| Метод  | Путь                                      | Краткое описание   | Аутентификация |
| :----- | :---------------------------------------- | :--------- | :------------: |
| `POST` | `/properties`                             | Добавление недвижимости |       ✅ Требуется JWT       |
| `GET`  | `/properties?city=...`                    | Поиск по городу     |       ❌ Не требуется JWT       |
| `GET`  | `/properties?min_price=...&max_price=...` | Поиск по диапазону цен     |       ❌ Не требуется JWT       |

#### Просмотры

| Метод  | Путь                        | Краткое описание | Аутентификация |
| :----- | :-------------------------- | :------- | :------------: |
| `POST` | `/properties/{id}/viewings` | Запись на просмотр недвижимости   |       ✅ Требуется JWT       |

#### Документация

| Метод | Путь                 | Краткое описание   |
| :---- | :------------------- | :--------- |
| `GET` | `/docs`              | Swagger UI |
| `GET` | `/docs/openapi.yaml` | OpenAPI    |

<i><br/>Подробное описание смотри в Swagger UI.</i>

---

## 🔐 Аутентификация

Получение токена состоит из двух этапов.

1. Регистрация
```
curl -X POST http://localhost:8080/api/v1/register \
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
    "title": "Уютная квартира в центре",
    "description": "Светлая 2-комнатная квартира с ремонтом",
    "city": "Moscow",
    "address": "ул. Тверская, 15",
    "price": 15000000,
    "status": "active"
  }'
```

---

## 📝 Примеры запросов

#### Регистрация
```
curl -X POST http://localhost:8080/api/v1/register \
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
    "user_id": 2,
    "property_id": 101,
    "scheduled_time": "2024-02-01T14:00:00Z",
    "comment": "Хочу посмотреть зал и ванную"
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

## 📚 Документация

Swagger UI
```
http://localhost:8080/api/v1/docs
```

OpenAPI
```
http://localhost:8080/api/v1/docs/openapi.yaml
```

---

## 🧪 Тестирование

Для запуска тестов выполните следующие команды
```
pip install -r tests/requirements.txt  
pytest tests/test_api.py -v  
```