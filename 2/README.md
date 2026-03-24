# Recipe Management API — вариант 23

## Что реализовано

Сервис хранит в памяти:
- пользователей;
- рецепты;
- ингредиенты.

### Поддерживаемые операции
- регистрация нового пользователя;
- логин пользователя и получение JWT токена;
- поиск пользователя по логину;
- поиск пользователя по маске имени и фамилии;
- создание рецепта;
- получение списка рецептов;
- поиск рецептов по названию;
- добавление ингредиента в рецепт;
- получение ингредиентов рецепта;
- получение рецептов пользователя;
- добавление рецепта в избранное;
- получение избранных рецептов текущего пользователя.

## Стек
- Python 3.13
- FastAPI
- Uvicorn
- JWT (PyJWT)
- Pytest
- In-memory storage


## Запуск локально

```bash
python -m venv .venv
source .venv/bin/activate  # Linux/macOS
# .venv\Scripts\activate   # Windows
pip install -r requirements.txt
uvicorn app.main:app --reload
```

После запуска:
- Swagger UI: `http://127.0.0.1:8000/docs`
- Healthcheck: `http://127.0.0.1:8000/health`

## Запуск через Docker

```bash
docker compose up --build
```

## Аутентификация

Используется JWT Bearer Token.

### 1. Регистрация

```bash
curl -X POST http://127.0.0.1:8000/auth/register \
  -H "Content-Type: application/json" \
  -d '{
    "login": "chef_anna",
    "first_name": "Anna",
    "last_name": "Ivanova",
    "password": "StrongPass123"
  }'
```

### 2. Логин

```bash
curl -X POST http://127.0.0.1:8000/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "login": "chef_anna",
    "password": "StrongPass123"
  }'
```

В ответе придёт `access_token`.

## Примеры запросов

### Создание рецепта

```bash
curl -X POST http://127.0.0.1:8000/recipes \
  -H "Authorization: Bearer <TOKEN>" \
  -H "Content-Type: application/json" \
  -d '{
    "title": "Spaghetti Carbonara",
    "description": "Classic Italian pasta recipe",
    "instructions": "Boil pasta, fry pancetta, mix with eggs and cheese."
  }'
```

### Получение списка рецептов

```bash
curl http://127.0.0.1:8000/recipes
```

### Поиск рецептов по названию

```bash
curl "http://127.0.0.1:8000/recipes/search?title=Carbonara"
```

### Добавление ингредиента в рецепт

```bash
curl -X POST http://127.0.0.1:8000/recipes/<RECIPE_ID>/ingredients \
  -H "Authorization: Bearer <TOKEN>" \
  -H "Content-Type: application/json" \
  -d '{
    "name": "Parmesan",
    "quantity": 100,
    "unit": "g"
  }'
```

### Получение ингредиентов рецепта

```bash
curl http://127.0.0.1:8000/recipes/<RECIPE_ID>/ingredients
```

### Получение рецептов пользователя

```bash
curl http://127.0.0.1:8000/users/<USER_ID>/recipes
```

### Добавление рецепта в избранное

```bash
curl -X POST http://127.0.0.1:8000/recipes/<RECIPE_ID>/favorite \
  -H "Authorization: Bearer <TOKEN>"
```

## Защищённые endpoint'ы

JWT требуется для:
- `POST /recipes`
- `POST /recipes/{recipe_id}/ingredients`
- `POST /recipes/{recipe_id}/favorite`
- `GET /users/me/recipes`
- `GET /users/me/favorites`
- `DELETE /recipes/{recipe_id}/favorite`

Проверка токена выполняется через middleware, после чего текущий пользователь доступен в `request.state.user`.

## HTTP статус-коды

Основные коды:
- `200 OK` — успешное получение данных;
- `201 Created` — успешное создание ресурса;
- `204 No Content` — успешное удаление из избранного;
- `401 Unauthorized` — отсутствует или неверен токен;
- `403 Forbidden` — нет прав на изменение рецепта;
- `404 Not Found` — ресурс не найден;
- `409 Conflict` — логин уже существует;
- `422 Unprocessable Entity` — ошибка валидации входных данных.

## Тесты

Запуск тестов:

```bash
python -m pytest -v
```

Покрыты сценарии:
- регистрация и поиск пользователя;
- конфликт при повторной регистрации;
- запрет создания рецепта без токена;
- создание рецепта;
- добавление ингредиента;
- добавление в избранное;
- поиск рецептов;
- обработка 404 для несуществующего рецепта.

## Генерация OpenAPI файла

```bash
python -m scripts.generate_openapi
```

Файл будет сохранён как `openapi.yaml`.

## Ограничения текущей реализации

Так как в заднии не указано, то:
- используется in-memory хранилище без базы данных;
- данные пропадают после перезапуска приложения;
- хеширование пароля и секрет JWT заданы в коде для демонстрации.

В идеале следует вынести конфигурацию в переменные окружения и использовать PostgreSQL/SQLite.
