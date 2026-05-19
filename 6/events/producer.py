import os
import time

import psycopg2

from common import declare_exchange, open_db_connection, open_rabbitmq_connection, publish_message

BATCH_SIZE = int(os.getenv("OUTBOX_BATCH_SIZE", "20"))
POLL_INTERVAL_SECONDS = float(os.getenv("OUTBOX_POLL_INTERVAL_SECONDS", "2"))
MAX_ATTEMPTS = int(os.getenv("OUTBOX_MAX_ATTEMPTS", "10"))


def load_new_events(db_connection):
    with db_connection.cursor() as cursor:
        cursor.execute(
            """
            SELECT id,
                   event_type,
                   event_version,
                   routing_key,
                   aggregate_type,
                   aggregate_id,
                   payload,
                   created_at
            FROM event_outbox
            WHERE status = 'NEW'
              AND attempts < %s
            ORDER BY id
            LIMIT %s
            FOR UPDATE SKIP LOCKED
            """,
            (MAX_ATTEMPTS, BATCH_SIZE),
        )
        return cursor.fetchall()


def mark_published(db_connection, event_id: int) -> None:
    with db_connection.cursor() as cursor:
        cursor.execute(
            """
            UPDATE event_outbox
            SET status = 'PUBLISHED',
                published_at = NOW(),
                last_error = NULL
            WHERE id = %s
            """,
            (event_id,),
        )


def mark_failed_attempt(db_connection, event_id: int, error: str) -> None:
    with db_connection.cursor() as cursor:
        cursor.execute(
            """
            UPDATE event_outbox
            SET attempts = attempts + 1,
                status = CASE WHEN attempts + 1 >= %s THEN 'FAILED' ELSE 'NEW' END,
                last_error = %s
            WHERE id = %s
            """,
            (MAX_ATTEMPTS, error[:1000], event_id),
        )


def process_batch(db_connection, channel) -> int:
    rows = load_new_events(db_connection)
    if not rows:
        db_connection.commit()
        return 0

    published = 0
    for row in rows:
        try:
            publish_message(channel, row)
            mark_published(db_connection, row["id"])
            published += 1
            print(
                f"Published event #{row['id']} {row['event_type']} via {row['routing_key']}",
                flush=True,
            )
        except Exception as exc:  # noqa: BLE001 - failed publish must be stored in outbox
            mark_failed_attempt(db_connection, row["id"], str(exc))
            print(f"Could not publish event #{row['id']}: {exc}", flush=True)

    db_connection.commit()
    return published


def main() -> None:
    rabbit_connection = open_rabbitmq_connection()
    channel = rabbit_connection.channel()
    channel.confirm_delivery()
    declare_exchange(channel)

    db_connection = open_db_connection()
    try:
        print("Outbox producer started", flush=True)
        while True:
            try:
                count = process_batch(db_connection, channel)
                if count == 0:
                    time.sleep(POLL_INTERVAL_SECONDS)
            except (psycopg2.Error, OSError) as exc:
                db_connection.rollback()
                print(f"Temporary producer error: {exc}", flush=True)
                time.sleep(POLL_INTERVAL_SECONDS)
    finally:
        db_connection.close()
        rabbit_connection.close()


if __name__ == "__main__":
    main()
