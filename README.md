# ВсОШ: L4 балансировщик на eBPF/XDP

## Сборка
```bash
cmake -S . -B build
cmake --build build
```

## Запуск
```bash
./build/bin/vsoshlb --config config.yaml
```

## Настройка

Балансировщик конфигурируется по отдельным секциям, трафик в которые фильтруется по опредленным правилам: портам, протоколам и значениям SNI.
Пример базовой конфигурации:
```yaml
sections:
  testSection:
    port: 80
    proto: tcp
    upstreams:
      - 10.10.10.1
      - 10.10.10.2
  testSection1:
    port: 53
    proto: udp
    upstreams:
      - 1.1.1.1
      - 8.8.8.8
  testSection2:
    port: 443
    proto: tcp
    sni: testdomain.net
    upstreams:
      - 10.10.10.1
      - 10.10.10.2
```

Есть несколько способов фильтрации трафика: по подсетям (опция `allowSubnets` для списка разрешений, или `denySubnets` для списка запретов), по подстрокам (опция `filterSubstring`), по регулярным выражениям (опция `filterRegex`). Пример конфигурации:
```yaml
sections:
  testSection:
    port: 80
    proto: tcp
    upstreams:
      - 10.10.10.1
      - 10.10.10.2
    allowSubnets: 10.0.0.0/8
  testSection1:
    port: 81
    proto: tcp
    upstreams:
      - 10.10.10.1
      - 10.10.10.2
    denySubnets: 192.168.0.0/16
  testSection2:
    port: 82
    proto: tcp
    upstreams:
      - 10.10.10.1
      - 10.10.10.2
    filterSubstring: "os.system"
  testSection3:
    port: 83
    proto: tcp
    upstreams:
      - 10.10.10.1
      - 10.10.10.2
    filterRegex: "/([0-9]{1,3}\.){3}[0-9]{1,3}/i"
```

Можно настроить ограничение на количество запросов в секунду (опция `rpsLimiter`):
```yaml
sections:
  testSection:
    port: 80
    proto: tcp
    upstreams:
      - 10.10.10.1
      - 10.10.10.2
    rpsLimiter: 10/s
```
