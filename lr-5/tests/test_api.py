"""
Real Estate API — Комплексные тесты
Покрывает все успешные сценарии и ошибки согласно OpenAPI спецификации.

Запуск:
    pytest tests/test_api.py -v
    pytest tests/test_api.py -v --tb=short  # Краткий вывод ошибок
"""

import pytest
import requests
import time
import nanoid
import copy
from typing import Dict, Any
from datetime import datetime, timezone

# =============================================================================
# CONFIGURATION
# =============================================================================

BASE_URL = "http://localhost:8080/api/v1"
TIMEOUT = 10  # seconds

BASE_TEST_USER = {
    "login": "test_user",
    "password": "password123",
    "first_name": "Test",
    "last_name": "User",
    "email": "test@example.com"
}

BASE_TEST_PROPERTY = {
    "owner_id": 0,
    "type": "apartment",
    "title": "Test Apartment Title",
    "city": "Moscow",
    "address": {
        "street": "Test Street",
        "house": "1",
        "flat": "1"
    },
    "price": 10000000,
    "details": {
        "rooms": 1,
        "floor": 5,
        "total_floors": 5,
        "has_elevator": "false"
    },
    "features": [
        "internet"
    ],
    "status": "active"
}

BASE_TEST_VIEWING = {
    "scheduled_time": "2026-04-16T10:00:00Z",
    "comments": [
        {
            "text": "Test comment for viewing",
            "author": "user",
            "timestamp": "2026-04-15T12:00:00Z"
        }
    ]
}


# =============================================================================
# HELPER FUNCTIONS
# =============================================================================

def generate_unique_id() -> str:
    alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    return nanoid.generate(alphabet, 8)


def normalize(dt_str: str) -> datetime:
    # handle Z and +00:00 uniformly
    return datetime.fromisoformat(dt_str.replace("Z", "+00:00")).astimezone(timezone.utc)


def generate_unique_string(prefix: str = "") -> str:
    """Генерирует уникальную строку для тестовых данных"""
    return f"{prefix}_{generate_unique_id()}"


def create_test_user_data() -> Dict[str, Any]:
    """Создает уникальные данные тестового пользователя"""
    unique_id = generate_unique_id()
    return {
        "login": f"user_{unique_id}",
        "password": "password123",
        "first_name": "Test",
        "last_name": "User",
        "email": f"test_{unique_id}@example.com"
    }


def create_test_property_data(owner_id: int) -> Dict[str, Any]:
    """Создает данные тестового объекта недвижимости"""
    unique_id = generate_unique_id()
    data = copy.deepcopy(BASE_TEST_PROPERTY)
    data["city"] = f"Test City {unique_id}" 
    data["owner_id"] = owner_id
    data["title"] = f"{BASE_TEST_PROPERTY['title']}_{unique_id}"
    return data


def create_test_viewing_data(user_id: int, property_id: str) -> Dict[str, Any]:
    """Создает данные тестового просмотра"""
    data = BASE_TEST_VIEWING.copy()
    data["user_id"] = user_id
    data["property_id"] = property_id
    return data


def assert_error_response(response: requests.Response, expected_code: str):
    """Проверка формата ответа об ошибке"""
    
    assert response.status_code in [400, 401, 403, 404, 409], \
        f"Expected error status code, got {response.status_code}"
    
    if response.headers.get("Content-Type", "").startswith("application/json"):
        data = response.json()
        assert "code" in data, "Error response missing 'code' field"
        assert "message" in data, "Error response missing 'message' field"
        assert data["code"] == expected_code, f"Expected code '{expected_code}', got '{data['code']}'"
    else:
        assert len(response.text) > 0 or len(response.content) > 0


def assert_success_response(response: requests.Response, expected_status: int):
    """Проверка успешного ответа"""
    assert response.status_code == expected_status, \
        f"Expected {expected_status}, got {response.status_code}. Body: {response.text}"
    
    if response.headers.get("Content-Type", "").startswith("application/json"):
        data = response.json()
        assert data is not None
    else:
        assert len(response.text) > 0 or len(response.content) > 0


# =============================================================================
# FIXTURES
# =============================================================================

@pytest.fixture(scope="session")
def api_base_url() -> str:
    """Базовый URL API"""
    return BASE_URL


@pytest.fixture(scope="function")
def session() -> requests.Session:
    """Сессия requests с автоматическими заголовками"""
    session = requests.Session()
    session.headers.update({"Content-Type": "application/json"})
    yield session
    session.close()


@pytest.fixture(scope="function")
def registered_user(session: requests.Session, api_base_url: str) -> Dict[str, Any]:
    """Фикстура: регистрирует и возвращает данные пользователя"""
    user_data = create_test_user_data()
    
    response = session.post(
        f"{api_base_url}/users",
        json=user_data,
        timeout=TIMEOUT
    )
    
    assert response.status_code == 201, f"Registration failed: {response.text}"
    user_id = response.json()["id"]
    
    return {
        "id": user_id,
        "login": user_data["login"],
        "password": user_data["password"],
        "data": user_data
    }


@pytest.fixture(scope="function")
def auth_token(session: requests.Session, api_base_url: str, registered_user: Dict[str, Any]) -> str:
    """Фикстура: логинится и возвращает токен"""
    response = session.post(
        f"{api_base_url}/login",
        json={
            "login": registered_user["login"],
            "password": registered_user["password"]
        },
        timeout=TIMEOUT
    )
    
    assert response.status_code == 200, f"Login failed: {response.text}"
    return response.json()["access_token"]


@pytest.fixture(scope="function")
def auth_headers(auth_token: str) -> dict:
    """Заголовки с авторизацией"""
    return {"Authorization": f"Bearer {auth_token}"}


@pytest.fixture(scope="function")
def test_user_with_token(session: requests.Session, api_base_url: str) -> Dict[str, Any]:
    """Фикстура: возвращает полные данные пользователя с токеном"""
    user_data = create_test_user_data()
    
    response = session.post(
        f"{api_base_url}/users",
        json=user_data,
        timeout=TIMEOUT
    )
    assert response.status_code == 201
    user_id = response.json()["id"]
    
    response = session.post(
        f"{api_base_url}/login",
        json={"login": user_data["login"], "password": user_data["password"]},
        timeout=TIMEOUT
    )
    assert response.status_code == 200
    token = response.json()["access_token"]
    
    return {
        "id": user_id,
        "login": user_data["login"],
        "password": user_data["password"],
        "token": token,
        "headers": {"Authorization": f"Bearer {token}"}
    }


@pytest.fixture(scope="function")
def created_property(
    session: requests.Session,
    api_base_url: str,
    test_user_with_token: Dict[str, Any]
) -> dict:
    """Фикстура: создаёт тестовый объект недвижимости"""
    property_data = create_test_property_data(test_user_with_token["id"])
    
    response = session.post(
        f"{api_base_url}/properties",
        json=property_data,
        headers=test_user_with_token["headers"],
        timeout=TIMEOUT
    )
    
    assert response.status_code == 201, f"Property creation failed: {response.text}"
    return response.json()


# =============================================================================
# TESTS: AUTH ENDPOINTS
# =============================================================================

class TestAuthEndpoints:
    """Тесты эндпоинта аутентификации: /login"""
    
    def test_login_success(self, session: requests.Session, api_base_url: str, registered_user: Dict[str, Any]):
        """Успешный логин с получением токена"""
        response = session.post(
            f"{api_base_url}/login",
            json={
                "login": registered_user["login"],
                "password": registered_user["password"]
            },
            timeout=TIMEOUT
        )
        
        assert_success_response(response, 200)
        data = response.json()
        assert "access_token" in data
        assert data.get("token_type") == "Bearer"
        assert data.get("login") == registered_user["login"]
        assert isinstance(data.get("user_id"), int)
    
    def test_login_wrong_password(self, session: requests.Session, api_base_url: str, registered_user: Dict[str, Any]):
        """Логин с неправильным паролем -> 401"""
        response = session.post(
            f"{api_base_url}/login",
            json={"login": registered_user["login"], "password": "wrong_password"},
            timeout=TIMEOUT
        )
        
        assert response.status_code == 401
        assert_error_response(response, "UNAUTHORIZED")
    
    def test_login_nonexistent_user(self, session: requests.Session, api_base_url: str):
        """Логин несуществующего пользователя -> 401"""
        response = session.post(
            f"{api_base_url}/login",
            json={"login": "nonexistent_user_12345", "password": "any_password"},
            timeout=TIMEOUT
        )
        
        assert response.status_code == 401
        assert_error_response(response, "UNAUTHORIZED")
    
    def test_login_missing_credentials(self, session: requests.Session, api_base_url: str):
        """Логин без обязательных полей -> 400"""
        response = session.post(
            f"{api_base_url}/login",
            json={"login": "test"},
            timeout=TIMEOUT
        )
        
        assert response.status_code == 400
        assert_error_response(response, "VALIDATION_ERROR")


# =============================================================================
# TESTS: USER ENDPOINTS
# =============================================================================

class TestUserEndpoints:
    """Тесты эндпоинтов пользователей: /users"""
    
    def test_create_user_success(self, session: requests.Session, api_base_url: str):
        """Успешная регистрация нового пользователя"""
        user_data = create_test_user_data()
        
        response = session.post(
            f"{api_base_url}/users",
            json=user_data,
            timeout=TIMEOUT
        )
        
        assert_success_response(response, 201)
        data = response.json()
        assert data["login"] == user_data["login"]
        assert data["email"] == user_data["email"]
        assert "id" in data
        assert "created_at" in data
    
    def test_create_user_missing_required_fields(self, session: requests.Session, api_base_url: str):
        """Регистрация без обязательных полей -> 400"""
        invalid_data = {"login": "test"}
        
        response = session.post(
            f"{api_base_url}/users",
            json=invalid_data,
            timeout=TIMEOUT
        )
        
        assert response.status_code == 400
        assert_error_response(response, "VALIDATION_ERROR")
    
    def test_create_user_login_already_exists(self, session: requests.Session, api_base_url: str, registered_user: Dict[str, Any]):
        """Повторная регистрация с существующим логином -> 409"""
        user_data = {
            "login": registered_user["login"],
            "password": "password123",
            "first_name": "Another",
            "last_name": "User",
            "email": "another@example.com"
        }
        
        response = session.post(
            f"{api_base_url}/users",
            json=user_data,
            timeout=TIMEOUT
        )
        
        assert response.status_code == 409
        assert_error_response(response, "CONFLICT")
    
    def test_create_user_email_already_exists(self, session: requests.Session, api_base_url: str, registered_user: Dict[str, Any]):
        """Повторная регистрация с существующим email -> 409"""
        user_data = {
            "login": "AnotherLogin",
            "password": "password123",
            "first_name": "Another",
            "last_name": "User",
            "email": registered_user["data"]["email"]
        }
        
        response = session.post(
            f"{api_base_url}/users",
            json=user_data,
            timeout=TIMEOUT
        )
        
        assert response.status_code == 409
        assert_error_response(response, "CONFLICT")
    
    def test_search_users_by_login_success(self, session: requests.Session, api_base_url: str, registered_user: Dict[str, Any]):
        """Поиск пользователя по точному логину -> 200"""
        response = session.get(
            f"{api_base_url}/users",
            params={
                "from": 1,
                "to": 10,
                "login": registered_user["login"]
            },
            timeout=TIMEOUT
        )
        
        assert_success_response(response, 200)
        data = response.json()
        assert isinstance(data, list)
        assert len(data) >= 1
        assert any(u["login"] == registered_user["login"] for u in data)
    
    def test_search_users_by_login_not_found(self, session: requests.Session, api_base_url: str):
        """Поиск несуществующего пользователя по логину -> 404"""
        response = session.get(
            f"{api_base_url}/users",
            params={
                "from": 1,
                "to": 10,
                "login": "nonexistent_login_xyz"
            },
            timeout=TIMEOUT
        )
        
        assert response.status_code == 404
        assert_error_response(response, "NOT_FOUND")
    
    def test_search_users_no_query_params(self, session: requests.Session, api_base_url: str):
        """Поиск без параметров -> 400"""
        response = session.get(f"{api_base_url}/users", timeout=TIMEOUT)
        
        assert response.status_code == 400
        assert_error_response(response, "BAD_REQUEST")


# =============================================================================
# TESTS: PROPERTY ENDPOINTS
# =============================================================================

class TestPropertyEndpoints:
    """Тесты эндпоинтов объектов недвижимости: /properties"""
    
    def test_create_property_with_auth_success(self, session: requests.Session, api_base_url: str, test_user_with_token: Dict[str, Any]):
        """Создание объекта с аутентификацией -> 201"""
        property_data = create_test_property_data(test_user_with_token["id"])
        
        response = session.post(
            f"{api_base_url}/properties",
            json=property_data,
            headers=test_user_with_token["headers"],
            timeout=TIMEOUT
        )
        
        assert response.status_code == 201, f"Expected 201, got {response.status_code}"
        
        if response.headers.get("Content-Type", "").startswith("application/json"):
            data = response.json()
            assert data["title"] == property_data["title"]
            assert data["city"] == property_data["city"]
            assert "id" in data
            assert "created_at" in data
    
    def test_create_property_without_auth(self, session: requests.Session, api_base_url: str):
        """Создание объекта без токена -> 401"""
        property_data = create_test_property_data(owner_id=1)
        
        response = session.post(
            f"{api_base_url}/properties",
            json=property_data,
            timeout=TIMEOUT
        )
        
        assert response.status_code == 401, f"Expected 401, got {response.status_code}"
    
    def test_create_property_invalid_owner_id(self, session: requests.Session, api_base_url: str, auth_headers: dict):
        """Создание объекта с несуществующим owner_id -> 404"""
        property_data = create_test_property_data(owner_id=999999)
        
        response = session.post(
            f"{api_base_url}/properties",
            json=property_data,
            headers=auth_headers,
            timeout=TIMEOUT
        )
        
        assert response.status_code == 404
        assert response.status_code == 404
    
    def test_create_property_missing_required_fields(self, session: requests.Session, api_base_url: str, auth_headers: dict):
        """Создание объекта без обязательных полей -> 400"""
        invalid_property = {"title": "Incomplete"}
        
        response = session.post(
            f"{api_base_url}/properties",
            json=invalid_property,
            headers=auth_headers,
            timeout=TIMEOUT
        )
        
        assert response.status_code == 400
    
    def test_search_properties_by_city_success(self, session: requests.Session, api_base_url: str, created_property: dict):
        """Поиск объектов по городу -> 200"""
        response = session.get(
            f"{api_base_url}/properties",
            params={
                "from": 1,
                "to": 10,
                "city": created_property["city"]
            },
            timeout=TIMEOUT
        )
        
        assert_success_response(response, 200)
        data = response.json()
        assert isinstance(data, list)
        assert len(data) >= 1
    
    def test_search_properties_no_params(self, session: requests.Session, api_base_url: str):
        """Поиск без параметров -> 400"""
        response = session.get(f"{api_base_url}/properties", timeout=TIMEOUT)
        
        assert response.status_code == 400
        assert_error_response(response, "BAD_REQUEST")
    
    def test_search_properties_invalid_price_format(self, session: requests.Session, api_base_url: str):
        """Поиск с невалидным форматом цены -> 400"""
        response = session.get(
            f"{api_base_url}/properties",
            params={
                "from": 1,
                "to": 10,
                "min_price": "not_a_number",
                "max_price": "10000000"
            },
            timeout=TIMEOUT
        )
        
        assert response.status_code == 400
        assert_error_response(response, "VALIDATION_ERROR")


# =============================================================================
# TESTS: VIEWING ENDPOINTS
# =============================================================================

class TestViewingEndpoints:
    """Тесты эндпоинтов записей на просмотр: /properties/{id}/viewings"""
    
    def test_create_viewing_with_auth_success(self, session: requests.Session, api_base_url: str, test_user_with_token: Dict[str, Any], created_property: dict):
        """Запись на просмотр с аутентификацией -> 201"""
        viewing_data = create_test_viewing_data(
            test_user_with_token["id"],
            created_property["id"]
        )
        
        response = session.post(
            f"{api_base_url}/properties/{created_property['id']}/viewings",
            json=viewing_data,
            headers=test_user_with_token["headers"],
            timeout=TIMEOUT
        )
        
        assert response.status_code == 201, f"Expected 201, got {response.status_code}"
        
        if response.headers.get("Content-Type", "").startswith("application/json"):
            data = response.json()
            assert data["property_id"] == created_property["id"]
            assert normalize(data["scheduled_time"]) == normalize(viewing_data["scheduled_time"])
    
    def test_create_viewing_without_auth(self, session: requests.Session, api_base_url: str, created_property: dict, test_user_with_token: Dict[str, Any]):
        """Запись на просмотр без токена -> 401"""
        viewing_data = create_test_viewing_data(
            test_user_with_token["id"],
            created_property["id"]
        )
        
        response = session.post(
            f"{api_base_url}/properties/{created_property['id']}/viewings",
            json=viewing_data,
            timeout=TIMEOUT
        )
        
        assert response.status_code == 401, f"Expected 401, got {response.status_code}"
    
    def test_create_viewing_nonexistent_property(self, session: requests.Session, api_base_url: str, test_user_with_token: Dict[str, Any]):
        """Запись на просмотр несуществующего объекта -> 400"""
        viewing_data = create_test_viewing_data(
            test_user_with_token["id"],
            999999999
        )
        
        response = session.post(
            f"{api_base_url}/properties/999999/viewings",
            json=viewing_data,
            headers=test_user_with_token["headers"],
            timeout=TIMEOUT
        )
        
        assert response.status_code == 400
    
    def test_create_viewing_missing_required_fields(self, session: requests.Session, api_base_url: str, auth_headers: dict, created_property: dict):
        """Запись на просмотр без обязательных полей -> 400"""
        invalid_viewing = {"user_id": 1}
        
        response = session.post(
            f"{api_base_url}/properties/{created_property['id']}/viewings",
            json=invalid_viewing,
            headers=auth_headers,
            timeout=TIMEOUT
        )
        
        assert response.status_code == 400


# =============================================================================
# TESTS: DOCS ENDPOINTS
# =============================================================================

class TestDocsEndpoints:
    """Тесты эндпоинтов документации: /docs, /docs/openapi.yaml"""
    
    def test_get_openapi_spec_success(self, session: requests.Session, api_base_url: str):
        """Получение OpenAPI спецификации -> 200"""
        response = session.get(f"{api_base_url}/docs/openapi.yaml", timeout=TIMEOUT)
        
        assert response.status_code == 200
        assert "openapi:" in response.text or "swagger:" in response.text
    
    def test_get_swagger_ui_success(self, session: requests.Session, api_base_url: str):
        """Получение Swagger UI -> 200"""
        response = session.get(f"{api_base_url}/docs", timeout=TIMEOUT)
        
        assert response.status_code == 200
        assert response.headers["Content-Type"].startswith("text/html")
        assert "<!DOCTYPE html>" in response.text or "<html" in response.text


# =============================================================================
# TESTS: EDGE CASES & INTEGRATION
# =============================================================================

class TestEdgeCases:
    """Пограничные случаи и интеграционные тесты"""
    
    def test_full_workflow(self, session: requests.Session, api_base_url: str):
        """Полный сценарий: регистрация -> логин -> создание объекта -> запись на просмотр"""
        user_data = create_test_user_data()
        response = session.post(f"{api_base_url}/users", json=user_data, timeout=TIMEOUT)
        assert response.status_code == 201
        user_id = response.json()["id"]
        
        response = session.post(
            f"{api_base_url}/login",
            json={"login": user_data["login"], "password": user_data["password"]},
            timeout=TIMEOUT
        )
        assert response.status_code == 200
        token = response.json()["access_token"]
        headers = {"Authorization": f"Bearer {token}"}
        
        property_data = create_test_property_data(user_id)
        response = session.post(
            f"{api_base_url}/properties",
            json=property_data,
            headers=headers,
            timeout=TIMEOUT
        )
        assert response.status_code == 201
        property_id = response.json()["id"]
        
        viewing_data = create_test_viewing_data(user_id, property_id)
        response = session.post(
            f"{api_base_url}/properties/{property_id}/viewings",
            json=viewing_data,
            headers=headers,
            timeout=TIMEOUT
        )
        assert response.status_code == 201
        
        response = session.get(
            f"{api_base_url}/properties?from=1&to=10&city={property_data['city']}",
            timeout=TIMEOUT
        )
        assert response.status_code == 200
        assert any(p["id"] == property_id for p in response.json())
    
    def test_token_expiration_simulation(self, session: requests.Session, api_base_url: str):
        """Тест на просроченный токен (симуляция через невалидный токен)"""
        headers = {"Authorization": "Bearer expired_token_simulation"}
        property_data = create_test_property_data(1)
        
        response = session.post(
            f"{api_base_url}/properties",
            json=property_data,
            headers=headers,
            timeout=TIMEOUT
        )
        
        assert response.status_code in [401, 403], f"Expected 401 or 403, got {response.status_code}"


# =============================================================================
# WAIT FOR API FIXTURE
# =============================================================================

@pytest.fixture(autouse=True, scope="session")
def wait_for_api(api_base_url: str):
    """Фикстура: ожидание готовности API перед запуском тестов"""
    max_retries = 30
    timeout = 5
    
    for attempt in range(max_retries):
        try:
            response = requests.get(f"{api_base_url}/docs", timeout=timeout)
            if response.status_code == 200:
                return
        except (requests.exceptions.ConnectionError, requests.exceptions.ReadTimeout) as e:
            if attempt == max_retries - 1:
                pytest.fail(f"API не доступен по адресу {api_base_url} после {max_retries} попыток")
            time.sleep(2)


# =============================================================================
# PYTEST CONFIGURATION
# =============================================================================

def pytest_configure(config):
    """Дополнительная конфигурация pytest"""
    config.addinivalue_line(
        "markers", "slow: mark test as slow (for long-running tests)"
    )