## 6.1.2

### [Added]

* `Layer` : rétablissement d'un style par défaut au chargement de la couche. L'identifiant de ce style par défaut peut être fourni via le champ `default_style` dans le fichier de configuration des services, à la racine (optionnel, `normal` par défaut)

## 6.1.1

### [Fixed]

* `WMS` : correction de l'URL du site du fournisseur du service dans le GetCapabilities
* `WMTS` : correction du nom du fournisseur du service dans le GetCapabilities

## 6.1.0

### [Added]

* OGC API Tiles :
    * ajout des routes de découvertes pour connaître les éventuels styles, TMS et tuiles limites pour les couches
    * ajout des routes de lecture des TMS
    * ajout d'une route de découverte de ce service
    * ajout de la route de conformité aux classes

### [Changed]

* Les CRS et TMS additionnels dans le descripteur de couche sont à la racine pour pouvoir être exploités dans plusieurs services

## 6.0.1

### [Added]

* Tous les services sont désactivable, routes de santé incluses
* La racine de consultation des services de diffusion est configurable
* Titre, résumé et mots clés sont définis par service de diffusion

### [Changed]

* Passage complet en snake case
* Plus de style par défaut au chargement d'une couche, il faut forcément en fournir un. Pas de détection du style Inspire : ce sera toujours le premier qui sera utilisé par défaut à l'interrogation de la couche
* La route globale des services devient une route du service COMMON (futur OGC API Common)
* OGC API Tiles : dans le fichier de configuration des services, la section devient 'tiles'
* Utilisation des librairies boost (property tree) pour écrire du XML et json11 pour écrire du JSON
* Changement du format des réponses au getFeatureInfo interne (JSON -> FeatureCollection, XML -> Pixel.Band)
* WMTS : on ne met dans le getcapabilities que les styles que l'on pourra effectivement appliquer lors d'un GetTile
* Les GetFeatureInfo sur un WMS externe se font toujours avec le format image/tiff

### [Removed]

* Suppression de la classe WebService, l'envoi de requête est assuré par la classe Request

## 5.5.2

### [Fixed]

* `TMS` : les backslash sont échappés dans l'abstract pour éviter la casse du metadata.json

## 5.5.1

### [Added]

* Services WMS, WMTS, TMS et Tiles : il est possible de fournir un filename en paramètre de requête et imposer le téléchargement dans un navigateur

## 5.5.0

### [Added]

* `Layer` : lors de la configuration du get feature info de type EXTERNALWMS, il est possible de fournir des paramètres additionnels à ajouter à la requête vers le WMS source avec le champ "extra_query_params"

## 5.4.2

### [Changed]

* WMTS :
  * On ajoute à la liste des TMS au niveau du getcapabilities les TMS "de base" en entier

## 5.4.1

### [Added]

* WMS :
  * Ajout de la configuration du titre et nom de la couche racine dans le getCapabilities

### [Changed]

* WMS :
  * Déplacement des fonctions d'écriture d'une bbox du getCapabilities dans UtilsXML
  * Ajout de bbox au niveau de la couche racine dans le getCapabilities
  * L'attribution d'une couche est mise après les éventuelles métadonnées
* WMTS :
  * On ne liste plus les couches de tuiles vectorielles dans le getCapabilities
  * On exporte dans le getcapabilities un TMS différent pour chaque couple haut / bas présent dans les couches

## 5.3.0

### [Added]

* Sur les routes admin de ajout, modification et suppression de couche, on peut demander que les capacités des services ne soient pas regénérés
* Un nouvelle route admin (PUT /admin/layers) sans paramètre permet de demander la réécriture des capacités des services

### [Changed]

* Ajout de l'ID de thread dans les logs

## 5.2.1

### [Changed]

* En WMS, si la requête ne précise pas de version, on met la version 1.3.0 par défaut
* En WMTS, si la requête ne précise pas de version, on met la version 1.0.0 par défaut

## 5.2.0

### [Fixed]

* La fonction de copie d'une instance MetadataURL recopie bien le format et le href
* Lors de l'écriture de nombres flottants dans les réponses à un appel GetFeatureInfo, on précise les décimales
* Utils API Tiles 
    * correction l'extraction de la collection et du style lors d'appels à la route `/tiles/collections/{}/styles/{}/map/tiles/{}/{}/{}/{}(/info)?` 
    * le style par défaut de la couche est bien utilisée lors d'appels sans précision de style

### [Added]

* Gestion de la métadonnée de service pour l'OGC API Tiles

### [Changed]

* Pour le WMS, WMTS et OGC API Tiles, la fourniture d'une métadonnée de service est obligatoire dans le cas inspire
* Pour tous les services de diffusion, si une métadonnée de service est fournie, elle est mise dans la réponse au GetCapabilities
* `Layer` : si aucune bbox n'est fournie, elle est déduite du niveau le mieux résolu des pyramides utilisées par la couche * Dans le getcapabilities WMTS, dans le cas de TMS additionnel pour une couche, on ajoute une marge de une tuile pour les tuiles limites calculées

## 5.1.0

Implémentation partielle de l'API OGC Tiles - Part 1 [v1.0.0 final release](https://github.com/opengeospatial/ogcapi-tiles/releases/tag/1.0.0)

### [Added]

* Liste de nouvelles routes pour obtenir le **GetCapabilities**: 
    * /tiles/collections
     avec les paramètres facultatifs : 
       * bbox
       * limit
    * /tiles/collections/{layer}/map/tiles
    * /tiles/collections/{layer}/tiles
    * /tiles/tilematrixsets
    * /tiles/tilematrixsets/{tms}

* Liste des nouvelles routes pour obtenir le **GetTile** :

   * Raster
       * /tiles/map/tiles/{tms}/{level}/{row}/{col}
          avec le paramètre obligatoire : collections={layer}
       * /tiles/styles/{style}/map/tiles/{tms}/{level}/{row}/{col}
         avec le paramètre obligatoire : collections={layer}
       * /tiles/collections/{layer}/styles/{style}/map/tiles/{tms}/{level}/{row}/{col}
       * /tiles/collections/{layer}/map/tiles/{tms}/{level}/{row}/{col}

   * Vecteur
        * /tiles/tiles/{tms}/{level}/{row}/{col}?collections={layer}
        * /tiles/collections/{layer}/tiles/{tms}/{level}/{row}/{col}

### [Changed]

* Si la liste des styles est fournie mais vide, on le traite comme si rien n'était fourni : on ajoute le style par défaut

## 5.0.4

### [Fixed]

* SwiftContext : lecture des header insensible à la casse

## 5.0.1

### [Changed]

* Le style par défaut d'une couche est défini comme le premier style défini au niveau de la couche

### [Fixed]

* Pas de récupération du style lors de l'interrogation en TMS d'une couche de tuiles vectorielles

## 5.0.0

Les configurations des couches, styles et tile matrix sets peuvent être des objets. Styles et TMS sont chargé à l'utilisation dans des couches, et les descripteurs des couches à charger sont listés.

### [Changed]

* Chargement des styles et des TMS au besoin : ce n'est que lors de l'utilisation du style dans une couche ou d'un TMS dans une pyramide que l'on charge le fichier correspondant.
* Styles et TMS peuvent être stockés en mode fichier ou objet. Dans la configuration du serveur, le répertoire précisé est préfixé par le type de stockage (`file://`, `s3://`, `swift://`, `ceph://`, mode fichier sinon).
* Initialisation des couches à partir d'une liste de descripteurs
* La configuration de la sortie de log en standard passe de `standard_output_stream_for_errors` à `standard_output`


### [Removed]

* Chargement de tous les descripteurs de couche d'un dossier
* Gestion de la persistence lors de l'ajout/modification/suppression d'une couche via l'API admin

## 4.1.0

### [Added]

* Implémentation de routes de santé
  * `/healthcheck` : informations générales, version, date de lancement, statut général
  * `/healthcheck/info` : informations détaillées, listes de couches, styles et tile matrix sets
  * `/healthcheck/depends` : informations sur les stockages, nombres de contextes par type
  * `/healthcheck/threads` : informations sur les threads, statut, requêtes prises en charge, dernier temps de réponse

### [Fixed]

* Passage du nombre de jours sur 2 chiffres dans les appels S3


## 4.0.0

Le projet ROK4 a été totalement refondu, dans son organisation et sa mise à disposition. Les composants sont désormais disponibles dans des releases sur GitHub au format debian.

Cette release contient le serveur de diffusion de données raster ou vecteur stockées dans des pyramides ROK4.

### [Added]

* Ajout d'une API d'administration permettant la création, la modification et la suppression de couches. Le dossier des couches peut être vide au démarrage. Corps de requête 
* Les descripteurs de pyramide peuvent être lus directement sur un stockage objet
* Une couche peut exploiter plusieurs pyramides, sur des niveaux différents
* Gestion d'un cache des en-têtes des dalles, permettant d'éviter des lectures lors de la récupération de tuiles par le serveur.
* Possibilité de configurer des attributions et des métadonnées au niveau des couches, afin qu'elles apparaissent dans les réponses au GetCapabilities TMS et WMS
* Possibilité de lire des liens symboliques inter contenants sur des stockages objets
* La configuration d'une couche peut ne pas préciser de bbox : on considère alors l'étendue maximale des pyramides utilisées. Préciser une bbox permet de limiter cette étendue de définition de la couche.
* Les URL d'exposition du serveur sont précisées dans la configuration, pour être injectées au besoin dans les réponses.
* Le descripteur de couche peut préciser une liste de TMS additionnels différents de celui natif des pyramides exploitées. Cela permet d'interroger les données en WMTS selon ces TMS non natifs

### [Changed]

* Réorganisation des configurations et passage en JSON, dont les spécifications sont décrites sous forme de schémas JSON. Le passage en JSON concerne les configurations du serveur et des services, les descripteurs de couches, les styles et les TMS.
* Passage de la librairie PROJ à la version 6 

### [Removed]

* Suppression du support du WMS 1.1.1
* Suppression de la gestion de styles complexes (quand le valeur finale d'un pixel dépend des valeurs initiales du voisinage, comme le calcul de pente ou d'ombrage) à la volée
