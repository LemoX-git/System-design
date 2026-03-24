from __future__ import annotations

import pytest
from fastapi.testclient import TestClient

from app.main import app
from app.storage import db


@pytest.fixture(autouse=True)
def clear_db() -> None:
    db.clear()


@pytest.fixture()
def client() -> TestClient:
    return TestClient(app)


def register_user(client: TestClient, login: str = "chef_anna") -> dict:
    response = client.post(
        "/auth/register",
        json = {
            "login": login,
            "first_name": "Anna",
            "last_name": "Ivanova",
            "password": "StrongPass123",
        },
    )
    assert response.status_code == 201
    return response.json()


def login_user(client: TestClient, login: str = "chef_anna") -> str:
    response = client.post(
        "/auth/login",
        json = {"login": login, "password": "StrongPass123"},
    )
    assert response.status_code == 200
    return response.json()["access_token"]


def auth_headers(token: str) -> dict[str, str]:
    return {"Authorization": f"Bearer {token}"}


def test_register_and_find_user_by_login(client: TestClient) -> None:
    created_user = register_user(client)

    response = client.get(f"/users/by-login/{created_user['login']}")

    assert response.status_code == 200
    assert response.json()["login"] == "chef_anna"


def test_duplicate_login_returns_409(client: TestClient) -> None:
    register_user(client)

    response = client.post(
        "/auth/register",
        json = {
            "login": "chef_anna",
            "first_name": "Other",
            "last_name": "User",
            "password": "StrongPass123",
        },
    )

    assert response.status_code == 409
    assert response.json()["detail"] == "Login already exists"


def test_create_recipe_requires_authentication(client: TestClient) -> None:
    response = client.post(
        "/recipes",
        json = {
            "title": "Carbonara",
            "description": "Italian pasta",
            "instructions": "Cook pasta and mix all ingredients.",
        },
    )

    assert response.status_code == 401
    assert response.json()["detail"] == "Authentication required"


def test_create_recipe_add_ingredient_and_get_favorites(client: TestClient) -> None:
    user = register_user(client)
    token = login_user(client)

    create_recipe_response = client.post(
        "/recipes",
        headers = auth_headers(token),
        json = {
            "title": "Carbonara",
            "description": "Italian pasta",
            "instructions": "Cook pasta and mix eggs with cheese.",
        },
    )
    assert create_recipe_response.status_code == 201
    recipe_id = create_recipe_response.json()["id"]

    add_ingredient_response = client.post(
        f"/recipes/{recipe_id}/ingredients",
        headers = auth_headers(token),
        json = {"name": "Parmesan", "quantity": 100, "unit": "g"},
    )
    assert add_ingredient_response.status_code == 201
    assert add_ingredient_response.json()["name"] == "Parmesan"

    ingredient_list_response = client.get(f"/recipes/{recipe_id}/ingredients")
    assert ingredient_list_response.status_code == 200
    assert len(ingredient_list_response.json()) == 1

    favorite_response = client.post(f"/recipes/{recipe_id}/favorite", headers=auth_headers(token))
    assert favorite_response.status_code == 200

    favorites_response = client.get("/users/me/favorites", headers=auth_headers(token))
    assert favorites_response.status_code == 200
    assert favorites_response.json()[0]["id"] == recipe_id

    my_recipes_response = client.get("/users/me/recipes", headers=auth_headers(token))
    assert my_recipes_response.status_code == 200
    assert my_recipes_response.json()[0]["author_id"] == user["id"]


def test_search_recipes_by_title(client: TestClient) -> None:
    register_user(client)
    token = login_user(client)

    client.post(
        "/recipes",
        headers = auth_headers(token),
        json = {
            "title": "Tomato Soup",
            "description": "Warm soup",
            "instructions": "Boil tomatoes and blend.",
        },
    )
    client.post(
        "/recipes",
        headers = auth_headers(token),
        json = {
            "title": "Pumpkin Soup",
            "description": "Autumn soup",
            "instructions": "Bake pumpkin and blend.",
        },
    )

    response = client.get("/recipes/search", params={"title": "Soup"})

    assert response.status_code == 200
    assert len(response.json()) == 2


def test_add_ingredient_to_missing_recipe_returns_404(client: TestClient) -> None:
    register_user(client)
    token = login_user(client)

    response = client.post(
        "/recipes/unknown/ingredients",
        headers = auth_headers(token),
        json = {"name": "Salt", "quantity": 1, "unit": "tsp"},
    )

    assert response.status_code == 404
    assert response.json()["detail"] == "Recipe not found"
