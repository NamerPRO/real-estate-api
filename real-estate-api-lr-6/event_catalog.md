# Каталог событий (Event Catalog)

## 1. UserRegistered

**Описание:** Генерируется при успешной регистрации нового пользователя в системе.

| Параметр | Значение |
| :--- | :--- |
| **Название события** | `user.registered` |
| **Производитель** | Auth Service / User Service |
| **Потребители** | Notification Service (приветственное письмо), Analytics Service |
| **Гарантии доставки** | At-least-once |

**Payload Structure (JSON):**
```json
{
    "user_id": 123,
    "login": "john_doe",
    "email": "john@example.com",
    "first_name": "John",
    "last_name": "Doe"
}
```

## 2. PropertyCreated

**Описание:** Генерируется при добавлении нового объекта недвижимости.

| Параметр | Значение |
| :--- | :--- |
| **Название события** | `property.created` |
| **Производитель** | Property Service |
| **Потребители** | Search Indexer, Cache Invalidator, Analytics Service |
| **Гарантии доставки** | At-least-once |

**Payload Structure (JSON):**
```json
{
    "owner_id": 1,
    "type": "commercial",
    "title": "Офисное помещение IT",
    "city": "Moscow",
    "address": {
        "street": "Leninsky Prospekt",
        "house": "45",
        "flat": "Office 301"
    },
    "details": {
        "purpose": "office",
        "area_sqm": 80,
        "floor": 3
    },
    "price": 500000,
    "features": [
        "parking",
        "security",
        "metro_nearby"
    ],
    "status": "active",
    "created_at": "2026-04-12T15:48:05.918+00:00"
}
```

---

## 3. PropertyStatusChanged

**Описание:** Генерируется при изменении статуса объекта (продажа, аренда, архив).

| Параметр | Значение |
| :--- | :--- |
| **Название события** | `property.status_changed` |
| **Производитель** | Property Service |
| **Потребители** | Search Indexer (удаление из активного поиска), Cache Invalidator |
| **Гарантии доставки** | At-least-once |

**Payload Structure (JSON):**
```json
{
    "property_id": "prop_abc123",
    "old_status": "active",
    "new_status": "sold",
    "updated_by_user_id": 123
}
```

---

## 4. ViewingScheduled

**Описание:** Генерируется при записи пользователя на просмотр объекта.

| Параметр | Значение |
| :--- | :--- |
| **Название события** | `viewing.scheduled` |
| **Производитель** | Viewing Service |
| **Потребители** | Notification Service (уведомление владельца и агента), Calendar Service |
| **Гарантии доставки** | At-least-once |

**Payload Structure (JSON):**
```json
{
  "property_id": "a6547b912ea14ad8b0443b1d00922517",
  "user_id": 1,
  "scheduled_time": "2026-05-10T12:00:00Z",
  "comments": [
    {
      "text": "Объект понравился",
      "author": "agent",
      "timestamp": "2026-04-12T12:00:00.000Z"
    }
  ]
}
```

---

## 5. ViewingCancelled

**Описание:** Генерируется при отмене записи на просмотр.

| Параметр | Значение |
| :--- | :--- |
| **Название события** | `viewing.cancelled` |
| **Производитель** | Viewing Service |
| **Потребители** | Notification Service (отмена напоминаний) |
| **Гарантии доставки** | At-least-once |

**Payload Structure (JSON):**
```json
{
    "viewing_id": "4fe7782d119b44af907639693b3b3560",
    "property_id": "a6547b912ea14ad8b0443b1d00922517",
    "user_id": 456,
    "reason": "change_of_plans"
}
```