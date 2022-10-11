# PyBase

## Relacijske pod. baze
PyBase knjižnca temelji na principu relacijskih podatkovnih baz. Podatki se shranjujejo v binarne datoteke, s katerimi potem operiramo v C.

Prednost tega pristopa je zelo hitro iskanje, sploh kadar imamo točno dolečeno strukturo in točno določeno velikosti za posamezen stolpec v tabeli;
Če vemo da je posamezna vrstica dolga `N` bajtov potem lahko `M`-to vrstico najdemo, tako da se pomaknemo na `N*M`-ti bajt.

Primer. strukture:
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
 
Za lažjo predstavo lahko tabelo `students` definiramo, kot matriko z `N` vrstic in `M` stolpcev. 

Kadar za vsakega učenca želimo dobiti njegov `email` iz vsake vrstice izberemo drugi stolpec, ki predstavlja željeno polje. Posledično, za tak nabor potrebujemo tabelo, 
ki bo velikosti `N` in v posameznem polju tabele, shranimo email.

## Integracija direktno v prog. jeziku

## Pristop indeksiranja

V klasičnih `SQL` `DBMS`-jih programerju dovoljujemo, da ključe določa sam. V PyBase, pa bi ključe določama sama,
kot priparni ključ, bo vedno določen kot indeks vrstice v matriki.

### Uporaba relaciji - združevanje podatkov

Kadar združujemo podatke želimo ohranjati strukturo in ne direktno povezovati relaciji, kot dela to SQL.
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
```Python
my_schema.get(my_schema['users'].with(my_schema['courses']))
```
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
vnešenimi podatki spremeniti samo strukturo (npr. tabeli `students` želimo dodati stolpec `birth_date`), to je seveda preprost primer
in dodati nekaj takega dosežemo v eni vrstici. Se pa takšni in drugačni primeri migraciji same strukture dogajajo zelo pogosto, zato bi v
implementaciji PyBase poskusili implementirati avtomatske migracije, ki bi glede na spremembe v sami strukturi, izvedli migracijo na nivoju 
podatkovnih tabel v `.bin` datotekah.

## Področja uporabe

- Logiranje podatkov iz nekega kompleksnega sistema, kjer želimo določeno strukturo in hitro iskanje napak. 
- Preproste 