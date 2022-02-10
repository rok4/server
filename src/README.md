# Partie ROK4SERVER

![Logo ROK4SERVER](../docs/images/rok4server.png)

- [Déploiement du serveur](#déploiement-du-serveur)
  - [Configurer et lancer le serveur ROK4](#configurer-et-lancer-le-serveur-rok4)
  - [Installer et configurer NGINX](#installer-et-configurer-nginx)
- [Accès aux capacités du serveur](#accès-aux-capacités-du-serveur)
- [Fonctionnement général du serveur ROK4](#fonctionnement-général-du-serveur-rok4)
  - [Identification du service et du type de requête](#identification-du-service-et-du-type-de-requête)
  - [Accès aux données](#accès-aux-données)
  - [Gestion des configurations](#gestion-des-configurations)
  - [Personnalisation des points d'accès aux services](#personnalisation-des-points-daccès-aux-services)

## Déploiement du serveur

Le serveur ROK4 est lancé en mode stand alone. Nous utilisierons ici Nginx comme serveur front pour "traduire" les requêtes HTTP en FCGI et les rediriger vers le serveur ROK4.

### Configurer et lancer le serveur ROK4

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

La ligne de commande permettant de lancer ROK4 comme instance autonome est la suivante :
```
rok4 -f /chemin/vers/fichier/server.json &
```

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

## Fonctionnement général du serveur ROK4

### Identification du service et du type de requête

Lorsque le serveur reçoit une requête, c'est le premier élément du chemin qui détermine le service :

* `/` -> requête globale
* `/wmts` -> requête WMTS
* `/wms` -> requête WMS
* `/tms` -> requête TMS
* `/admin` -> requête d'administration

En WMS et WMTS, si c'est une requête POST, le corps est interprété pour extraire les informations. Seuls le getCapabilities, le getMap et le getTile sont disponibles en POST. Le paramètre de requête ou le corps doit confirmer le service pour que la requête soit valide.

Les requêtes gérées par le serveur sont décrites dans des spécifications au format Open API.

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
