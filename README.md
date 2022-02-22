# Serveur de diffusion WMS, WMTS et TMS

- [Récupération du projet](#récupération-du-projet)
- [Variables CMake](#variables-cmake)
- [Dépendances à la compilation](#dépendances-à-la-compilation)
- [Compilation et installation](#compilation-et-installation)
- [Dépendances à l'exécution](#dépendances-à-lexécution)

## Récupération du projet

`git clone --recursive https://github.com/rok4/server`

## Variables CMake

* `OBJECT_ENABLED` : active la compilation des classes de gestion des stockages objet

## Dépendances à la compilation

* Submodule GIT
  * `https://github.com/rok4/core-cpp`
* Paquets debian
  * libfcgi-dev
  * libtinyxml-dev
  * zlib1g-dev
  * libcurl4-openssl-dev
  * libproj-dev
  * libssl-dev
  * libturbojpeg0-dev
  * libjpeg-dev
  * libc6-dev
  * libjson11-1-dev
  * libboost-log-dev
  * libboost-filesystem-dev
  * libboost-system-dev
  * libsqlite3-dev
  * Si `OBJECT_ENABLED` à 1
    * librados-dev

## Compilation et installation

```shell
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/ -DBUILD_VERSION=0.0.1 -DOBJECT_ENABLED=1 ..
make
make install
```

## Dépendances à l'exécution

* Dépôt GIT
    * `https://github.com/rok4/tilematrixsets`
    * `https://github.com/rok4/styles`