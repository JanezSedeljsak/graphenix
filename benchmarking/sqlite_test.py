import sqlite3
con = sqlite3.connect('example.db')
cur = con.cursor()

"""
typedef struct _user
{
    u_int64_t id;
    char name[30];
    char surname[50];
    char email[100];
    char password[100];
} user;
"""

table = """ CREATE TABLE user (
            ID INT,
            Email VARCHAR(100) NOT NULL,
            Password CHAR(100) NOT NULL,
            First_Name CHAR(30) NOT NULL,
            Last_Name CHAR(50)
        ); """
 
cur.execute(table)

for i in range(1000):
    cur.execute("INSERT INTO user(ID, Email, Password, First_Name, Last_Name) VALUES(1, 'janez.sedeljsak@gmail.com', 'test123', 'janez', 'sedeljsak')")
    

con.commit()
cur.close()