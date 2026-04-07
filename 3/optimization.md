# Оптимизация запросов для варианта 23

Ниже приведён набор запросов, для которых в схеме были заложены индексы. В этом архиве я не прикладываю выдуманные цифры `EXPLAIN ANALYZE`: файл содержит реальные запросы, которые нужно выполнить в PostgreSQL после поднятия контейнеров.

## 1. Поиск пользователя по логину

```sql
EXPLAIN (ANALYZE, BUFFERS)
SELECT id, login, first_name, last_name
FROM users
WHERE login = 'chef01';
```

**Почему быстро:** ограничение `UNIQUE` на `users(login)` автоматически создаёт B-tree индекс.

## 2. Поиск пользователя по маске имени и фамилии

```sql
EXPLAIN (ANALYZE, BUFFERS)
SELECT id, login, first_name, last_name
FROM users
WHERE lower(first_name || ' ' || last_name) LIKE lower('%anna iva%')
ORDER BY id;
```

**Оптимизация:** индекс `idx_users_full_name_trgm` на `lower(first_name || ' ' || last_name)` с `gin_trgm_ops`.

**Ожидаемый эффект:** вместо полного последовательного обхода таблицы PostgreSQL сможет использовать GIN index для поиска по подстроке.

## 3. Поиск рецептов по названию

```sql
EXPLAIN (ANALYZE, BUFFERS)
SELECT r.id,
       r.author_id,
       r.title,
       r.description,
       COUNT(i.id)::int AS ingredients_count
FROM recipes r
LEFT JOIN ingredients i ON i.recipe_id = r.id
WHERE lower(r.title) LIKE lower('%soup%')
GROUP BY r.id, r.author_id, r.title, r.description
ORDER BY r.id;
```

**Оптимизация:** индекс `idx_recipes_title_trgm`.

**Ожидаемый эффект:** ускорение поиска по частичному вхождению названия рецепта.

## 4. Получение ингредиентов рецепта

```sql
EXPLAIN (ANALYZE, BUFFERS)
SELECT id, recipe_id, name, amount
FROM ingredients
WHERE recipe_id = 1
ORDER BY id;
```

**Оптимизация:** индекс `idx_ingredients_recipe_id`.

**Ожидаемый эффект:** быстрый доступ ко всем ингредиентам конкретного рецепта по внешнему ключу.

## 5. Получение рецептов пользователя

```sql
EXPLAIN (ANALYZE, BUFFERS)
SELECT r.id,
       r.author_id,
       r.title,
       r.description,
       COUNT(i.id)::int AS ingredients_count
FROM recipes r
LEFT JOIN ingredients i ON i.recipe_id = r.id
WHERE r.author_id = 1
GROUP BY r.id, r.author_id, r.title, r.description
ORDER BY r.id;
```

**Оптимизация:** индекс `idx_recipes_author_id`.

**Ожидаемый эффект:** PostgreSQL быстро отберёт рецепты автора, а затем выполнит join с ингредиентами.

## 6. Получение избранного пользователя

```sql
EXPLAIN (ANALYZE, BUFFERS)
SELECT r.id,
       r.author_id,
       r.title,
       r.description,
       COUNT(i.id)::int AS ingredients_count
FROM favorite_recipes f
JOIN recipes r ON r.id = f.recipe_id
LEFT JOIN ingredients i ON i.recipe_id = r.id
WHERE f.user_id = 1
GROUP BY r.id, r.author_id, r.title, r.description
ORDER BY r.id;
```

**Оптимизация:** составной первичный ключ `(user_id, recipe_id)` и индекс `idx_favorite_recipes_recipe_id`.

**Ожидаемый эффект:** быстрое извлечение связей пользователя с рецептами и эффективный join к `recipes`.

## 7. Валидация токена

```sql
EXPLAIN (ANALYZE, BUFFERS)
SELECT user_id
FROM auth_tokens
WHERE token = 'runtime-generated-token'
  AND (expires_at IS NULL OR expires_at > NOW());
```

**Оптимизация:** первичный ключ по `token` и индекс `idx_auth_tokens_expires_at`.

**Ожидаемый эффект:** быстрый поиск токена при каждом авторизованном запросе.

## Как проверять локально

1. Поднять контейнеры:
   ```bash
   docker compose up --build
   ```
2. Зайти в БД:
   ```bash
   docker compose exec postgres psql -U recipe_user -d recipe_service
   ```
3. Выполнить `EXPLAIN (ANALYZE, BUFFERS)` для запросов выше.
4. При желании временно удалить нужный индекс и сравнить план до/после.
