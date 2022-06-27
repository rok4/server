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

<!-- 
### [Added]

### [Changed]

### [Deprecated]

### [Removed]

### [Fixed]

### [Security] 
-->