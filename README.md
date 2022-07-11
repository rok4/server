# Serveur de diffusion WMS, WMTS et TMS

Le serveur fait partie du projet open-source ROK4 (sous licence CeCILL-C) développé par les équipes du projet [Géoportail](https://www.geoportail.gouv.fr)([@Geoportail](https://twitter.com/Geoportail)) de l’[Institut National de l’Information Géographique et Forestière](https://ign.fr) ([@IGNFrance](https://twitter.com/IGNFrance)). Il est écrit en C++ et permet la diffusion de données raster ou vecteur.

Le serveur implémente les standards ouverts de l’Open Geospatial Consortium (OGC) WMS 1.3.0 et WMTS 1.0.0, ainsi que le TMS (Tile Map Service). Il vise deux objectifs principaux :

* L’utilisation d’un cache de données raster unique permettant de servir indifféremment des flux WMS, WMTS et TMS
* Des performances de traitement d’image et de diffusion accrues
* La diffusion de tuiles vecteur telles qu'elles sont stockées, sans transformation (TMS uniquement)
* La diffusion en WMTS selon des Tile Matrix Sets différents de celui de la pyramide utilisée.

Les pyramides de données utilisées sont produites via les outils de [prégénération](https://github.com/rok4/pregeneration) et de [génération](https://github.com/rok4/generation).

- [Installation via le paquet debian](#installation-via-le-paquet-debian)
- [Installation depuis les sources](#installation-depuis-les-sources)
  - [Récupération du projet](#récupération-du-projet)
  - [Variables CMake](#variables-cmake)
  - [Dépendances à la compilation](#dépendances-à-la-compilation)
  - [Compilation et installation](#compilation-et-installation)
- [Variables d'environnement utilisées dans les librairies de core-cpp](#variables-denvironnement-utilisées-dans-les-librairies-de-core-cpp)
- [Utilisation du serveur](#utilisation-du-serveur)
  - [Configurer le serveur](#configurer-le-serveur)
  - [Lancer le serveur](#lancer-le-serveur)
    - [En ligne de commande](#en-ligne-de-commande)
    - [En tant que service systemctl](#en-tant-que-service-systemctl)
  - [Installer et configurer NGINX](#installer-et-configurer-nginx)
- [Accès aux capacités du serveur](#accès-aux-capacités-du-serveur)
- [Fonctionnement général du serveur](#fonctionnement-général-du-serveur)
  - [Identification du service et du type de requête](#identification-du-service-et-du-type-de-requête)
  - [Accès aux données](#accès-aux-données)
  - [Gestion des configurations](#gestion-des-configurations)
  - [Personnalisation des points d'accès aux services](#personnalisation-des-points-daccès-aux-services)

## Installation via le paquet debian

Télécharger les paquets sur GitHub :
* [le serveur](https://github.com/rok4/server/releases/)
* [les styles](https://github.com/rok4/styles/releases/)
* [les TMS](https://github.com/rok4/tilematrixsets/releases/)

```
apt install ./rok4-styles_<version>_all.deb
apt install ./rok4-tilematrixsets_<version>_all.deb
apt install ./ROK4SERVER-<version>-Linux-64bit.deb
```

## Installation depuis les sources

### Récupération du projet

`git clone --recursive https://github.com/rok4/server`
ou

```bash
git clone https://github.com/rok4/server
git submodule update --init --recursive
```

### Variables CMake

* `CMAKE_INSTALL_PREFIX` : dossier d'installation du serveur. Valeur par défaut : `/usr/local`
* `BUILD_VERSION` : version du serveur compilé. Valeur par défaut : `0.0.0`
* `OBJECT_ENABLED` : active la compilation des classes de gestion des stockages objet. Valeur par défaut : `0`, `1` pour activer.
* `DEBUG_BUILD` : active la compilation en mode debug. Valeur par défaut : `0`, `1` pour activer.
* `UNITTEST_ENABLED` : active la compilation des tests unitaires. Valeur par défaut : `0`, `1` pour activer.

### Dépendances à la compilation

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
  * Si `OBJECT_ENABLED` à `1`
    * librados-dev
  * Si `UNITTEST_ENABLED` à `1`
    * libcppunit-dev

### Compilation et installation

```bash
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/ -DBUILD_VERSION=0.0.1 -DOBJECT_ENABLED=1 ..
make
make install
```

## Variables d'environnement utilisées dans les librairies de core-cpp

Leur définition est contrôlée à l'usage.

* Pour le stockage CEPH
    - `ROK4_CEPH_CONFFILE`
    - `ROK4_CEPH_USERNAME`
    - `ROK4_CEPH_CLUSTERNAME`
* Pour le stockage S3
    - `ROK4_S3_URL`
    - `ROK4_S3_KEY`
    - `ROK4_S3_SECRETKEY`
* Pour le stockage SWIFT
    - `ROK4_SWIFT_AUTHURL`
    - `ROK4_SWIFT_USER`
    - `ROK4_SWIFT_PASSWD`
    - `ROK4_SWIFT_PUBLICURL`
    - Si authentification via Swift
        - `ROK4_SWIFT_ACCOUNT`
    - Si connection via keystone (présence de `ROK4_KEYSTONE_DOMAINID`)
        - `ROK4_KEYSTONE_DOMAINID`
        - `ROK4_KEYSTONE_PROJECTID`
    - `ROK4_SWIFT_TOKEN_FILE` afin de sauvegarder le token d'accès, et ne pas le demander si ce fichier en contient un
* Pour configurer l'usage de libcurl (intéraction SWIFT et S3)
    - `ROK4_SSL_NO_VERIFY`
    - `HTTP_PROXY`
    - `HTTPS_PROXY`
    - `NO_PROXY`

## Utilisation du serveur

Le serveur ROK4 est lancé en mode stand alone. Nous utiliserons ici Nginx comme serveur front pour "traduire" les requêtes HTTP en FCGI et les rediriger vers le serveur ROK4.

### Configurer le serveur

Dans le fichier `server.json`, on précise le port d'écoute :

```json
    "port": ":9000"
```

On configure les logs de manière à les retrouver dans un fichier par jour :

```json
"logger": {
    "output": "rolling_file",
    "level": "info",
    "file_prefix": "/var/log/rok4",
    "file_period": 86400
}
```

* Les paramètres possibles du fichier de configuration `server.json` sont décrits [ici](./config/server.schema.json)
* Les paramètres possibles du fichier de configuration `services.json` sont décrits [ici](./config/services.schema.json)

### Lancer le serveur

#### En ligne de commande

La ligne de commande permettant de lancer ROK4 comme instance autonome est la suivante :
```bash
rok4 -f /chemin/vers/fichier/server.json &
```

#### En tant que service systemctl

Selon l'emplacement d'installation, le fichier dans `service/rok4.service` peut déjà être à un endroit pris en compte par systemctl (comme `/usr/lib/systemd/system`). Celui ci est écrit pour un déploiement à la racine, modifiez les chemins pour qu'il soit adapté à votre déploiement. Si l'installation a été faite via le paquet debian, le service est déjà correctement installé, et les configurations sont dans `/etc/rok4`.

```
EnvironmentFile=/etc/rok4/config/env
WorkingDirectory=/etc/rok4/config/
```

Le fichier `config/env` permet de définir les variables d'environnement propres au serveur pour configurer l'utilisation de stockages objets (voir [ici](#variables-denvironnement-utilisées-dans-les-librairies-de-core-cpp)).

Le serveur est lancé en tant que user (et group) `rok4`. Il convient donc de le créer : `useradd rok4`.

Il suffit alors de recharger le démon systemctl avec la commande `systemctl daemon-reload`. Vous pouvez maintenant piloter le serveur ROK4 via ce système, comme le démarrer avec `systemctl start rok4`.

### Installer et configurer NGINX

* Sous Debian : `apt install nginx`
* Sous Centos : `yum install nginx`

Remplacer le fichier `default` présent dans le répertoire `/etc/nginx/sites-enabled` par le contenu suivant :

```
upstream rok4 { server localhost:9000; }

server {
    listen 80;
    root /var/www;
    server_name localhost;

    access_log /var/log/rok4_access.log;
    error_log /var/log/rok4_error.log;

    location /rok4 {
        rewrite /rok4/?(.*) /$1 break;
        fastcgi_pass rok4;
        include fastcgi_params;
    }
}
```

On redémarre nginx : `systemctl restart nginx`

## Accès aux capacités du serveur

* Liste des services de diffusion : http://localhost/rok4/
* GetCapabilities des services de diffusion
    - WMS : http://localhost/rok4/wms?request=GetCapabilities&service=WMS
    - WMTS : http://localhost/rok4/wmts?request=GetCapabilities&service=WMTS
    - TMS : http://localhost/rok4/tms/1.0.0
* Racine de l'API d'administration : http://localhost/rok4/admin/

## Fonctionnement général du serveur

### Identification du service et du type de requête

Lorsque le serveur reçoit une requête, c'est le premier élément du chemin qui détermine le service :

* `/` -> requête globale
* `/wmts` -> requête WMTS
* `/wms` -> requête WMS
* `/tms` -> requête TMS
* `/admin` -> requête d'administration

En WMS et WMTS, si c'est une requête POST, le corps est interprété pour extraire les informations. Seuls le getCapabilities, le getMap et le getTile sont disponibles en POST. Le paramètre de requête ou le corps doit confirmer le service pour que la requête soit valide.

Les requêtes gérées par le serveur sont décrites dans des spécifications au format [Open API](openapi.yaml).

### Accès aux données

L'accès aux données stockées dans les pyramides se fait toujours par tuile. Dans le cas du TMS et WMTS, la requête doit contenir les indices (colonne et ligne) de la tuile voulue. La tuile est ensuite renvoyée sans traitement, ou avec simple ajout/modification de l'en-tête (en TIFF et en PNG). Dans le cas d'un GetMap en WMS, l'emprise demandée est convertie dans le système de coordonnées de la pyramide, et on identifie ainsi la liste des indices des tuiles requises pour calculée l'image voulue. De la même manière qu'en WMTS et TMS, le serveur sait à partir des indices où récupérer la donnée dans l'espace de stockage des pyramides.

Avec les indices de la tuile à lire, le serveur calcule le nom de la dalle qui la contient et le numéro de la tuile dans cette dalle. Le serveur commence par récupérer le header et l'index de la dalle, contenant les offsets et les tailles de toutes les tuiles de la dalle. Le header fait toujours 2048 octets et l'index a une taille connue par le serveur.

Dans le cas du stockage objet (CEPH, S3, SWIFT), les objets symboliques ne font jamais plus de 2047 octets. Cette première lecture permet donc de les identifier (on lit moins que voulu). Dans ce cas, ce qu'on a lu contient le nom de l'objet contenant réellement la donnée (précédé de la signature `SYMLINK#`). On va donc reproduire l'opération sur ce nouvel objet, qui lui ne doit pas être un objet symbolique (pas de lien en cascade). En mode fichier, ce mécanisme est transparent pour le serveur car géré par le système de fichiers.

Une fois que l'on a récupéré l'index, et grâce au numéro de la tuile dans la dalle, on va pouvoir connaître l'offset et la taille. On va donc faire une deuxième lecture de la dalle pour récupérer la donnée de la tuile.

### Gestion des configurations

Au démarrage du serveur, toutes les configurations (serveur, services, tile matrix sets, styles et couches) sont chargées depuis les fichiers. Lors du fonctionnement, si une requête d'administration modifie cette configuration, les fichiers sont mis à jour (écrits, écrasés ou supprimés).

### Personnalisation des points d'accès aux services

Pour que les URLs présentes dans les réponses des services soient correctes malgré des réecritures, il est important de bien renseigner les champs suivant dans le fichier `services.json`:

```json
    "wms": {
        "endpoint_uri": "http://localhost/rok4/wms",
    },
    "wmts": {
	    "endpoint_uri": "http://localhost/rok4/wmts"
    },
    "tms": {
        "endpoint_uri": "http://localhost/rok4/tms"
    }
```
