# Rebuild Charter

## But

Graphion doit etre reconstruit autour d'un seul pipeline :

- `.gion -> bytecode -> VM`

Le langage doit etre optimise de maniere generale.
Il ne doit pas etre optimise pour un fichier de test, un benchmark particulier, ou un sous-ensemble cache.

## Principes

### 1. Pipeline unique

Toute forme du langage supportee doit passer par :

- `.gion -> bytecode -> VM`

Il ne doit pas exister d'autre moteur semantique.
Le bytecode genere doit etre inspectable.

### 2. Entree `.gion` preservee

L'entree `.gion` doit rester une vraie entree du langage.
Elle ne doit pas etre contournee pour faire marcher une feature, un test ou un benchmark.

### 3. Pas de fallback semantique

Si une forme du langage n'est pas encore supportee par le pipeline `.gion -> bytecode -> VM` :

- erreur claire

Il ne doit pas y avoir de fallback d'execution vers un autre moteur semantique.

### 4. Meme semantique partout

Un meme `.gion` doit produire la meme semantique quel que soit le contexte d'execution :

- release
- test
- benchmark

Le comportement ne doit pas dependre du chemin d'execution, du mode debug/release, ou du benchmark lance.

### 5. Optimisation generale uniquement

Toute optimisation doit viser une forme generale du langage, le lowering bytecode, ou la VM.

Il ne faut jamais optimiser :

- pour un test
- pour un benchmark
- pour un fichier particulier
- pour un cas ultra-specifique qui ne represente pas le langage

## Validation d'une feature

L'ordre de validation est strict :

1. fonctionnement general
2. tests
3. benchmarks

Une feature n'est validee que si elle passe ces trois niveaux dans cet ordre.

### 1. Fonctionnement general

Une feature doit fonctionner dans un `.gion` general si le code ecrit par l'utilisateur entre bien dans le cadre de cette feature.

Cela doit etre vrai :

- quel que soit le nom des variables
- quelles que soient les valeurs dans le domaine supporte
- quel que soit l'ordre legal des lignes
- quel que soit le contenu global du fichier, tant qu'il reste semantiquement coherent

Si une feature ne marche que dans un fichier de test, un benchmark, ou un cas fixe, elle n'est pas faite.

### 2. Compatibilite cumulative

Une nouvelle feature ne doit pas casser les precedentes.

Une feature validee doit fonctionner :

- seule
- et en combinaison avec les features deja validees

Si une feature ne marche qu'isolee, elle n'est pas validee.

### 3. Tests obligatoires

Toute feature doit passer :

- des tests cibles
- des tests de non-regression
- des tests d'integration inter-features

Les tests doivent varier pour eviter les faux positifs :

- noms
- valeurs
- ordre legal des lignes
- combinaisons de features

### 4. Erreurs claires

Si le code est hors perimetre supporte :

- erreur claire

Si le programme utilisateur est invalide :

- erreur claire

Exemples :

- syntaxe invalide
- variable non definie
- forme non encore supportee

Il ne doit pas y avoir de support implicite, partiel, ou cache.

### 5. Validation tracable

L'etat d'une feature doit rester explicite.

On distingue si necessaire :

- implemente
- teste
- valide
- benchmarke

La trace doit indiquer au minimum :

- support
- tests
- benchmarks
- erreurs connues

## Benchmarks

### 1. Role des benchmarks

Un benchmark sert a verifier les performances.
Il ne sert pas a prouver qu'une feature fonctionne.

Un benchmark n'a de valeur qu'apres validation fonctionnelle et passage des tests.

### 2. Representativite

Un benchmark doit representer une forme generale du langage.

Il ne doit pas tirer sa valeur d'un traitement de faveur applique a :

- un fichier precis
- un ordre de lignes particulier
- un nommage particulier
- un cas de test specialise

Un benchmark specialise n'a pas de valeur de validation.

### 3. Seuils d'acceptation

Une feature ne passe la validation benchmark que si :

- `VM < x1.15`
- `.gion < x1.5`
- `variation < 10%`

Ces seuils ne valent que pour des benchmarks representatifs.

## Etat actuel

A l'etat actuel du repo :

- l'interpreteur legacy ne doit plus servir de base de reconstruction
- l'entree `.gion` existe toujours
- la base actuelle doit seulement servir de point de depart propre pour reconstruire le vrai pipeline `.gion -> bytecode -> VM`
