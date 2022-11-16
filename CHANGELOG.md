# Serveur de diffusion ROK4

## Summary

Des routes de santé sont ajoutées afin de pouvoir surveiller l'activité du serveur à plusieurs niveaux (version, threads, stockages...)

## Changelog

### [Added]

* Implémentation de routes de santé
  * `/healthcheck` : informations générales, version, date de lancement, statut général
  * `/healthcheck/info` : informations détaillées, listes de couches, styles et tile matrix sets
  * `/healthcheck/depends` : informations sur les stockages, nombres de contextes par type
  * `/healthcheck/threads` : informations sur les threads, statut, requêtes prises en charge, dernier temps de réponse


### [Changed]

* Chargement des styles et des TMS au besoin : ce n'est que lors de l'utilisation du style dans une couche ou d'un TMS dans une pyramide que l'on charge le fichier correspondant.
* Styles et TMS peuvent être stockés en mode fichier ou objet. Dans la configuration du serveur, le répertoire précisé est préfixé par le type de stockage (`file://`, `s3://`, `swift://`, `ceph://`, mode fichier sinon).

<!-- 
### [Added]

### [Changed]

### [Deprecated]

### [Removed]

### [Fixed]

### [Security] 
-->