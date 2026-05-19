# Event-Driven архитектура сервиса управления рецептами

## 1. Контекст варианта

Вариант 23: **система управления рецептами**. Система хранит пользователей, рецепты и ингредиенты. Базовые API операции:

- создание нового пользователя;
- поиск пользователя по логину;
- поиск пользователя по маске имени и фамилии;
- создание рецепта;
- получение списка рецептов;
- поиск рецептов по названию;
- добавление ингредиента в рецепт;
- получение ингредиентов рецепта;
- получение рецептов пользователя;
- добавление рецепта в избранное.


## 2. Команды и события

Команда — это действие пользователя или внешнего клиента, которое меняет состояние системы. Событие — факт, который уже произошёл после успешного выполнения команды.

| Команда | HTTP API | Событие | Комментарий |
|---|---|---|---|
| `RegisterUser` | `POST /api/v1/auth/register` | `UserRegistered` | Пользователь успешно создан. |
| `CreateRecipe` | `POST /api/v1/recipes` | `RecipeCreated` | Автор создал новый рецепт. |
| `AddIngredientToRecipe` | `POST /api/v1/recipes/{recipe_id}/ingredients` | `IngredientAdded` | Автор рецепта добавил ингредиент. |
| `AddRecipeToFavorites` | `POST /api/v1/users/me/favorites/{recipe_id}` | `FavoriteRecipeAdded` | Пользователь добавил рецепт в избранное. |

Операции чтения (`GET`) не создают доменных событий, потому что они не изменяют состояние системы.

## 3. Сервисы и компоненты

### Write-side сервис

`recipe_service` — основной HTTP-сервис:

- принимает команды через REST API;
- валидирует входные данные;
- пишет основные данные в PostgreSQL;
- после успешного изменения состояния записывает событие в таблицу `event_outbox`.

### Outbox producer

`event_producer` — отдельный producer:

- периодически читает новые записи из `event_outbox`;
- публикует сообщения в RabbitMQ exchange `recipe.events`;
- после успешной публикации переводит событие в статус `PUBLISHED`;
- при ошибке увеличивает счётчик `attempts`, сохраняет `last_error`, после превышения лимита переводит событие в `FAILED`.

### RabbitMQ

`rabbitmq` — брокер сообщений:

- exchange: `recipe.events`;
- тип exchange: `topic`;
- routing keys:
  - `user.registered`;
  - `recipe.created`;
  - `ingredient.added`;
  - `favorite.recipe.added`.

### Consumer / read-model сервис

`event_consumer` — простой consumer:

- подписывается на очередь `recipe.events.read_model`;
- получает сообщения из RabbitMQ;
- сохраняет все обработанные события в `event_log`;
- обновляет CQRS read-модель:
  - `recipe_read_model`;
  - `recipe_ingredient_read_model`;
  - `user_favorite_recipe_read_model`.

## 4. Поток событий

Общий поток обработки команды выглядит так:

```text
HTTP client
    |
    | command
    v
recipe_service
    |
    | write transaction / state change
    v
PostgreSQL main tables
    |
    | insert integration event
    v
event_outbox
    |
    | polling
    v
event_producer
    |
    | publish JSON message
    v
RabbitMQ topic exchange recipe.events
    |
    | route by routing_key
    v
event_consumer
    |
    | update projections
    v
CQRS read-model tables + event_log
```

Пример для создания рецепта:

1. Клиент вызывает `POST /api/v1/recipes`.
2. `recipe_service` создаёт запись в таблице `recipes`.
3. `recipe_service` записывает событие `RecipeCreated` в `event_outbox`.
4. `event_producer` публикует сообщение с routing key `recipe.created` в RabbitMQ.
5. `event_consumer` получает сообщение и создаёт/обновляет запись в `recipe_read_model`.

## 5. Формат сообщений

Все события публикуются в формате JSON. Сообщение состоит из envelope и payload:

```json
{
  "event_id": 1,
  "event_type": "RecipeCreated",
  "event_version": 1,
  "occurred_at": "2026-05-18T10:00:00+00:00",
  "producer": "recipe-service",
  "aggregate": {
    "type": "recipe",
    "id": 42
  },
  "payload": {
    "recipe_id": 42,
    "author_id": 7,
    "title": "Pancakes",
    "description": "Thin pancakes with milk"
  }
}
```

Metadata хранится в envelope, а бизнес-данные — внутри `payload`.

## 6. Гарантии доставки

Выбрана гарантия **at-least-once**:

- событие сначала сохраняется в PostgreSQL outbox;
- producer публикует событие в RabbitMQ с persistent delivery mode;
- событие помечается как `PUBLISHED` только после успешной публикации;
- consumer подтверждает сообщение `ack` только после успешной записи в PostgreSQL;
- при ошибке обработки consumer выполняет `nack` с повторной постановкой в очередь.

Такая схема допускает повторную доставку сообщения, поэтому consumer сделан идемпотентным: таблица `event_log` имеет первичный ключ по `event_id`, а read-модель использует `ON CONFLICT`.

`exactly-once` в данной реализации не используется, потому что для HTTP-сервиса + PostgreSQL + RabbitMQ это значительно усложнило бы архитектуру. Практически достаточно `at-least-once` + идемпотентной обработки.

## 7. Применение CQRS

CQRS применим, потому что операции записи и чтения имеют разный характер:

- команды изменяют состояние и требуют валидации прав;
- запросы должны быстро отдавать данные пользователю;
- для чтения удобно иметь отдельные проекции, уже подготовленные под конкретные сценарии.

### Write model

Write model находится в основных таблицах:

- `users`;
- `recipes`;
- `ingredients`;
- `favorite_recipes`;
- `auth_tokens`.

Она используется командами и содержит нормализованную структуру данных.

### Read model

Read model находится в таблицах:

- `recipe_read_model` — рецепты с количеством ингредиентов;
- `recipe_ingredient_read_model` — ингредиенты рецептов;
- `user_favorite_recipe_read_model` — избранные рецепты пользователей.

Она обновляется асинхронно через события. Например, `IngredientAdded` добавляет строку в `recipe_ingredient_read_model` и пересчитывает `ingredients_count` в `recipe_read_model`.
