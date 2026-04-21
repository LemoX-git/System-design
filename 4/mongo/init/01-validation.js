use('recipe_service');

const recipeValidator = {
  $jsonSchema: {
    bsonType: 'object',
    required: ['id', 'author_id', 'title', 'title_lc', 'description', 'ingredients', 'created_at', 'updated_at'],
    additionalProperties: true,
    properties: {
      _id: {},
      id: { bsonType: 'int', minimum: 1 },
      author_id: { bsonType: 'int', minimum: 1 },
      title: { bsonType: 'string', minLength: 2, maxLength: 200 },
      title_lc: { bsonType: 'string', minLength: 2, maxLength: 200 },
      description: { bsonType: 'string', minLength: 3, maxLength: 5000 },
      created_at: { bsonType: 'date' },
      updated_at: { bsonType: 'date' },
      ingredients: {
        bsonType: 'array',
        items: {
          bsonType: 'object',
          required: ['id', 'name', 'amount'],
          properties: {
            id: { bsonType: 'int', minimum: 1 },
            name: { bsonType: 'string', minLength: 1, maxLength: 200 },
            amount: { bsonType: 'string', minLength: 1, maxLength: 100 },
            created_at: { bsonType: ['date', 'null'] }
          }
        }
      }
    }
  }
};

if (!db.getCollectionNames().includes('users')) db.createCollection('users');
if (!db.getCollectionNames().includes('counters')) db.createCollection('counters');
if (!db.getCollectionNames().includes('recipes')) {
  db.createCollection('recipes', { validator: recipeValidator, validationLevel: 'strict' });
} else {
  db.runCommand({ collMod: 'recipes', validator: recipeValidator, validationLevel: 'strict' });
}

db.users.createIndex({ id: 1 }, { unique: true });
db.users.createIndex({ login: 1 }, { unique: true });
db.users.createIndex({ full_name_lc: 1 });
db.users.createIndex({ 'auth_tokens.token': 1 });

db.recipes.createIndex({ id: 1 }, { unique: true });
db.recipes.createIndex({ author_id: 1 });
db.recipes.createIndex({ title_lc: 1 });
