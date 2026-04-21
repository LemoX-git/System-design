# MongoDB schema design for recipe_service

## Goal
Вариант 23 из задания требует реализовать систему управления рецептами с сущностями:
- Пользователь
- Рецепт
- Ингредиент

Текущий HTTP API проекта уже соответствует этому варианту, поэтому при переходе на MongoDB сохранён существующий REST-контракт, а изменён только слой хранения данных.

## Collections

### 1. `users`
Пример документа:
```json
{
  "_id": {"$oid": "..."},
  "id": 1,
  "login": "chef_anna",
  "first_name": "Anna",
  "last_name": "Ivanova",
  "full_name_lc": "anna ivanova",
  "password_hash": "5f4dcc3b5aa765d61d8327deb882cf99",
  "favorite_recipe_ids": [1, 3, 7],
  "auth_tokens": [
    {
      "token": "token-1-1",
      "issued_at": {"$date": "2026-04-09T10:00:00Z"}
    }
  ],
  "created_at": {"$date": "2026-04-09T10:00:00Z"},
  "updated_at": {"$date": "2026-04-09T10:00:00Z"}
}
```

Индексы:
- `id` unique
- `login` unique
- `auth_tokens.token`
- `full_name_lc`

### 2. `recipes`
Пример документа:
```json
{
  "_id": {"$oid": "..."},
  "id": 10,
  "author_id": 1,
  "title": "Tomato Soup",
  "title_lc": "tomato soup",
  "description": "Classic домашний томатный суп",
  "ingredients": [
    {
      "id": 101,
      "name": "Tomato",
      "amount": "4 pcs",
      "created_at": {"$date": "2026-04-09T10:05:00Z"}
    },
    {
      "id": 102,
      "name": "Salt",
      "amount": "1 tsp",
      "created_at": {"$date": "2026-04-09T10:05:00Z"}
    }
  ],
  "created_at": {"$date": "2026-04-09T10:00:00Z"},
  "updated_at": {"$date": "2026-04-09T10:05:00Z"}
}
```

Индексы:
- `id` unique
- `author_id`
- `title_lc`

### 3. `counters`
Техническая коллекция для генерации последовательных числовых идентификаторов, чтобы сохранить текущий формат API:
```json
{
  "_id": "recipes",
  "seq": 10
}
```

Инкремент счётчика выполняется атомарно через Mongo `findAndModify` + `$inc` + `upsert`, поэтому выдача новых `id` корректна даже при нескольких экземплярах API.

## Embedded vs references

### `recipes.ingredients` — embedded documents
Ингредиенты встроены в документ рецепта, потому что:
- ингредиент существует только в контексте одного рецепта;
- основная операция — «добавить ингредиент в рецепт»;
- чтение ингредиентов почти всегда идёт вместе с рецептом;
- не нужен отдельный lifecycle и отдельная коллекция.

Такое решение уменьшает число запросов и делает обновление рецепта атомарным на уровне одного документа.

### `recipes.author_id -> users.id` — reference
Рецепт — самостоятельная сущность, пользователь — самостоятельная сущность. Их связь лучше хранить как reference:
- один пользователь создаёт много рецептов;
- пользователь и рецепт читаются независимо;
- исключается дублирование пользовательских данных в каждом рецепте.

### `users.favorite_recipe_ids -> recipes.id` — references array
Избранное — это связь many-to-many между пользователем и рецептами, но для текущего API достаточно хранить массив ссылок в пользователе:
- быстрое добавление через `$addToSet`;
- простой сценарий чтения «мои избранные рецепты»;
- нет дублирования тела рецепта.

## Почему нет отдельной коллекции `ingredients`
По варианту задания ингредиент является сущностью предметной области, но в документной модели MongoDB сущность не обязана быть отдельной коллекцией. Здесь ингредиенты логично моделируются как embedded documents внутри `recipes`, потому что не используются независимо от рецепта.

## Типы данных MongoDB
Используются:
- `String`: login, title, description, ingredient name;
- `Number`: id, author_id, seq;
- `Date`: created_at, updated_at, issued_at;
- `Array`: ingredients, favorite_recipe_ids, auth_tokens;
- `Object`: embedded ingredient/token documents.
