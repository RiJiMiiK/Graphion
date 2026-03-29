# Rebuild Charter

## But

Graphion doit etre reconstruit autour d'un seul pipeline :

- `source Graphion -> tokens/parsing -> representation interne du code -> bytecode -> VM`

Le langage doit etre optimise de maniere generale.
Il ne doit jamais etre optimise pour un seul benchmark, un seul fichier d'exemple, ou un test particulier.

## Principes

### 1. Pipeline unique

Toute forme du langage supportee doit passer par un seul pipeline :

- `source Graphion -> tokens/parsing -> representation interne du code -> bytecode -> VM`

Il ne doit pas exister d'autre moteur semantique cache.
Le bytecode produit doit rester inspectable.

### 2. Entree `.gion` preservee

L'entree `.gion` reste l'entree normale du langage.
Elle ne doit pas etre contournee pour faire marcher une feature, un test, ou un benchmark.

### 3. Pas de fallback semantique

Si une forme du langage n'est pas encore supportee :

- erreur claire

Il ne doit pas y avoir de deuxieme moteur d'execution qui "fait quand meme marcher" le programme.

### 4. Meme semantique partout

Un meme programme `.gion` doit produire la meme semantique en :

- release
- test
- benchmark

Le comportement ne doit pas dependre d'un chemin d'execution special, d'un binaire de bench, ou d'une optimisation cachee.

### 5. Optimisation generale uniquement

Toute optimisation doit viser :

- le parsing
- la representation interne
- le lowering bytecode
- la VM

Il ne faut jamais optimiser :

- pour un seul test
- pour un seul benchmark
- pour un fichier particulier
- pour un cas artificiel qui ne represente pas le langage

## Validation d'une feature

L'ordre de validation est strict :

1. fonctionnement general
2. tests
3. benchmarks

### 1. Fonctionnement general

Une feature doit fonctionner dans un `.gion` general tant que le programme utilisateur reste dans le perimetre supporte.

Cela doit etre vrai :

- quels que soient les noms
- quelles que soient les valeurs supportees
- quel que soit l'ordre legal des lignes
- quelle que soit la combinaison avec les features deja supportees

### 2. Compatibilite cumulative

Une nouvelle feature ne doit pas casser les precedentes.

Une feature validee doit marcher :

- seule
- et en combinaison avec les features deja validees

### 3. Tests obligatoires

Toute feature doit avoir :

- des tests cibles
- des tests de non-regression
- des tests d'erreur
- des tests d'integration inter-features

Les tests doivent varier pour limiter les faux positifs :

- noms
- valeurs
- cas negatifs
- combinaisons de features

### 4. Erreurs claires

Si le code est hors perimetre supporte :

- erreur claire

Si le programme utilisateur est invalide :

- erreur claire

Exemples :

- syntaxe invalide
- variable inconnue
- operande inconnu
- operation non supportee pour les types fournis

### 5. Validation tracable

L'etat d'une feature doit rester explicite.

On distingue si necessaire :

- implemente
- teste
- valide
- benchmarke

## Benchmarks

### 1. Role des benchmarks

Un benchmark sert a mesurer la performance.
Il ne sert pas a prouver qu'une feature fonctionne.

### 2. Representativite

Un benchmark doit representer une forme generale du langage.

Il ne doit pas tirer sa valeur d'un traitement de faveur applique a :

- un fichier precis
- un ordre de lignes particulier
- un nommage particulier
- un cas specialise

### 3. Seuils d'acceptation

Pour les benchmarks representatifs :

- `VM / Rust < 1.15x`
- `.gion / Rust = 2x a 3x` comme cible principale
- `.gion / Rust < 2x` seulement comme stretch goal si de vrais leviers generaux restent disponibles
- `variation < 10%`

## Etat actuel

A l'etat actuel du repo :

- la priorite fonctionnelle est le sous-ensemble `.gion` scalaire
- la VM est deja un backend reel et mesurable
- la reconstruction doit continuer sans recreer de fallback semantique
- la doc utilisateur doit decrire seulement ce qui est vraiment implemente
