# Recipe Service — вариант 23

REST API на `userver` для варианта 23 из задания: **пользователь / рецепт / ингредиент / избранное**.

## Что изменено

Проект переведён с in-memory хранения на PostgreSQL:

- данные пользователей хранятся в таблице `users`;
- рецепты хранятся в `recipes`;
- ингредиенты вынесены в отдельную таблицу `ingredients`;
- избранное реализовано через таблицу связи `favorite_recipes`;
- Bearer-токены хранятся в `auth_tokens`;
- пароли в БД сохраняются как SHA-256 hash.

## Структура файлов

- `schema.sql` — схема PostgreSQL, ограничения и индексы;
- `data.sql` — тестовые данные;
- `queries.sql` — SQL для всех операций варианта 23;
- `optimization.md` — список запросов для анализа через `EXPLAIN`;
- `docker-compose.yaml` — запуск API и PostgreSQL;
- `configs/config_vars.docker.yaml` — настройки для запуска в Docker.

## ER-модель

### `users`
- `id` — PK
- `login` — уникальный логин
- `first_name`, `last_name`
- `password_hash`
- `created_at`

### `recipes`
- `id` — PK
- `author_id` — FK на `users(id)`
- `title`
- `description`
- `created_at`

### `ingredients`
- `id` — PK
- `recipe_id` — FK на `recipes(id)`
- `name`
- `amount`
- `created_at`

### `favorite_recipes`
- `user_id` — FK на `users(id)`
- `recipe_id` — FK на `recipes(id)`
- `created_at`
- `PRIMARY KEY (user_id, recipe_id)`

### `auth_tokens`
- `token` — PK
- `user_id` — FK на `users(id)`
- `created_at`
- `expires_at`

## Запуск через Docker

```bash
docker compose up --build
```

После старта сервис доступен на `http://localhost:8080`.

PostgreSQL поднимается на `localhost:5432` со следующими параметрами:

Сидовые пользователи из `data.sql` имеют логины `chef01`…`chef10` и пароли `secret01`…`secret10`.

- database: `recipe_service`
- user: `recipe_user`
- password: `recipe_password`

## Полезные команды

Зайти в базу:

```bash
docker compose exec postgres psql -U recipe_user -d recipe_service
```

Проверить API регистрации:

```bash
curl -X POST http://localhost:8080/api/v1/auth/register \
  -H 'Content-Type: application/json' \
  -d '{
    "login":"chef11",
    "first_name":"Ivan",
    "last_name":"Petrov",
    "password":"secret"
  }'
```

Логин:

```bash
curl -X POST http://localhost:8080/api/v1/auth/login \
  -H 'Content-Type: application/json' \
  -d '{"login":"chef11","password":"secret"}'
```

Создание рецепта:

```bash
curl -X POST http://localhost:8080/api/v1/recipes \
  -H 'Authorization: Bearer <TOKEN>' \
  -H 'Content-Type: application/json' \
  -d '{"title":"Pasta","description":"Quick pasta"}'
```

## Замечания

- для старта контейнеров таблицы создаются из `schema.sql`;
- тестовые данные загружаются из `data.sql` только при первой инициализации пустого тома PostgreSQL;
- при инициализации также создаётся база `recipe_service_test` для testsuite;
- сам сервис также пытается выполнить `schema.sql` на старте, чтобы схема была создана и без init-скриптов;
- ошибки валидации и плохие path/body параметры возвращаются как `400`, а внутренние ошибки БД — как `500` с JSON-ответом.
