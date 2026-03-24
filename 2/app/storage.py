from __future__ import annotations

from dataclasses import dataclass, field
from datetime import datetime, UTC
from threading import Lock
from typing import Any
from uuid import uuid4


@dataclass(slots=True)
class UserRecord:
    id: str
    login: str
    first_name: str
    last_name: str
    password_hash: str
    created_at: datetime
    favorite_recipe_ids: set[str] = field(default_factory=set)


@dataclass(slots=True)
class IngredientRecord:
    id: str
    name: str
    quantity: float
    unit: str
    added_at: datetime


@dataclass(slots=True)
class RecipeRecord:
    id: str
    title: str
    description: str
    instructions: str
    author_id: str
    created_at: datetime
    ingredients: list[IngredientRecord] = field(default_factory=list)


class DataStore:
    def __init__(self) -> None:
        self._lock = Lock()
        self.users: dict[str, UserRecord] = {}
        self.users_by_login: dict[str, str] = {}
        self.recipes: dict[str, RecipeRecord] = {}

    @staticmethod
    def _now() -> datetime:
        return datetime.now(UTC)

    @staticmethod
    def _id() -> str:
        return uuid4().hex

    def clear(self) -> None:
        with self._lock:
            self.users.clear()
            self.users_by_login.clear()
            self.recipes.clear()

    def create_user(
        self,
        *,
        login: str,
        first_name: str,
        last_name: str,
        password_hash: str,
    ) -> UserRecord:
        with self._lock:
            if login.lower() in self.users_by_login:
                raise ValueError("login_already_exists")

            user = UserRecord(
                id = self._id(),
                login = login,
                first_name = first_name,
                last_name = last_name,
                password_hash = password_hash,
                created_at = self._now(),
            )
            self.users[user.id] = user
            self.users_by_login[login.lower()] = user.id
            return user

    def get_user_by_id(self, user_id: str) -> UserRecord | None:
        return self.users.get(user_id)

    def get_user_by_login(self, login: str) -> UserRecord | None:
        user_id = self.users_by_login.get(login.lower())
        if user_id is None:
            return None
        return self.users.get(user_id)

    def search_users_by_name_mask(self, mask: str) -> list[UserRecord]:
        normalized = mask.strip().lower()
        return [
            user
            for user in self.users.values()
            if normalized in f"{user.first_name} {user.last_name}".lower()
        ]

    def create_recipe(
        self,
        *,
        title: str,
        description: str,
        instructions: str,
        author_id: str,
    ) -> RecipeRecord:
        with self._lock:
            recipe = RecipeRecord(
                id = self._id(),
                title = title,
                description = description,
                instructions = instructions,
                author_id = author_id,
                created_at = self._now(),
            )
            self.recipes[recipe.id] = recipe
            return recipe

    def list_recipes(self) -> list[RecipeRecord]:
        return sorted(self.recipes.values(), key=lambda recipe: recipe.created_at, reverse=True)

    def get_recipe(self, recipe_id: str) -> RecipeRecord | None:
        return self.recipes.get(recipe_id)

    def search_recipes_by_title(self, title: str) -> list[RecipeRecord]:
        normalized = title.strip().lower()
        return [
            recipe
            for recipe in self.list_recipes()
            if normalized in recipe.title.lower()
        ]

    def add_ingredient(
        self,
        *,
        recipe_id: str,
        name: str,
        quantity: float,
        unit: str,
    ) -> IngredientRecord:
        with self._lock:
            recipe = self.recipes.get(recipe_id)
            if recipe is None:
                raise KeyError("recipe_not_found")

            ingredient = IngredientRecord(
                id = self._id(),
                name = name,
                quantity = quantity,
                unit = unit,
                added_at = self._now(),
            )
            recipe.ingredients.append(ingredient)
            return ingredient

    def list_recipe_ingredients(self, recipe_id: str) -> list[IngredientRecord]:
        recipe = self.recipes.get(recipe_id)
        if recipe is None:
            raise KeyError("recipe_not_found")
        return list(recipe.ingredients)

    def list_recipes_by_user(self, user_id: str) -> list[RecipeRecord]:
        return [
            recipe
            for recipe in self.list_recipes()
            if recipe.author_id == user_id
        ]

    def add_favorite(self, *, user_id: str, recipe_id: str) -> None:
        with self._lock:
            user = self.users.get(user_id)
            recipe = self.recipes.get(recipe_id)
            if user is None:
                raise KeyError("user_not_found")
            if recipe is None:
                raise KeyError("recipe_not_found")
            user.favorite_recipe_ids.add(recipe_id)

    def list_favorites(self, user_id: str) -> list[RecipeRecord]:
        user = self.users.get(user_id)
        if user is None:
            raise KeyError("user_not_found")
        return [
            self.recipes[recipe_id]
            for recipe_id in user.favorite_recipe_ids
            if recipe_id in self.recipes
        ]


db = DataStore()
