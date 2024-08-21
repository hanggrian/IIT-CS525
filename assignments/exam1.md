<!-- hotfix: KaTeX -->
<!-- https://github.com/yzane/vscode-markdown-pdf/issues/21/ -->
<script type="text/javascript" src="http://cdn.mathjax.org/mathjax/latest/MathJax.js?config=TeX-AMS-MML_HTMLorMML"></script>
<script type="text/x-mathjax-config">MathJax.Hub.Config({ tex2jax: { inlineMath: [['$', '$']] }, messageStyle: 'none' });</script>

# Mid exam

> ## Database context
>
> Consider the following database schema (__*RateMyProfDB*__) to answer all the
  questions below.
>
> <small>Table: Professor<br>PK: PID</small><br>PID | <br><br>Pname | <br><br>Papers | <br><br>Topic
> --- | --- | --- | ---
> **109** | Steven | 10 | Java
> **110** | Francis | 50 | Databases
> **111** | Daniel | 40 | Java
> **112** | Joy | 20 | Java
>
> <small>Table: Rating<br>PK: SID + PID<br>FK: SID, PID</small><br>SID | <br><br><br>PID | <br><br><br>Score | <br><br><br>Attended
> --- | --- | --- | ---
> **23** | **109** | 6 | 60
> **23** | **110** | 10 | 70
> **25** | **111** | 8 | 40
> **27** | **111** | 9 | 100
> **27** | **109** | 4 | 20
> **33** | **109** | 5 | 80
> **33** | **112** | 1 | 4
>
> <small>Table: Student<br>PK: SID</small><br>SID | <br><br>Sname | <br><br>Uni | <br><br>GPA
> --- | --- | --- | ---
> **23** | Michelle | Illinois Tech | 3.05
> **25** | Tomas | UChi | 2.71
> **27** | Biden | Illinois Tech | 3.66
> **33** | John | UChi | 4.80
>
> <small>Metadata</small><br>Item | <br>Description
> --- | ---
> PID | professor identification number (auto number field)
> Pname | name of Profeesor (not nullable field)
> Papers | number of papers published by professor (nullable field)
> Topic | professor's topic/area of speciality (nullable field)
> SID | student's identification number (non-auto increment field)
> Sname | name of student (not nullable)
> Uni | a university students attend (not nullable)
> GPA | student's grade average point (nullable)
> Score | rating score from student (can only be between 0 and 10), and not nullable.
> Attended | percentage of students that participated in the rating (not nullable)
> PK | primary key
> FK | foreign key

## SQL problem 1

> Write the SQL commands to create the database and the relations provided in
  the diagram above.

```sql
CREATE SCHEMA RateMyProfDB;
USE RateMyProfDB;

CREATE TABLE Professor(
  PID INT AUTO_INCREMENT PRIMARY KEY,
  Pname VARCHAR(20) NOT NULL,
  Papers INT,
  Topic VARCHAR(10)
);

CREATE TABLE Student(
  SID INT PRIMARY KEY,
  Sname VARCHAR(20) NOT NULL,
  Uni VARCHAR(20) NOT NULL,
  GPA DOUBLE(3,2)
);

CREATE TABLE Rating(
  SID INT NOT NULL,
  PID INT NOT NULL,
  Score INT NOT NULL,
  Attended INT NOT NULL,
  PRIMARY KEY(SID, PID),
  FOREIGN KEY(SID) REFERENCES Student(SID),
  FOREIGN KEY(PID) REFERENCES Professor(PID),
  CHECK(Score >= 0 AND Score <= 10)
);
```

## RA-SQL problem 1

> Select all professors whose specialty is not Java and have published at most
  20 papers.

$$
\sigma_{Topic \neq 'Java' \land Papers \leq 20} (Professor)
$$

```sql
SELECT *
FROM Professor
WHERE Topic != 'Java' AND Papers <= 20;
```

## RA-SQL problem 2

> Find all the (PIDs) of professors who have received a rating score grater than
  that of Professor Joy.

$$
\pi_{PID} (\sigma_{P.PID = R.PID} (\sigma_{Pname = 'Joy'} (Rating \times Professor))) \\\\
\textsf{or} \\\\
\pi_{PID} (\sigma_{Pname = 'Joy'} (Professor \Join Rating))
$$

```sql
SELECT P.PID
FROM Professor P, Rating R
WHERE P.PID = R.PID AND Score > SOME(
  SELECT Score
  FROM Rating
  WHERE P.Pname = 'Joy'
);
```

## RA-SQL problem 3

> Find the name and average score given to each professor who has been rated at
  most once.

$$
\pi_{Pname, \textsf{avg}(Score)} (\sigma_{\textsf{count}(PID) \leq 1} (Professor \Join Rating))
$$

```sql
SELECT P.Pname, AVG(R.Score)
FROM Professor P, Rating R
WHERE P.PID = R.PID
GROUP BY P.PID
HAVING COUNT(P.PID) <= 1;
```

## B+-tree problem 1

> Suppose each B+-tree node can hold up to four (4) pointers and three (3) keys.
  Draw the B+-tree that would result after insertion (a) and delection (b)
  operations as shown below.
>
> First, insert **4.** Then, insert **6.** Next, insert **8.** Finally, insert
  **10.**

Inserting 4:

![Inserted 4.](https://github.com/hanggrian/IIT-CS525/raw/assets/assignments/exam1_bptree1_1.jpg)

Inserting 6:

![Inserted 6.](https://github.com/hanggrian/IIT-CS525/raw/assets/assignments/exam1_bptree1_2.jpg)

Inserting 8:

![Inserted 8.](https://github.com/hanggrian/IIT-CS525/raw/assets/assignments/exam1_bptree1_3.jpg)

Inserting 10:

![Inserted 10.](https://github.com/hanggrian/IIT-CS525/raw/assets/assignments/exam1_bptree1_4.jpg)

## B+-tree problem 2

> First, delete **13.** Then, delete **15.** Finally, delete **1.**

Deleting 13:

![Deleted 13.](https://github.com/hanggrian/IIT-CS525/raw/assets/assignments/exam1_bptree2_1.jpg)

Deleting 15:

![Deleted 15.](https://github.com/hanggrian/IIT-CS525/raw/assets/assignments/exam1_bptree2_2.jpg)

Deleting 1:

![Deleted 1.](https://github.com/hanggrian/IIT-CS525/raw/assets/assignments/exam1_bptree2_3.jpg)
