# Recipe service with MongoDB

Сервис реализует вариант 23 домашнего задания: **система управления рецептами**.

## Что сделано
- существующий API на C++/userver переведён с in-memory storage на MongoDB;
- добавлен Docker Compose для запуска API + MongoDB;
- спроектирована документная модель (`schema_design.md`);
- подготовлены тестовые данные (`data.js`);
- подготовлены CRUD-запросы и агрегация (`queries.js`);
- подготовлена валидация схемы для коллекции `recipes` (`validation.js`).

## Стек
- C++20
- userver
- MongoDB
- Docker / Docker Compose
- pytest + testsuite userver

## Структура MongoDB
- `users`
- `recipes`
- `counters`

Ингредиенты хранятся **embedded** внутри документов `recipes`.

## Запуск через Docker
```bash
docker compose up --build
```

После старта:
- API: `http://localhost:8080`
- MongoDB: `mongodb://localhost:27017/recipe_service`

## Инициализация MongoDB
При загрузке seed-данных у всех предсозданных пользователей пароль: `test123`.

При старте контейнера Mongo выполняются скрипты из `mongo/init`:
- `01-validation.js` — создаёт коллекции, индексы и validator;
- `02-seed.js` — загружает тестовые данные.

## Локальный запуск Mongo-скриптов вручную
```bash
mongosh mongodb://localhost:27017/recipe_service validation.js
mongosh mongodb://localhost:27017/recipe_service data.js
mongosh mongodb://localhost:27017/recipe_service queries.js
```

## Основные API-эндпоинты
- `POST /api/v1/auth/register`
- `POST /api/v1/auth/login`
- `GET /api/v1/users/by-login`
- `GET /api/v1/users/search`
- `POST /api/v1/recipes`
- `GET /api/v1/recipes`
- `POST /api/v1/recipes/{recipe_id}/ingredients`
- `GET /api/v1/recipes/{recipe_id}/ingredients`
- `GET /api/v1/users/me/recipes`
- `POST /api/v1/users/me/favorites/{recipe_id}`
- `GET /api/v1/users/me/favorites`

## Тесты
В `tests/conftest.py` подключён `pytest_userver.plugins.mongo`. Перед каждым тестом Mongo очищается, чтобы тесты оставались изолированными.

## Примечания
- Для совместимости с текущим API сервис продолжает использовать числовые `id`, а не `ObjectId`, поэтому добавлена техническая коллекция `counters`.
- Генерация новых `id` реализована атомарно через Mongo `findAndModify`/`$inc` с upsert, чтобы не было гонок между несколькими инстансами сервиса.
- В пароле сохраняется `password_hash`, но текущая реализация намеренно упрощённая и сделана для учебного проекта.
- Для воспроизводимых seed/test-данных используется детерминированное преобразование `pw$<password>` вместо `std::hash`, чтобы логин работал одинаково из C++ и Mongo-скриптов.
