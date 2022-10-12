# PyBase

## Relacijske pod. baze
`PyBase` knjižnca temelji na principu relacijskih podatkovnih baz. Gre za pristop, kjer imamo točno dolečeno strukturo podatkov in podatke shranjujemo v tabele, ki se med sabo vežejo s pomočjo tujih ključev. 

Pri implementaciji `PyBase` podatke shranjujemo v binarne datoteke, kar nam omogoča preprosto in hitro iskanje (npr. če vemo da je vsaka posamezna vrstica dolga `Z` bajtov potem lahko `X`-to vrstico najdemo, tako da se pomaknemo na `X*Z`-ti bajt).

Primer strukture:
```yml
students
    - fullname string size=100
    - email string size=255
    - courses courses link multi=1
courses
    - name string size=100
    - teacher teachers link multi=0
    - students students link multi=1
teachers
    - fullname string size=100
    - email string size=255
    - birthdate datetime
    - courses courses link multi=1
```

## Integracija direktno v prog. jezik

PyBase, kot že ime pove je vezan na Python (dejansko gre za knjižnico integrirano direktno v sam jezik). 

**Zakaj?**
V praksi, ko se srečujemo z izvedbo aplikacije, ki uporablja relacijsko podatkovno bazo, pogosto srečamo enega izmed dveh pristopov:

- Imamo ORM (object relational mapping), ki posamezne tabele preslika v razrede in potem operiramo direktno nad razredi, v ozadju, pa se vse operacije pretvorijo v SQL poizvedbe.
- Med nivojema baze in aplikacije moramo skrbeti za usklajenost strukture podatkovnega modela.

**Slabost** - omejeni smo na uporabo le enega jezika.

## Pristop indeksiranja

V klasičnih `DBMS`-jih programerju dovoljujemo, da ključe določa sam. V PyBase, pa bi se ključi določili sami (za primarni ključ bo vedno vzet indeks vrstice v matriki).

Pristop indeksiranja je treba omogočiti tudi nad polji, ki niso primarni ali tuji ključi, saj tako z uporabo principov, kot so `dvojiško iskanje` in `iskanje po "hash" tabeli` omogočimo hitro iskanje tudi po drugih poljih.

\pagebreak
### Uporaba relaciji - združevanje podatkov

Kadar združujemo podatke želimo ohranjati strukturo in ne direktno povezovati relaciji, kot to delajo klasični relacijski sistemi.
Primer rezultata, kjer za vse študente naberemo še njihove predmete:
SQL:
```SQL
SELECT students.*, courses.name AS 'course_name' FROM students
LEFT OUTER JOIN courses ON courses.id = students.course_id
```
```JSON
[
    {
        "fullname": "lorem ipsum",
        "email": "example@gmail.com",
        "course_name": "Course name"
    }, {
        "fullname": "lorem ipsum",
        "email": "example@gmail.com",
        "course_name": "Course2 name"
    }, {
        "fullname": "other user",
        "email": "other@gmail.com",
        "course_name": "COurse name"
    }
]
```

PyBase:
```JSON
[
    {
        "fullname": "lorem ipsum",
        "email": "example@gmail.com",
        "courses": [
            { "name": "Course name" },
            { "name": "Course 2 name"}
        ]
    }, {
        "fullname": "other user",
        "email": "other@gmail.com",
        "courses": [
            { "name": "Course name" }
        ]
    }
]
```

## Avtomatske migracije
Pogost problem ostalih relacijskih podatkovnih baz so migracije. Gre za problem, ki nastopi, kadar želimo strukturo z že 
vnešenimi podatki spremeniti (npr. tabeli `students` želimo dodati stolpec `birthdate`), to je seveda preprost primer
in dodati nekaj takega dosežemo v eni vrstici. Se pa takšni in drugačni primeri migraciji same strukture dogajajo zelo pogosto, zato bi v
implementaciji PyBase poskusili implementirati avtomatske migracije, ki bi glede na spremembe v sami strukturi, izvedli migracijo na nivoju 
podatkovnih tabel v `.bin` datotekah.

## Paralelizem
...?

## Področja uporabe
- Logiranje podatkov iz nekega kompleksnega sistema, kjer želimo hitro in strukturirano iskanje med napakami. 
- Podatkovni sistem za manjše aplikacije/mobilne aplikacije