SELECT 'CREATE DATABASE recipe_service_test OWNER recipe_user'
WHERE NOT EXISTS (
    SELECT 1 FROM pg_database WHERE datname = 'recipe_service_test'
)\gexec
