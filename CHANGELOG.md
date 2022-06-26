# Serveur de diffusion ROK4

## Summary

Le projet ROK4 a été totalement refondu, dans son organisation et sa mise à disposition. Les composants sont désormais disponibles dans des releases sur GitHub au format debian.

Cette release contient le serveur de diffusion de données raster ou vecteur stockées dans des pyramides ROK4.

## Changelog

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

<!-- 
### [Added]

### [Changed]

### [Deprecated]

### [Removed]

### [Fixed]

### [Security] 
-->