#!/bin/bash

curl 'http://127.0.0.1:9999' \
-H "Connection: Upgrade" \
-H "Upgrade: websocket" \
-H "Sec-WebSocket-Key: dQKSfB/ZOzYjAxrWReKghQ==" \
-H "Sec-WebSocket-Version: 13" \
--include -o curlout.txt -N -v
