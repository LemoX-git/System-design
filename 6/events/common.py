import json
import os
import time
from datetime import datetime, timezone
from typing import Any, Dict

import pika
import psycopg2
from pika.adapters.blocking_connection import BlockingChannel
from psycopg2.extras import Json, RealDictCursor

DB_DSN = os.getenv(
    "DB_DSN",
    "host=postgres port=5432 dbname=recipe_service user=recipe_user password=recipe_password",
)
RABBITMQ_URL = os.getenv("RABBITMQ_URL", "amqp://recipe:recipe@rabbitmq:5672/%2F")
EXCHANGE_NAME = os.getenv("EXCHANGE_NAME", "recipe.events")
QUEUE_NAME = os.getenv("QUEUE_NAME", "recipe.events.read_model")


def utc_now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def open_db_connection():
    return psycopg2.connect(DB_DSN, cursor_factory=RealDictCursor)


def open_rabbitmq_connection(retries: int = 30, delay_seconds: float = 2.0):
    last_error: Exception | None = None
    for attempt in range(1, retries + 1):
        try:
            return pika.BlockingConnection(pika.URLParameters(RABBITMQ_URL))
        except Exception as exc:  # noqa: BLE001 - reconnect loop must catch broker startup errors
            last_error = exc
            print(f"RabbitMQ is not ready yet, attempt {attempt}/{retries}: {exc}", flush=True)
            time.sleep(delay_seconds)
    raise RuntimeError(f"Could not connect to RabbitMQ: {last_error}")


def declare_exchange(channel: BlockingChannel) -> None:
    channel.exchange_declare(
        exchange=EXCHANGE_NAME,
        exchange_type="topic",
        durable=True,
    )


def declare_read_model_queue(channel: BlockingChannel) -> None:
    declare_exchange(channel)
    channel.queue_declare(queue=QUEUE_NAME, durable=True)
    for routing_key in (
        "user.registered",
        "recipe.created",
        "ingredient.added",
        "favorite.recipe.added",
    ):
        channel.queue_bind(
            exchange=EXCHANGE_NAME,
            queue=QUEUE_NAME,
            routing_key=routing_key,
        )


def to_message(row: Dict[str, Any]) -> Dict[str, Any]:
    created_at = row["created_at"]
    if hasattr(created_at, "isoformat"):
        created_at = created_at.isoformat()

    return {
        "event_id": row["id"],
        "event_type": row["event_type"],
        "event_version": row["event_version"],
        "occurred_at": created_at,
        "producer": "recipe-service",
        "aggregate": {
            "type": row["aggregate_type"],
            "id": row["aggregate_id"],
        },
        "payload": row["payload"],
    }


def publish_message(channel: BlockingChannel, row: Dict[str, Any]) -> None:
    message = to_message(row)
    body = json.dumps(message, ensure_ascii=False).encode("utf-8")
    channel.basic_publish(
        exchange=EXCHANGE_NAME,
        routing_key=row["routing_key"],
        body=body,
        properties=pika.BasicProperties(
            content_type="application/json",
            delivery_mode=pika.DeliveryMode.Persistent,
            message_id=str(row["id"]),
            type=row["event_type"],
            timestamp=int(time.time()),
            headers={
                "event_id": row["id"],
                "event_type": row["event_type"],
                "event_version": row["event_version"],
                "published_at": utc_now_iso(),
            },
        ),
        mandatory=False,
    )


def insert_event_log(db_connection, event: Dict[str, Any], routing_key: str) -> None:
    with db_connection.cursor() as cursor:
        cursor.execute(
            """
            INSERT INTO event_log (event_id, event_type, routing_key, payload)
            VALUES (%s, %s, %s, %s)
            ON CONFLICT (event_id) DO NOTHING
            """,
            (
                event["event_id"],
                event["event_type"],
                routing_key,
                Json(event),
            ),
        )
