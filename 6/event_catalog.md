# Event Catalog

## Общие параметры сообщений

Все события публикуются в RabbitMQ exchange `recipe.events` типа `topic`.

Общий envelope:

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
  "payload": {}
}
```

Гарантия доставки для всех событий: **at-least-once**. Consumer должен быть идемпотентным.

## 1. UserRegistered

| Поле | Значение |
|---|---|
| Название события | `UserRegistered` |
| Routing key | `user.registered` |
| Aggregate | `user` |
| Producer | `recipe_service` через `event_outbox` + `event_producer` |
| Consumers | `event_consumer`, потенциально notification/audit сервис |
| Delivery | `at-least-once` |

Payload:

```json
{
  "user_id": 7,
  "login": "chef07",
  "first_name": "Elena",
  "last_name": "Morozova"
}
```

Назначение: уведомить систему, что создан новый пользователь.

## 2. RecipeCreated

| Поле | Значение |
|---|---|
| Название события | `RecipeCreated` |
| Routing key | `recipe.created` |
| Aggregate | `recipe` |
| Producer | `recipe_service` через `event_outbox` + `event_producer` |
| Consumers | `event_consumer`, read-model сервис, audit сервис |
| Delivery | `at-least-once` |

Payload:

```json
{
  "recipe_id": 42,
  "author_id": 7,
  "title": "Pancakes",
  "description": "Thin pancakes with milk"
}
```

Назначение: создать или обновить проекцию рецепта в read model.

## 3. IngredientAdded

| Поле | Значение |
|---|---|
| Название события | `IngredientAdded` |
| Routing key | `ingredient.added` |
| Aggregate | `recipe` |
| Producer | `recipe_service` через `event_outbox` + `event_producer` |
| Consumers | `event_consumer`, read-model сервис, notification/audit сервис |
| Delivery | `at-least-once` |

Payload:

```json
{
  "ingredient_id": 101,
  "recipe_id": 42,
  "author_id": 7,
  "name": "Milk",
  "amount": "500 ml"
}
```

Назначение: добавить ингредиент в read model и пересчитать количество ингредиентов рецепта.

## 4. FavoriteRecipeAdded

| Поле | Значение |
|---|---|
| Название события | `FavoriteRecipeAdded` |
| Routing key | `favorite.recipe.added` |
| Aggregate | `favorite_recipe` |
| Producer | `recipe_service` через `event_outbox` + `event_producer` |
| Consumers | `event_consumer`, recommendation/audit сервис |
| Delivery | `at-least-once` |

Payload:

```json
{
  "user_id": 7,
  "recipe_id": 42
}
```

Назначение: обновить read model избранных рецептов пользователя и дать возможность другим сервисам реагировать на интерес пользователя к рецепту.
