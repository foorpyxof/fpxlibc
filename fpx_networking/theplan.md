# The grand plan:
---
Allow HttpServer class to function as either HTTP-only, WebSockets-only or both.

This way we can choose for:
- Simplicity (running as *one* executable)
- Performance (running as two different executables, possibly on different machines)

<img src="https://goodgirl.dev/_images/popcat.png" width="100">