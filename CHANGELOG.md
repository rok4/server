# Serveur de diffusion ROK4

## Summary

Les configurations des couches, styles et tile matrix sets peuvent être des objets. Styles et TMS sont chargé à l'utilisation dans des couches, et les descripteurs des couches à charger sont listés.

## Changelog

### [Changed]

* Chargement des styles et des TMS au besoin : ce n'est que lors de l'utilisation du style dans une couche ou d'un TMS dans une pyramide que l'on charge le fichier correspondant.
* Styles et TMS peuvent être stockés en mode fichier ou objet. Dans la configuration du serveur, le répertoire précisé est préfixé par le type de stockage (`file://`, `s3://`, `swift://`, `ceph://`, mode fichier sinon).
* Initialisation des couches à partir d'une liste de descripteurs
* La configuration de la sortie de log en standard passe de `standard_output_stream_for_errors` à `standard_output`


### [Removed]

* Chargement de tous les descripteurs de couche d'un dossier
* Gestion de la persistence lors de l'ajout/modification/suppression d'une couche via l'API admin

<!-- 
### [Added]

### [Changed]

### [Deprecated]

### [Removed]

### [Fixed]

### [Security] 
-->
