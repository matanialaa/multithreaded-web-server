# Multithreaded Web Server

A multithreaded HTTP/1.0 web server written in C with POSIX threads. It is built on a single-threaded skeleton server and adds a fixed-size **worker thread pool**, a **bounded request queue**, several **overload-handling policies**, and **per-request and per-thread statistics** returned in the HTTP response headers.

Built for the Operating Systems course (234123) at the Technion.

## Features

- **Thread pool:** the main thread only accepts connections and puts them in a queue. Worker threads take requests off the queue and handle them, either serving static files or running CGI programs for dynamic content.
- **Producer–consumer synchronization:** one mutex and condition variables (`pthread_cond_t`) keep the queue thread-safe. Workers never busy-wait.
- **Two queues:** a *waiting* queue for accepted requests and a *working* queue for requests currently being handled. Together they enforce the total capacity limit.
- **Overload policies:** these decide what happens when the queue is full.

  | Policy    | Behavior                                                                                                   |
  |-----------|------------------------------------------------------------------------------------------------------------|
  | `block`   | The main thread waits until a slot frees up.                                                               |
  | `dt`      | Drop tail: the new request is dropped.                                                                     |
  | `dh`      | Drop head: the oldest waiting request is dropped and the new one is queued.                                |
  | `bf`      | Block flush: waits until all queued and in-progress requests finish, then drops the new request.          |
  | `dynamic` | Drops the new request and grows the queue capacity by one, up to a set maximum.                            |
  | `random`  | Drops about half of the waiting requests at random, then queues the new one.                               |

- **Statistics headers:** every response includes timing and per-thread counters:

  ```
  Stat-Req-Arrival:: <arrival time>
  Stat-Req-Dispatch:: <time between arrival and dispatch>
  Stat-Thread-Id:: <worker id>
  Stat-Thread-Count:: <total requests handled by this thread>
  Stat-Thread-Static:: <static requests handled by this thread>
  Stat-Thread-Dynamic:: <dynamic requests handled by this thread>
  ```

## Project structure

| File                       | Description                                                          |
|----------------------------|----------------------------------------------------------------------|
| `server.c`                 | Argument parsing, thread pool setup, accept loop and overload policies |
| `request.c` / `request.h`  | HTTP request parsing, static/dynamic serving, statistics headers     |
| `queue.c` / `queue.h`      | Bounded linked-list queue that stores arrival timestamps             |
| `myThread.c` / `myThread.h`| Per-thread request counters                                          |
| `segel.c` / `segel.h`      | Wrappers for socket and system calls, with error checking (course-provided) |
| `client.c`                 | Simple HTTP client for testing                                       |
| `output.c`                 | Sample CGI program for dynamic requests                              |
| `home.html`, `favicon.ico` | Sample static content                                                |

## Build & run

Requires Linux, `gcc` and `make`.

```bash
make
./server <port> <threads> <queue_size> <schedalg> [max_size]
```

- `port`: a port number above 2000
- `threads`: number of worker threads
- `queue_size`: maximum number of requests the server holds at once
- `schedalg`: one of `block`, `dt`, `dh`, `bf`, `dynamic`, `random`
- `max_size`: only for `dynamic`; the largest the queue is allowed to grow

`make` also copies `output.cgi`, `home.html` and `favicon.ico` into `public/`, which is where the server serves files from. Run it from the project root:

```bash
./server 8080 4 10 block
```

Then test it:

```bash
./client localhost 8080 /home.html
./client localhost 8080 /output.cgi?1
```

Or open `http://localhost:8080/home.html` in a browser.
