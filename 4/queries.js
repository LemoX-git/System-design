use('recipe_service');

// ---------------------------
// CREATE
// ---------------------------
db.users.insertOne({
  id: 11,
  login: 'chef_new',
  first_name: 'New',
  last_name: 'Cook',
  full_name_lc: 'new cook',
  password_hash: 'pw$test123',
  favorite_recipe_ids: [],
  auth_tokens: [],
  created_at: new Date(),
  updated_at: new Date()
});

db.recipes.insertOne({
  id: 11,
  author_id: 11,
  title: 'Veggie Bowl',
  title_lc: 'veggie bowl',
  description: 'Healthy lunch bowl',
  ingredients: [],
  created_at: new Date(),
  updated_at: new Date()
});

// Добавить ингредиент через $push
db.recipes.updateOne(
  { id: 11 },
  {
    $push: {
      ingredients: {
        id: 21,
        name: 'Avocado',
        amount: '1 pc',
        created_at: new Date()
      }
    },
    $set: { updated_at: new Date() }
  }
);

// Добавить рецепт в избранное через $addToSet
db.users.updateOne(
  { id: 11 },
  {
    $addToSet: { favorite_recipe_ids: 1 },
    $set: { updated_at: new Date() }
  }
);

// ---------------------------
// READ
// ---------------------------

// Поиск пользователя по логину ($eq)
db.users.find({ login: { $eq: 'chef_anna' } });

// Поиск пользователей по маске имени и фамилии
// Для substring-поиска используется lowercase поле + regex
db.users.find({ full_name_lc: /anna iva/i });

// Получение списка всех рецептов
db.recipes.find({}, { title: 1, author_id: 1, description: 1 });

// Поиск рецептов по названию
db.recipes.find({ title_lc: /soup/i });

// Получение ингредиентов рецепта
db.recipes.findOne({ id: 1 }, { ingredients: 1, _id: 0 });

// Получение рецептов пользователя
db.recipes.find({ author_id: 1 });

// Получение избранного пользователя через $in
const user = db.users.findOne({ id: 1 });
db.recipes.find({ id: { $in: user.favorite_recipe_ids } });

// $and, $or, $ne, $gt, $lt
// Рецепты не пользователя 1, у которых id > 3 и id < 9
db.recipes.find({
  $and: [
    { author_id: { $ne: 1 } },
    { id: { $gt: 3 } },
    { id: { $lt: 9 } }
  ]
});

// Рецепты с soup или pasta в названии
db.recipes.find({
  $or: [
    { title_lc: /soup/i },
    { title_lc: /pasta/i }
  ]
});

// ---------------------------
// UPDATE
// ---------------------------

// Изменить описание рецепта
db.recipes.updateOne(
  { id: 2 },
  { $set: { description: 'Updated chicken curry description', updated_at: new Date() } }
);

// Удалить ингредиент через $pull
db.recipes.updateOne(
  { id: 2 },
  { $pull: { ingredients: { name: 'Salt' } }, $set: { updated_at: new Date() } }
);

// Удалить рецепт из избранного через $pull
db.users.updateOne(
  { id: 1 },
  { $pull: { favorite_recipe_ids: 2 }, $set: { updated_at: new Date() } }
);

// ---------------------------
// DELETE
// ---------------------------

// Удаление рецепта
db.recipes.deleteOne({ id: 11 });

// Удаление пользователя
db.users.deleteOne({ id: 11 });

// ---------------------------
// AGGREGATION (optional part)
// ---------------------------

// Сколько рецептов у каждого автора
// Используются $match, $group, $project, $sort

db.recipes.aggregate([
  { $match: { id: { $gt: 0 } } },
  {
    $group: {
      _id: '$author_id',
      recipes_count: { $sum: 1 },
      avg_ingredients: { $avg: { $size: '$ingredients' } }
    }
  },
  {
    $project: {
      _id: 0,
      author_id: '$_id',
      recipes_count: 1,
      avg_ingredients: { $round: ['$avg_ingredients', 2] }
    }
  },
  { $sort: { recipes_count: -1, author_id: 1 } }
]);
