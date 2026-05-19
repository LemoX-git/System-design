CREATE EXTENSION IF NOT EXISTS pg_trgm;

CREATE TABLE IF NOT EXISTS users (
    id BIGSERIAL PRIMARY KEY,
    login VARCHAR(64) NOT NULL UNIQUE,
    first_name VARCHAR(100) NOT NULL,
    last_name VARCHAR(100) NOT NULL,
    password_hash TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CHECK (char_length(login) > 0),
    CHECK (char_length(first_name) > 0),
    CHECK (char_length(last_name) > 0)
);

CREATE TABLE IF NOT EXISTS recipes (
    id BIGSERIAL PRIMARY KEY,
    author_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    title VARCHAR(200) NOT NULL,
    description TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CHECK (char_length(title) > 0),
    CHECK (char_length(description) > 0)
);

CREATE TABLE IF NOT EXISTS ingredients (
    id BIGSERIAL PRIMARY KEY,
    recipe_id BIGINT NOT NULL REFERENCES recipes(id) ON DELETE CASCADE,
    name VARCHAR(150) NOT NULL,
    amount VARCHAR(100) NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CHECK (char_length(name) > 0),
    CHECK (char_length(amount) > 0)
);

CREATE TABLE IF NOT EXISTS favorite_recipes (
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    recipe_id BIGINT NOT NULL REFERENCES recipes(id) ON DELETE CASCADE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    PRIMARY KEY (user_id, recipe_id)
);

CREATE TABLE IF NOT EXISTS auth_tokens (
    token TEXT PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    expires_at TIMESTAMPTZ
);

CREATE INDEX IF NOT EXISTS idx_recipes_author_id ON recipes(author_id);
CREATE INDEX IF NOT EXISTS idx_ingredients_recipe_id ON ingredients(recipe_id);
CREATE INDEX IF NOT EXISTS idx_favorite_recipes_recipe_id ON favorite_recipes(recipe_id);
CREATE INDEX IF NOT EXISTS idx_auth_tokens_user_id ON auth_tokens(user_id);
CREATE INDEX IF NOT EXISTS idx_auth_tokens_expires_at ON auth_tokens(expires_at);
CREATE INDEX IF NOT EXISTS idx_recipes_created_at ON recipes(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_users_full_name_trgm
    ON users USING GIN (lower(first_name || ' ' || last_name) gin_trgm_ops);
CREATE INDEX IF NOT EXISTS idx_recipes_title_trgm
    ON recipes USING GIN (lower(title) gin_trgm_ops);

-- Event-driven integration layer. The HTTP service writes integration events
-- into the outbox in the same PostgreSQL database. A separate producer service
-- reads this table and publishes messages to RabbitMQ.
CREATE TABLE IF NOT EXISTS event_outbox (
    id BIGSERIAL PRIMARY KEY,
    event_type VARCHAR(100) NOT NULL,
    event_version INT NOT NULL DEFAULT 1,
    routing_key VARCHAR(100) NOT NULL,
    aggregate_type VARCHAR(100) NOT NULL,
    aggregate_id BIGINT NOT NULL,
    payload JSONB NOT NULL,
    status VARCHAR(20) NOT NULL DEFAULT 'NEW',
    attempts INT NOT NULL DEFAULT 0,
    last_error TEXT,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    published_at TIMESTAMPTZ,
    CHECK (status IN ('NEW', 'PUBLISHED', 'FAILED'))
);

CREATE INDEX IF NOT EXISTS idx_event_outbox_status_created_at
    ON event_outbox(status, created_at);
CREATE INDEX IF NOT EXISTS idx_event_outbox_aggregate
    ON event_outbox(aggregate_type, aggregate_id);

-- Read side for the CQRS part of the assignment. It is filled asynchronously
-- by the RabbitMQ consumer and can be used by reporting/search read services.
CREATE TABLE IF NOT EXISTS recipe_read_model (
    recipe_id BIGINT PRIMARY KEY,
    author_id BIGINT NOT NULL,
    title VARCHAR(200) NOT NULL,
    description TEXT NOT NULL,
    ingredients_count INT NOT NULL DEFAULT 0,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS recipe_ingredient_read_model (
    ingredient_id BIGINT PRIMARY KEY,
    recipe_id BIGINT NOT NULL,
    author_id BIGINT NOT NULL,
    name VARCHAR(150) NOT NULL,
    amount VARCHAR(100) NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_recipe_ingredient_read_model_recipe_id
    ON recipe_ingredient_read_model(recipe_id);

CREATE TABLE IF NOT EXISTS user_favorite_recipe_read_model (
    user_id BIGINT NOT NULL,
    recipe_id BIGINT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    PRIMARY KEY (user_id, recipe_id)
);

CREATE INDEX IF NOT EXISTS idx_user_favorite_recipe_read_model_recipe_id
    ON user_favorite_recipe_read_model(recipe_id);

CREATE TABLE IF NOT EXISTS event_log (
    event_id BIGINT PRIMARY KEY,
    event_type VARCHAR(100) NOT NULL,
    routing_key VARCHAR(100) NOT NULL,
    payload JSONB NOT NULL,
    received_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
