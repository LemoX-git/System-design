-- 1. Создание нового пользователя
INSERT INTO users (login, first_name, last_name, password_hash)
VALUES ('new_chef', 'Ivan', 'Petrov', 'sha256_hash_here')
RETURNING id, login, first_name, last_name, created_at;

-- 2. Поиск пользователя по логину
SELECT id, login, first_name, last_name, created_at
FROM users
WHERE login = 'chef01';

-- 3. Поиск пользователя по маске имени и фамилии
SELECT id, login, first_name, last_name
FROM users
WHERE lower(first_name || ' ' || last_name) LIKE lower('%anna iva%')
ORDER BY id;

-- 4. Создание рецепта
INSERT INTO recipes (author_id, title, description)
VALUES (1, 'Khachapuri', 'Cheese flatbread from Georgia')
RETURNING id, author_id, title, description, created_at;

-- 5. Получение списка рецептов
SELECT r.id,
       r.author_id,
       r.title,
       r.description,
       COUNT(i.id)::int AS ingredients_count,
       r.created_at
FROM recipes r
LEFT JOIN ingredients i ON i.recipe_id = r.id
GROUP BY r.id, r.author_id, r.title, r.description, r.created_at
ORDER BY r.id;

-- 6. Поиск рецептов по названию
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

-- 7. Добавление ингредиента в рецепт
INSERT INTO ingredients (recipe_id, name, amount)
VALUES (1, 'Potato', '3 pcs')
RETURNING id, recipe_id, name, amount, created_at;

-- 8. Получение ингредиентов рецепта
SELECT id, recipe_id, name, amount, created_at
FROM ingredients
WHERE recipe_id = 1
ORDER BY id;

-- 9. Получение рецептов пользователя
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

-- 10. Добавление рецепта в избранное
INSERT INTO favorite_recipes (user_id, recipe_id)
VALUES (1, 10)
ON CONFLICT DO NOTHING;

-- 11. Получение избранных рецептов пользователя
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

-- 12. Логин пользователя и сохранение токена
SELECT id, login, first_name, last_name, password_hash
FROM users
WHERE login = 'chef01';

INSERT INTO auth_tokens (token, user_id, expires_at)
VALUES ('runtime-generated-token', 1, NOW() + INTERVAL '30 days');

-- 13. Валидация Bearer token
SELECT user_id
FROM auth_tokens
WHERE token = 'runtime-generated-token'
  AND (expires_at IS NULL OR expires_at > NOW());
