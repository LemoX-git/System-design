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

if (!db.getCollectionNames().includes('recipes')) {
  db.createCollection('recipes', { validator: recipeValidator, validationLevel: 'strict' });
} else {
  db.runCommand({
    collMod: 'recipes',
    validator: recipeValidator,
    validationLevel: 'strict'
  });
}

print('Validation configured for recipes collection');
print('Trying to insert invalid recipe...');

try {
  db.recipes.insertOne({
    id: 9999,
    author_id: 1,
    title: 'x',
    description: '',
    ingredients: 'not-an-array',
    created_at: new Date(),
    updated_at: new Date()
  });
  print('Unexpected: invalid document was inserted');
} catch (e) {
  print('Validation works, invalid document rejected');
  print(e.message);
}
