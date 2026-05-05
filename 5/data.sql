INSERT INTO users (id, login, first_name, last_name, password_hash) VALUES
    (1, 'chef01', 'Ivan', 'Petrov', '3064ce319bdba6c59a5610f4bd767cdd557ffed0c700c170c6612fe0863485da'),
    (2, 'chef02', 'Anna', 'Ivanova', '4fd52ac1d12be4ba3df78917cfc7cff686c3dbbbccb868f2d872d29617b5b1a0'),
    (3, 'chef03', 'Petr', 'Sidorov', 'bd674ed147f3544dabbacbc60cd3e3f948750177c2cedff5e0d0238cc5db556e'),
    (4, 'chef04', 'Maria', 'Smirnova', '7d6f145872ac80e9e9fc3d9d8b8661bdb1f30f7c4e40538769fd690a3a4318e4'),
    (5, 'chef05', 'Olga', 'Kuznetsova', '09c8f1d51ca04f36152038af078bba3435273f4b368fddf4b9061fde582db361'),
    (6, 'chef06', 'Dmitry', 'Volkov', '16b12e64a90730e63796220df0e3937f9f82fbcc5b54988b829cbfb183fa60b9'),
    (7, 'chef07', 'Elena', 'Morozova', 'cc5ab2e7170b7715c2053966d000fbe6a3e4da45f4e53875fc8cbb4966ea74fe'),
    (8, 'chef08', 'Nikita', 'Fedorov', '0457fe5285b797f01d40342ed0cfa338518332327dd40b1af5cb3372889980a2'),
    (9, 'chef09', 'Sofia', 'Lebedeva', '0357e6cdd5a768eed19dc2b9f4f4b93c4c5dbf5223aed7354391a2b69e7755e1'),
    (10, 'chef10', 'Artem', 'Pavlov', '788d3288f45a1f0fe88d760a7883637a2838741b713a91f18fb9f6537d784ad7')
ON CONFLICT (id) DO NOTHING;

INSERT INTO recipes (id, author_id, title, description) VALUES
    (1, 1, 'Borscht', 'Traditional beet soup'),
    (2, 1, 'Pancakes', 'Thin pancakes with milk'),
    (3, 2, 'Caesar Salad', 'Salad with chicken and sauce'),
    (4, 3, 'Tom Yum', 'Spicy Thai soup'),
    (5, 4, 'Carbonara', 'Pasta with bacon and egg sauce'),
    (6, 5, 'Miso Soup', 'Japanese soup with tofu'),
    (7, 6, 'Greek Salad', 'Vegetable salad with feta'),
    (8, 7, 'Cheesecake', 'Creamy dessert'),
    (9, 8, 'Shakshuka', 'Eggs in tomato sauce'),
    (10, 9, 'Pumpkin Soup', 'Cream soup from pumpkin')
ON CONFLICT (id) DO NOTHING;

INSERT INTO ingredients (id, recipe_id, name, amount) VALUES
    (1, 1, 'Beetroot', '2 pcs'),
    (2, 1, 'Cabbage', '300 g'),
    (3, 2, 'Milk', '500 ml'),
    (4, 2, 'Flour', '250 g'),
    (5, 3, 'Chicken breast', '200 g'),
    (6, 4, 'Shrimp', '150 g'),
    (7, 5, 'Spaghetti', '250 g'),
    (8, 6, 'Miso paste', '2 tbsp'),
    (9, 7, 'Feta', '120 g'),
    (10, 8, 'Cream cheese', '400 g'),
    (11, 9, 'Eggs', '4 pcs'),
    (12, 10, 'Pumpkin', '500 g')
ON CONFLICT (id) DO NOTHING;

INSERT INTO favorite_recipes (user_id, recipe_id) VALUES
    (1, 3),
    (1, 5),
    (2, 1),
    (2, 8),
    (3, 2),
    (4, 6),
    (5, 7),
    (6, 4),
    (7, 9),
    (8, 10)
ON CONFLICT (user_id, recipe_id) DO NOTHING;

INSERT INTO auth_tokens (token, user_id, expires_at) VALUES
    ('seed-token-01', 1, NOW() + INTERVAL '30 days'),
    ('seed-token-02', 2, NOW() + INTERVAL '30 days'),
    ('seed-token-03', 3, NOW() + INTERVAL '30 days'),
    ('seed-token-04', 4, NOW() + INTERVAL '30 days'),
    ('seed-token-05', 5, NOW() + INTERVAL '30 days'),
    ('seed-token-06', 6, NOW() + INTERVAL '30 days'),
    ('seed-token-07', 7, NOW() + INTERVAL '30 days'),
    ('seed-token-08', 8, NOW() + INTERVAL '30 days'),
    ('seed-token-09', 9, NOW() + INTERVAL '30 days'),
    ('seed-token-10', 10, NOW() + INTERVAL '30 days')
ON CONFLICT (token) DO NOTHING;

SELECT setval('users_id_seq', GREATEST((SELECT COALESCE(MAX(id), 1) FROM users), 1));
SELECT setval('recipes_id_seq', GREATEST((SELECT COALESCE(MAX(id), 1) FROM recipes), 1));
SELECT setval('ingredients_id_seq', GREATEST((SELECT COALESCE(MAX(id), 1) FROM ingredients), 1));
