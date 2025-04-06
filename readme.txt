[WIP] Web server project
Implementing web server in C for better understanding of Rest API, POSIX threads, sockets etc, and to get practice in C lang.
Plan:

[X] PHASE 1
- [X] Implement threadpool, to handle threads creation and serving client in using multiple threads
- [X] Implement queue for threadpool
- [X] Make server handle GET requests to various resources in /web directory
- [X] Split into response and request .h files

[X] PHASE 2
- [X] Add dynamic name for web folder
- [X] Add args ip, port, pool size
- [X] Add logging tool to save logs
- [X] Add arg to pass for logs

[ ] PHASE 3
- [ ] POST support 
- [ ] Groom authorisation, storing credentials, encryption
- [ ] Groom other REST methods


