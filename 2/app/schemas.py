from __future__ import annotations

from datetime import datetime

from pydantic import BaseModel, ConfigDict, Field


class MessageResponse(BaseModel):
    detail: str = Field(examples = ["Recipe added to favorites"])


class ErrorResponse(BaseModel):
    detail: str = Field(examples = ["Recipe not found"])


class UserRegisterRequest(BaseModel):
    login: str = Field(min_length = 3, max_length = 50, examples = ["chef_anna"])
    first_name: str = Field(min_length = 1, max_length = 50, examples = ["Anna"])
    last_name: str = Field(min_length = 1, max_length = 50, examples = ["Ivanova"])
    password: str = Field(min_length = 6, max_length = 128, examples = ["StrongPass123"])


class UserLoginRequest(BaseModel):
    login: str = Field(examples = ["chef_anna"])
    password: str = Field(examples = ["StrongPass123"])


class UserResponse(BaseModel): 
    model_config = ConfigDict(from_attributes=True)

    id: str
    login: str
    first_name: str
    last_name: str
    created_at: datetime


class TokenResponse(BaseModel):
    access_token: str
    token_type: str = Field(default="bearer")


class RecipeCreateRequest(BaseModel):
    title: str = Field(min_length = 2, max_length = 120, examples = ["Spaghetti Carbonara"])
    description: str = Field(min_length = 5, max_length = 300, examples = ["Classic Italian pasta recipe"])
    instructions: str = Field(min_length = 10, max_length = 2000, examples = ["Boil pasta, fry pancetta, mix with eggs and cheese."])


class RecipeResponse(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: str
    title: str
    description: str
    instructions: str
    author_id: str
    created_at: datetime


class IngredientCreateRequest(BaseModel):
    name: str = Field(min_length = 1, max_length = 100, examples = ["Parmesan"])
    quantity: float = Field(gt = 0, examples = [100])
    unit: str = Field(min_length = 1, max_length = 20, examples = ["g"])


class IngredientResponse(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: str
    name: str
    quantity: float
    unit: str
    added_at: datetime


class RecipeWithIngredientsResponse(RecipeResponse):
    ingredients: list[IngredientResponse] = Field(default_factory = list)

