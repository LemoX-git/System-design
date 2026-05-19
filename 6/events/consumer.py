import json

from psycopg2.extras import Json

from common import QUEUE_NAME, declare_read_model_queue, open_db_connection, open_rabbitmq_connection


def apply_user_registered(cursor, payload):
    # В этой работе событие логируется, но отдельная read-модель пользователей не нужна:
    # поисковые пользовательские запросы уже покрыты основной таблицей users.
    _ = payload


def apply_recipe_created(cursor, payload):
    cursor.execute(
        """
        INSERT INTO recipe_read_model (
            recipe_id,
            author_id,
            title,
            description,
            ingredients_count,
            created_at,
            updated_at
        )
        VALUES (%s, %s, %s, %s, 0, NOW(), NOW())
        ON CONFLICT (recipe_id) DO UPDATE
        SET author_id = EXCLUDED.author_id,
            title = EXCLUDED.title,
            description = EXCLUDED.description,
            updated_at = NOW()
        """,
        (
            payload["recipe_id"],
            payload["author_id"],
            payload["title"],
            payload["description"],
        ),
    )


def apply_ingredient_added(cursor, payload):
    cursor.execute(
        """
        INSERT INTO recipe_ingredient_read_model (
            ingredient_id,
            recipe_id,
            author_id,
            name,
            amount,
            created_at
        )
        VALUES (%s, %s, %s, %s, %s, NOW())
        ON CONFLICT (ingredient_id) DO NOTHING
        """,
        (
            payload["ingredient_id"],
            payload["recipe_id"],
            payload["author_id"],
            payload["name"],
            payload["amount"],
        ),
    )
    cursor.execute(
        """
        UPDATE recipe_read_model
        SET ingredients_count = (
                SELECT COUNT(*)
                FROM recipe_ingredient_read_model
                WHERE recipe_id = %s
            ),
            updated_at = NOW()
        WHERE recipe_id = %s
        """,
        (payload["recipe_id"], payload["recipe_id"]),
    )


def apply_favorite_recipe_added(cursor, payload):
    cursor.execute(
        """
        INSERT INTO user_favorite_recipe_read_model (user_id, recipe_id, created_at)
        VALUES (%s, %s, NOW())
        ON CONFLICT (user_id, recipe_id) DO NOTHING
        """,
        (payload["user_id"], payload["recipe_id"]),
    )


def apply_event(db_connection, event, routing_key):
    payload = event["payload"]
    event_type = event["event_type"]

    with db_connection.cursor() as cursor:
        cursor.execute(
            """
            INSERT INTO event_log (event_id, event_type, routing_key, payload)
            VALUES (%s, %s, %s, %s)
            ON CONFLICT (event_id) DO NOTHING
            RETURNING event_id
            """,
            (
                event["event_id"],
                event_type,
                routing_key,
                Json(event),
            ),
        )
        inserted = cursor.fetchone()
        if inserted is None:
            print(f"Skipped duplicate event #{event['event_id']}", flush=True)
            return

        if event_type == "UserRegistered":
            apply_user_registered(cursor, payload)
        elif event_type == "RecipeCreated":
            apply_recipe_created(cursor, payload)
        elif event_type == "IngredientAdded":
            apply_ingredient_added(cursor, payload)
        elif event_type == "FavoriteRecipeAdded":
            apply_favorite_recipe_added(cursor, payload)
        else:
            raise ValueError(f"Unknown event type: {event_type}")


def main() -> None:
    rabbit_connection = open_rabbitmq_connection()
    channel = rabbit_connection.channel()
    declare_read_model_queue(channel)
    channel.basic_qos(prefetch_count=10)

    db_connection = open_db_connection()

    def on_message(ch, method, properties, body):
        _ = properties
        try:
            event = json.loads(body.decode("utf-8"))
            apply_event(db_connection, event, method.routing_key)
            db_connection.commit()
            ch.basic_ack(delivery_tag=method.delivery_tag)
            print(
                f"Consumed event #{event['event_id']} {event['event_type']} from {method.routing_key}",
                flush=True,
            )
        except Exception as exc:  # noqa: BLE001 - message must be requeued on processing errors
            db_connection.rollback()
            print(f"Consumer error: {exc}", flush=True)
            ch.basic_nack(delivery_tag=method.delivery_tag, requeue=True)

    try:
        print(f"Read-model consumer started on queue {QUEUE_NAME}", flush=True)
        channel.basic_consume(queue=QUEUE_NAME, on_message_callback=on_message)
        channel.start_consuming()
    finally:
        db_connection.close()
        rabbit_connection.close()


if __name__ == "__main__":
    main()
