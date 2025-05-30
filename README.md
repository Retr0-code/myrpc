# MyRPC

Own basic implementation of RPC server and client for linux

# Building

**Initialize git submodules before building**

Build requirements:
 - make
 - cmake
 - docker
 - docker-compose

---

 - `make all` target builds all binaries;
 - `make deb` target builds .deb packages and places them into _/deb_ direcory;
 - `make deb-clean` target removes all debian files;
 - `make repo-deploy` target starts nginx server with vhost `mysyslog.apt.net` and http port 8080/tcp;
 - `make repo-stop` target gracefully stop nginx container;

# Usage

To run server use systemctl daemon service. Config and ACL are stored in `/etc/myRPC`. To connect to the _myRPC_ server use `myrpc-client`.
