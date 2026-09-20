# high_perf_http


- [Quick start](##Quick_start)
  - [Build](###Build)
  - [Server](###Server)
  - [Project](###Project)
  - [Request](###Request)
  - [Autotest](###Autotest)
- [Code architecture]((##Code_architecture))

## Quick start
### Build
Cmake build requires cmake installed.

Manual build requires all files except those in the `autotest` folder.
```
cmake -S <path_to_source> -B <path_to_build>
cmake --build <path_to_build>
```

### Server
Start the http server using any method, for example, Python:
```
python -m http.server <source_port> --bind <host>
```
An example command to check if it works correctly:
```
curl -s http://<host>:<source_port>/
```

### Project
After starting the server and building the project, the proxy can be started with the command:
```
<path_to_build>/hphs <port> <source_port> <host>
```
An example command to check if it works correctly:
```
curl -s http://<host>:<port>/
```

### Request
The request example using curl.

The response should be the same in both cases.
```
curl -s http://<host>:<source_port>/
curl -s http://<host>:<port>/
```

### Autotest
When building with cmake, tests can be run with the command below.

If building manually, you need to build and run the automated test manually.

```
ctest --test-dir <path_to_build> --output-on-failure
```

## Code architecture
