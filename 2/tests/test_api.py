import uuid


def unique_value(prefix='x'):
    return f"{prefix}-{uuid.uuid4().hex[:8]}"


async def register_user(
    service_client,
    login='chef1',
    first_name='Ivan',
    last_name='Petrov',
    password='secret',
):
    response = await service_client.post(
        '/api/v1/auth/register',
        json={
            'login': login,
            'first_name': first_name,
            'last_name': last_name,
            'password': password,
        },
    )
    return response


async def login_user(service_client, login='chef1', password='secret'):
    response = await service_client.post(
        '/api/v1/auth/login',
        json={'login': login, 'password': password},
    )
    return response


async def create_recipe(service_client, token, title='Borscht', description='Classic soup'):
    return await service_client.post(
        '/api/v1/recipes',
        headers={'Authorization': f'Bearer {token}'},
        json={'title': title, 'description': description},
    )


async def test_register_and_find_user(service_client):
    login = unique_value('chef')
    response = await register_user(service_client, login=login)
    assert response.status == 201

    response = await service_client.get('/api/v1/users/by-login', params={'login': login})
    assert response.status == 200
    assert response.json()['login'] == login


async def test_register_duplicate_user_returns_conflict(service_client):
    login = unique_value('chef')
    first = await register_user(service_client, login=login)
    second = await register_user(service_client, login=login)

    assert first.status == 201
    assert second.status == 409


async def test_login_wrong_password_returns_unauthorized(service_client):
    login = unique_value('chef')
    response = await register_user(service_client, login=login)
    assert response.status == 201

    response = await login_user(service_client, login=login, password='wrong-password')
    assert response.status == 401


async def test_user_search_by_mask(service_client):
    login = unique_value('chef')
    response = await register_user(service_client, login=login, first_name='Anna', last_name='Ivanova')
    assert response.status == 201

    response = await service_client.get('/api/v1/users/search', params={'mask': 'anna iva'})
    assert response.status == 200
    assert len(response.json()) >= 1
    assert any(user['login'] == login for user in response.json())


async def test_create_recipe_requires_auth(service_client):
    response = await service_client.post(
        '/api/v1/recipes',
        json={'title': 'Pasta', 'description': 'Quick pasta'},
    )
    assert response.status == 401


async def test_create_recipe_and_search(service_client):
    login = unique_value('chef')
    title = unique_value('borscht')
    response = await register_user(service_client, login=login)
    assert response.status == 201

    response = await login_user(service_client, login=login)
    assert response.status == 200
    token = response.json()['token']

    create_response = await create_recipe(service_client, token, title=title)
    assert create_response.status == 201
    recipe_id = create_response.json()['id']

    list_response = await service_client.get('/api/v1/recipes', params={'title': title[:6]})
    assert list_response.status == 200
    assert any(recipe['id'] == recipe_id for recipe in list_response.json())


async def test_add_ingredient_and_read_back(service_client):
    login = unique_value('chef')
    response = await register_user(service_client, login=login)
    assert response.status == 201

    response = await login_user(service_client, login=login)
    assert response.status == 200
    token = response.json()['token']

    recipe_response = await create_recipe(service_client, token, title=unique_value('salad'), description='Fresh salad')
    assert recipe_response.status == 201
    recipe_id = recipe_response.json()['id']

    ingredient_response = await service_client.post(
        f'/api/v1/recipes/{recipe_id}/ingredients',
        headers={'Authorization': f'Bearer {token}'},
        json={'name': 'Tomato', 'amount': '2 pcs'},
    )
    assert ingredient_response.status == 201

    list_response = await service_client.get(f'/api/v1/recipes/{recipe_id}/ingredients')
    assert list_response.status == 200
    assert list_response.json()[0]['name'] == 'Tomato'


async def test_add_ingredient_forbidden_for_non_author(service_client):
    author_login = unique_value('author')
    response = await register_user(service_client, login=author_login)
    assert response.status == 201
    response = await login_user(service_client, login=author_login)
    assert response.status == 200
    author_token = response.json()['token']

    other_login = unique_value('other')
    response = await register_user(service_client, login=other_login, first_name='Petr', last_name='Sidorov')
    assert response.status == 201
    response = await login_user(service_client, login=other_login)
    assert response.status == 200
    other_token = response.json()['token']

    recipe_response = await create_recipe(service_client, author_token, title=unique_value('soup'), description='Hot soup')
    assert recipe_response.status == 201
    recipe_id = recipe_response.json()['id']

    response = await service_client.post(
        f'/api/v1/recipes/{recipe_id}/ingredients',
        headers={'Authorization': f'Bearer {other_token}'},
        json={'name': 'Salt', 'amount': '1 tsp'},
    )
    assert response.status == 403


async def test_invalid_recipe_id_returns_bad_request(service_client):
    response = await service_client.get('/api/v1/recipes/not-an-int/ingredients')
    assert response.status == 400


async def test_favorites_require_auth(service_client):
    response = await service_client.get('/api/v1/users/me/favorites')
    assert response.status == 401


async def test_add_to_favorites_and_duplicate_conflict(service_client):
    login = unique_value('chef')
    response = await register_user(service_client, login=login)
    assert response.status == 201
    response = await login_user(service_client, login=login)
    assert response.status == 200
    token = response.json()['token']

    recipe_response = await create_recipe(service_client, token, title=unique_value('pasta'), description='Quick pasta')
    assert recipe_response.status == 201
    recipe_id = recipe_response.json()['id']

    first = await service_client.post(
        f'/api/v1/users/me/favorites/{recipe_id}',
        headers={'Authorization': f'Bearer {token}'},
    )
    second = await service_client.post(
        f'/api/v1/users/me/favorites/{recipe_id}',
        headers={'Authorization': f'Bearer {token}'},
    )

    assert first.status == 201
    assert second.status == 409

    list_response = await service_client.get(
        '/api/v1/users/me/favorites',
        headers={'Authorization': f'Bearer {token}'},
    )
    assert list_response.status == 200
    assert len(list_response.json()) == 1
    assert list_response.json()[0]['id'] == recipe_id


async def test_add_missing_recipe_to_favorites_returns_not_found(service_client):
    login = unique_value('chef')
    response = await register_user(service_client, login=login)
    assert response.status == 201
    response = await login_user(service_client, login=login)
    assert response.status == 200
    token = response.json()['token']

    response = await service_client.post(
        '/api/v1/users/me/favorites/999',
        headers={'Authorization': f'Bearer {token}'},
    )
    assert response.status == 404
