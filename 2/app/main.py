from __future__ import annotations

from contextlib import asynccontextmanager

from fastapi import Depends, FastAPI, HTTPException, Query, Response, status

from app.auth import AuthMiddleware, create_access_token, get_current_user, hash_password, verify_password
from app.schemas import (
    ErrorResponse,
    IngredientCreateRequest,
    IngredientResponse,
    MessageResponse,
    RecipeCreateRequest,
    RecipeResponse,
    TokenResponse,
    UserLoginRequest,
    UserRegisterRequest,
    UserResponse,
)
from app.storage import RecipeRecord, UserRecord, db


@asynccontextmanager
async def lifespan(_: FastAPI):
    yield


app = FastAPI(
    title = "Recipe Management API",
    version = "1.0.0",
    summary = "REST API for homework variant 23",
    description = (
        "FastAPI service for managing users, recipes and ingredients. "
        "Implements JWT authentication, in-memory storage and Swagger UI."
    ),
    lifespan=lifespan,
)
app.add_middleware(AuthMiddleware)


def to_recipe_response(recipe: RecipeRecord) -> RecipeResponse:
    return RecipeResponse.model_validate(recipe)


def to_user_response(user: UserRecord) -> UserResponse:
    return UserResponse.model_validate(user)


@app.get("/health", tags = ["system"])
def health() -> dict[str, str]:
    return {"status": "ok"}


@app.get("/", tags = ["Root"])
def root():
    return {
        "message": "Recipe API is running",
        "docs": "/docs",
        "health": "/health"
    }


@app.post(
    "/auth/register",
    response_model = UserResponse,
    status_code = status.HTTP_201_CREATED,
    tags = ["auth"],
    responses = {409: {"model": ErrorResponse}},
)
def register_user(payload: UserRegisterRequest) -> UserResponse:
    try:
        user = db.create_user(
            login = payload.login,
            first_name = payload.first_name,
            last_name = payload.last_name,
            password_hash=hash_password(payload.password),
        )
    except ValueError as exc:
        raise HTTPException(status_code = status.HTTP_409_CONFLICT, detail="Login already exists") from exc

    return to_user_response(user)


@app.post(
    "/auth/login",
    response_model=TokenResponse,
    tags = ["auth"],
    responses={401: {"model": ErrorResponse}},
)
def login_user(payload: UserLoginRequest) -> TokenResponse:
    user = db.get_user_by_login(payload.login)
    if user is None or not verify_password(payload.password, user.password_hash):
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Invalid login or password")

    token = create_access_token(user_id=user.id, login=user.login)
    return TokenResponse(access_token=token)


@app.get(
    "/users/by-login/{login}",
    response_model = UserResponse,
    tags = ["users"],
    responses = {404: {"model": ErrorResponse}},
)
def get_user_by_login(login: str) -> UserResponse:
    user = db.get_user_by_login(login)
    if user is None:
        raise HTTPException(status_code = status.HTTP_404_NOT_FOUND, detail = "User not found")
    return to_user_response(user)


@app.get("/users/search", response_model = list[UserResponse], tags = ["users"])
def search_users_by_name(mask: str = Query(..., min_length=1, description = "First name / last name mask")) -> list[UserResponse]:
    return [to_user_response(user) for user in db.search_users_by_name_mask(mask)]


@app.post(
    "/recipes",
    response_model = RecipeResponse,
    status_code = status.HTTP_201_CREATED,
    tags = ["recipes"],
    responses = {401: {"model": ErrorResponse}},
)
def create_recipe(payload: RecipeCreateRequest, current_user: UserRecord = Depends(get_current_user)) -> RecipeResponse:
    recipe = db.create_recipe(
        title = payload.title,
        description = payload.description,
        instructions = payload.instructions,
        author_id = current_user.id,
    )
    return to_recipe_response(recipe)


@app.get("/recipes", response_model = list[RecipeResponse], tags = ["recipes"])
def list_recipes() -> list[RecipeResponse]:
    return [to_recipe_response(recipe) for recipe in db.list_recipes()]


@app.get("/recipes/search", response_model = list[RecipeResponse], tags = ["recipes"])
def search_recipes_by_title(title: str = Query(..., min_length=1)) -> list[RecipeResponse]:
    return [to_recipe_response(recipe) for recipe in db.search_recipes_by_title(title)]


@app.post(
    "/recipes/{recipe_id}/ingredients",
    response_model = IngredientResponse,
    status_code = status.HTTP_201_CREATED,
    tags = ["ingredients"],
    responses={401: {"model": ErrorResponse}, 403: {"model": ErrorResponse}, 404: {"model": ErrorResponse}},
)
def add_ingredient_to_recipe(
    recipe_id: str,
    payload: IngredientCreateRequest,
    current_user: UserRecord = Depends(get_current_user),
) -> IngredientResponse:
    recipe = db.get_recipe(recipe_id)
    if recipe is None:
        raise HTTPException(status_code = status.HTTP_404_NOT_FOUND, detail = "Recipe not found")
    if recipe.author_id != current_user.id:
        raise HTTPException(status_code = status.HTTP_403_FORBIDDEN, detail = "Only recipe author can add ingredients")

    ingredient = db.add_ingredient(
        recipe_id = recipe_id,
        name = payload.name,
        quantity = payload.quantity,
        unit = payload.unit,
    )
    return IngredientResponse.model_validate(ingredient)


@app.get(
    "/recipes/{recipe_id}/ingredients",
    response_model = list[IngredientResponse],
    tags = ["ingredients"],
    responses = {404: {"model": ErrorResponse}},
)
def list_recipe_ingredients(recipe_id: str) -> list[IngredientResponse]:
    recipe = db.get_recipe(recipe_id)
    if recipe is None:
        raise HTTPException(status_code = status.HTTP_404_NOT_FOUND, detail = "Recipe not found")
    return [IngredientResponse.model_validate(item) for item in db.list_recipe_ingredients(recipe_id)]


@app.get(
    "/users/me/recipes",
    response_model = list[RecipeResponse],
    tags = ["users", "recipes"],
    responses = {401: {"model": ErrorResponse}},
)
def list_my_recipes(current_user: UserRecord = Depends(get_current_user)) -> list[RecipeResponse]:
    return [to_recipe_response(recipe) for recipe in db.list_recipes_by_user(current_user.id)]


@app.get(
    "/users/{user_id}/recipes",
    response_model = list[RecipeResponse],
    tags = ["users", "recipes"],
    responses = {404: {"model": ErrorResponse}},
)
def list_user_recipes(user_id: str) -> list[RecipeResponse]:
    user = db.get_user_by_id(user_id)
    if user is None:
        raise HTTPException(status_code = status.HTTP_404_NOT_FOUND, detail = "User not found")
    return [to_recipe_response(recipe) for recipe in db.list_recipes_by_user(user_id)]


@app.post(
    "/recipes/{recipe_id}/favorite",
    response_model = MessageResponse,
    tags = ["favorites"],
    responses = {401: {"model": ErrorResponse}, 404: {"model": ErrorResponse}},
)
def add_recipe_to_favorites(recipe_id: str, current_user: UserRecord = Depends(get_current_user)) -> MessageResponse:
    recipe = db.get_recipe(recipe_id)
    if recipe is None:
        raise HTTPException(status_code = status.HTTP_404_NOT_FOUND, detail="Recipe not found")
    db.add_favorite(user_id = current_user.id, recipe_id = recipe_id)
    return MessageResponse(detail = "Recipe added to favorites")


@app.get(
    "/users/me/favorites",
    response_model = list[RecipeResponse],
    tags = ["favorites"],
    responses = {401: {"model": ErrorResponse}},
)
def list_my_favorites(current_user: UserRecord = Depends(get_current_user)) -> list[RecipeResponse]:
    return [to_recipe_response(recipe) for recipe in db.list_favorites(current_user.id)]


@app.delete(
    "/recipes/{recipe_id}/favorite",
    status_code = status.HTTP_204_NO_CONTENT,
    tags = ["favorites"],
    responses = {401: {"model": ErrorResponse}, 404: {"model": ErrorResponse}},
)
def remove_recipe_from_favorites(recipe_id: str, current_user: UserRecord = Depends(get_current_user)) -> Response:
    recipe = db.get_recipe(recipe_id)
    if recipe is None:
        raise HTTPException(status_code = status.HTTP_404_NOT_FOUND, detail = "Recipe not found")

    current_user.favorite_recipe_ids.discard(recipe_id)
    return Response(status_code=status.HTTP_204_NO_CONTENT)
