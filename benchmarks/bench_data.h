#ifndef PYPEYAML_BENCH_DATA_H
#define PYPEYAML_BENCH_DATA_H

#include <string>
#include <vector>

namespace bench_data {

// Simple YAML data for basic operations
inline const char* small_yaml = R"(
name: PipeYAML
version: 0.1.0
language: C++17
license: MIT
authors:
  - name: Developer
    email: dev@example.com
)";

// Medium sized config data
inline const char* medium_yaml = R"(
database:
  host: localhost
  port: 5432
  name: pipeyaml_test
  user: admin
  pool_size: 10
  timeout: 30
  ssl: true
  options:
    max_connections: 100
    statement_timeout: 60000
    lock_timeout: 30000

cache:
  engine: redis
  host: redis.local
  port: 6379
  db: 0
  ttl: 3600
  cluster: false
  pool:
    min: 5
    max: 20

logging:
  level: info
  format: json
  output: stdout
  rotation:
    enabled: true
    max_size: 100MB
    max_backups: 30

server:
  host: 0.0.0.0
  port: 8080
  workers: 4
  timeout:
    read: 30
    write: 30
    idle: 120
  cors:
    enabled: true
    origins:
      - "*"
    methods:
      - GET
      - POST
      - PUT
      - DELETE
  rate_limit:
    enabled: true
    requests: 1000
    window: 3600
)";

// Deeply nested YAML structure
inline const char* nested_yaml = R"(
level1:
  level2:
    level3:
      level4:
        level5:
          level6:
            level7:
              level8:
                level9:
                  level10:
                    value: "deeply nested"
                    items:
                      - 1
                      - 2
                      - 3
    other_key: "sibling"
  name: "root sibling"
)";

// Complex YAML with anchors, aliases, and various types
inline const char* complex_yaml = R"(
---
defaults: &defaults
  timeout: 30
  retries: 3
  priority: normal

development:
  <<: *defaults
  url: http://dev.local
  debug: true

staging:
  <<: *defaults
  url: http://staging.example.com
  debug: false

production:
  <<: *defaults
  url: https://example.com
  debug: false
  pool: &pool
    min: 5
    max: 50
  database:
    host: db.example.com
    port: 5432
    pool: *pool
    credentials:
      user: admin
      password: secret123
      ssl_mode: require

features:
  - name: auth
    enabled: true
    config:
      provider: oauth2
      endpoints:
        authorize: /oauth/authorize
        token: /oauth/token
        refresh: /oauth/refresh
  - name: storage
    enabled: true
    config:
      provider: s3
      bucket: myapp-data
      region: us-east-1
      options:
        encryption: AES256
        compression: true
        caching:
          ttl: 300
          max_size: 1GB
)";

} // namespace bench_data

#endif // PYPEYAML_BENCH_DATA_H
