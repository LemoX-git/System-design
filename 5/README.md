### Оптимизации производительности
#### Кеширование
Реализован in-memory cache по стратегии **Cache-Aside** для endpoints:
- `GET /api/v1/users/by-login`
- `GET /api/v1/users/search`
- `GET /api/v1/recipes`
- `GET /api/v1/recipes/{recipe_id}/ingredients`
- `GET /api/v1/users/me/recipes`
- `GET /api/v1/users/me/favorites`

В ответах для кэшируемых endpoint добавлен заголовок:
- `X-Cache: MISS`
- `X-Cache: HIT`

#### Rate limiting
Для `POST /api/v1/auth/login` реализован **Fixed Window Counter**:
- лимит: **5 запросов / 60 секунд**;
- при превышении возвращается **429 Too Many Requests**;
- заголовки:
  - `X-RateLimit-Limit`
  - `X-RateLimit-Remaining`
  - `X-RateLimit-Reset`
