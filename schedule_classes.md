# Schedule classes

## [Recoverability-based](https://www.geeksforgeeks.org/cascadeless-in-dbms/)

- **Recoverable schedule:** Transactions must be committed in order. Dirty Read
  problem and Lost Update problem may occur.
- **Cascadeless schedule:** Dirty Read not allowed, means reading the data
  written by an uncommitted transaction is not allowed. Lost Update problem may
  occur.
- **Strict schedule:** Neither Dirty read nor Lost Update problem allowed, means
  reading or writing the data written by an uncommitted transaction is not
  allowed.

![](https://media.geeksforgeeks.org/wp-content/uploads/Types-of-schedules.png)

### Dirty read problem

When a Transaction reads data from uncommitted write in another transaction,
then it is known as Dirty Read. If that writing transaction failed, and that
written data may updated again. Therefore, this causes Dirty Read Problem.

In other words,

```
Reading the data written by an uncommitted transaction is called as dirty read.
```

It is called as dirty read because there is always a chance that the uncommitted
transaction might roll back later. Thus, uncommitted transaction might make
other transactions read a value that does not even exist. This leads to
inconsistency of the database.

For example, let’s say transaction 1 updates a row and leaves it uncommitted,
meanwhile, Transaction 2 reads the updated row. If transaction 1 rolls back the
change, transaction 2 will have read data that is considered never to have
existed.

![](https://media.geeksforgeeks.org/wp-content/uploads/20190806174713/33633.png)

Note that there is no Dirty Read problem, is a transaction is reading from
another committed transaction. So, no rollback required.

![](https://media.geeksforgeeks.org/wp-content/uploads/20190806175901/33633-2.png)

### Cascading rollback

If in a schedule, failure of one transaction causes several other dependent
transactions to rollback or abort, then such a schedule is called as a Cascading
Rollback or Cascading Abort or Cascading Schedule. It simply leads to the
wastage of CPU time.

These Cascading Rollbacks occur because of **Dirty Read problems.**

For example, transaction $T_1$ writes uncommitted $x$ that is read by
Transaction $T_2$. Transaction $T_2$ writes uncommitted $x$ that is read by
Transaction $T_3$. Suppose at this point $T_1$ fails. $T_1$ must be rolled back,
since $T_2$ is dependent on $T_1$, $T_2$ must be rolled back, and since $T_3$ is
dependent on $T_2$, $T_3$ must be rolled back.

Because of $T_1$ rollback, all $T_2$, $T_3$, and $T_4$ should also be rollback
(Cascading dirty read problem).

This phenomenon, in which a single transaction failure leads to a series of
transaction rollbacks is called **Cascading rollback.**

### Cascadeless schedule

This schedule avoids all possible *Dirty Read Problem*.

In Cascadeless Schedule, if a transaction is going to perform read operation on
a value, it has to wait until the transaction who is performing write on that
value commits. That means there must not be **Dirty Read.** Because Dirty Read
Problem can cause *Cascading Rollback*, which is inefficient.

Cascadeless Schedule avoids cascading aborts/rollbacks (ACA). Schedules in which
transactions read values only after all transactions whose changes they are
going to read commit are called cascadeless schedules. Avoids that a single
transaction abort leads to a series of transaction rollbacks. A strategy to
prevent cascading aborts is to disallow a transaction from reading uncommitted
changes from another transaction in the same schedule.

In other words, if some transaction $T_j$ wants to read value updated or written
by some other transaction $T_i$, then the commit of $T_j$ must read it after the
commit of $T_i$.

![](https://media.geeksforgeeks.org/wp-content/uploads/20190806182957/1133633-2.png)

Note that Cascadeless schedule allows only committed read operations. However,
it allows uncommitted write operations.

Also note that Cascadeless Schedules are always recoverable, but all [recoverable](https://www.geeksforgeeks.org/dbms-types-of-recoverability-of-schedules-and-easiest-way-to-test-schedule-set-2/)
transactions may not be Cascadeless Schedule.

## Conflict serializibility

[See extra content](https://github.com/hanggrian/IIT-CS525/blob/assets/ext4.pdf)

## 2PL

[See question & answer](https://gateoverflow.in/369645/2pl-made-easy-full-syllabus-test)
