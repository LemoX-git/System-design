use('recipe_service');

db.users.deleteMany({});
db.recipes.deleteMany({});
db.counters.deleteMany({});

const baseDate = new Date('2026-04-01T10:00:00Z');

const users = [
  { id: 1, login: 'chef_anna', first_name: 'Anna', last_name: 'Ivanova' },
  { id: 2, login: 'chef_petr', first_name: 'Petr', last_name: 'Sidorov' },
  { id: 3, login: 'chef_maria', first_name: 'Maria', last_name: 'Petrova' },
  { id: 4, login: 'chef_ivan', first_name: 'Ivan', last_name: 'Smirnov' },
  { id: 5, login: 'chef_olga', first_name: 'Olga', last_name: 'Kuznetsova' },
  { id: 6, login: 'chef_nikita', first_name: 'Nikita', last_name: 'Popov' },
  { id: 7, login: 'chef_elena', first_name: 'Elena', last_name: 'Morozova' },
  { id: 8, login: 'chef_dmitry', first_name: 'Dmitry', last_name: 'Volkov' },
  { id: 9, login: 'chef_sofia', first_name: 'Sofia', last_name: 'Fedorova' },
  { id: 10, login: 'chef_alexey', first_name: 'Alexey', last_name: 'Lebedev' }
].map((u, idx) => ({
  ...u,
  full_name_lc: `${u.first_name} ${u.last_name}`.toLowerCase(),
  password_hash: 'pw$test123',
  favorite_recipe_ids: idx < 3 ? [idx + 1, idx + 2] : [((idx + 1) % 10) + 1],
  auth_tokens: [{ token: `seed-token-${u.id}`, issued_at: new Date(baseDate.getTime() + idx * 60000) }],
  created_at: new Date(baseDate.getTime() + idx * 60000),
  updated_at: new Date(baseDate.getTime() + idx * 60000)
}));

const recipeTitles = [
  'Tomato Soup',
  'Chicken Curry',
  'Greek Salad',
  'Banana Pancakes',
  'Mushroom Pasta',
  'Fish Tacos',
  'Pumpkin Cream Soup',
  'Beef Stew',
  'Berry Smoothie Bowl',
  'Cheese Omelette'
];

const recipes = recipeTitles.map((title, idx) => ({
  id: idx + 1,
  author_id: (idx % 10) + 1,
  title,
  title_lc: title.toLowerCase(),
  description: `Sample description for ${title}`,
  ingredients: [
    { id: idx * 2 + 1, name: 'Salt', amount: '1 tsp', created_at: new Date(baseDate.getTime() + idx * 120000) },
    { id: idx * 2 + 2, name: 'Olive oil', amount: '2 tbsp', created_at: new Date(baseDate.getTime() + idx * 120000 + 30000) }
  ],
  created_at: new Date(baseDate.getTime() + idx * 120000),
  updated_at: new Date(baseDate.getTime() + idx * 120000 + 30000)
}));

const counters = [
  { _id: 'users', seq: 10 },
  { _id: 'recipes', seq: 10 },
  { _id: 'ingredients', seq: 20 },
  { _id: 'tokens', seq: 10 }
];

db.users.insertMany(users);
db.recipes.insertMany(recipes);
db.counters.insertMany(counters);
