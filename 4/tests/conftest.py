import pytest

pytest_plugins = ['pytest_userver.plugins.mongo']

MONGO_COLLECTIONS = {
    'users': {
        'settings': {
            'collection': 'users',
            'connection': 'recipe_service',
            'database': 'recipe_service',
        },
        'indexes': [],
    },
    'recipes': {
        'settings': {
            'collection': 'recipes',
            'connection': 'recipe_service',
            'database': 'recipe_service',
        },
        'indexes': [],
    },
    'counters': {
        'settings': {
            'collection': 'counters',
            'connection': 'recipe_service',
            'database': 'recipe_service',
        },
        'indexes': [],
    },
}


@pytest.fixture(scope='session')
def mongodb_settings():
    return MONGO_COLLECTIONS


@pytest.fixture(autouse=True)
def clean_mongo(mongodb):
    mongodb.users.delete_many({})
    mongodb.recipes.delete_many({})
    mongodb.counters.delete_many({})
