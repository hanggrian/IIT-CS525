# B+ Tree insertion & deletion

## [Insertion on a B+ Tree](https://www.programiz.com/dsa/insertion-on-a-b-plus-tree/)

Inserting an element into a B+ tree consists of three main events:
**searching the appropriate leaf**, **inserting** the element and
**balancing/splitting** the tree.

Let us understand these events below.

### Insertion operation

Before inserting an element into a B+ tree, these properties must be kept in
mind.

- The root has at least two children.
- Each node except root can have a maximum of `m` children and at least `m/2`
  children.
- Each node can contain a maximum of `m-1` keys and a minimum of `⌈m/2⌉ - 1`
  keys.

The following steps are followed for inserting an element.

1. Since every element is inserted into the leaf node, go to the appropriate
  leaf node.
2. Insert the key into the leaf node.

#### Case I

1. If the leaf is not full, insert the key into the leaf node in increasing
  order.

#### Case II

1. If the leaf is full, insert the key into the leaf node in increasing order
  and balance the tree in the following way.
2. Break the node at `m/2`-th position.
3. Add `m/2`-th key to the parent node as well.
4. If the parent node is already full, follow steps 2 to 3.

### Insertion example

Let us understand the insertion operation with the illustrations below.

The elements to be inserted are 5, 15, 25, 35, 45.

**Step 1**: Insert 5.

![](https://cdn.programiz.com/sites/tutorial2program/files/insert-1-b+tree.png)<br><small>Insert 5</small>

**Step 2**: Insert 15.

![](https://cdn.programiz.com/sites/tutorial2program/files/insert-2-b+tree.png)<br><small>Insert 15</small>

**Step 3**: Insert 25.

![](https://cdn.programiz.com/sites/tutorial2program/files/insert-3-b+tree.png)<br><small>Insert 25</small>

**Step 4**: Insert 35.

![](https://cdn.programiz.com/sites/tutorial2program/files/insert-4-b+tree.png)<br><small>Insert 35</small>

**Step 5**: Insert 45.

![](https://cdn.programiz.com/sites/tutorial2program/files/insert-5-b+tree.png)<br><small>Insert 45</small>

## [Deletion on a B+ Tree](https://www.programiz.com/dsa/deletion-from-a-b-plus-tree/)

Deleting an element on a B+ tree consists of three main events: **searching**
the node where the key to be deleted exists, deleting the key and balancing the
tree if required. **Underflow** is a situation when there is less number of keys
in a node than the minimum number of keys it should hold.

### Deletion operation

Before going through the steps below, one must know these facts about a B+ tree
of degree **m**.

1. A node can have a maximum of m children. (i.e. 3)
2. A node can contain a maximum of `m-1` keys. (i.e. 2)
3. A node should have a minimum of `⌈m/2⌉` children. (i.e. 2)
4. A node (except root node) should contain a minimum of `⌈m/2⌉ - 1` keys.
  (i.e. 1)

While deleting a key, we have to take care of the keys present in the internal
nodes (i.e. indexes) as well because the values are redundant in a B+ tree.
Search the key to be deleted then follow the following steps.

#### Case I

The key to be deleted is present only at the leaf node not in the indexes (or
internal nodes). There are two cases for it:

1. There is more than the minimum number of keys in the node. Simply delete the
  key.

![](https://cdn.programiz.com/sites/tutorial2program/files/deletion-1-b+tree.png)<br><small>Deleting 40 from B-tree</small>

2. There is an exact minimum number of keys in the node. Delete the key and
  borrow a key from the immediate sibling. Add the median key of the sibling
  node to the parent.

![](https://cdn.programiz.com/sites/tutorial2program/files/deletion-2-b+tree.png)<br><small>Deleting 5 from B-tree</small>

#### Case II

The key to be deleted is present in the internal nodes as well. Then we have to
remove them from the internal nodes as well. There are the following cases for
this situation.

1. If there is more than the minimum number of keys in the node, simply delete
  the key from the leaf node and delete the key from the internal node as well.
  Fill the empty space in the internal node with the inorder successor.

![](https://cdn.programiz.com/sites/tutorial2program/files/deletion-3-b+tree_0.png)<br><small>Deleting 45 from B-tree</small>

2. If there is an exact minimum number of keys in the node, then delete the key
  and borrow a key from its immediate sibling (through the parent). Fill the
  empty space created in the index (internal node) with the borrowed key.

![](https://cdn.programiz.com/sites/tutorial2program/files/deletion-4-b+tree_0.png)<br><small>Deleting 35 from B-tree</small>

3. This case is similar to Case II(1) but here, empty space is generated above
  the immediate parent node. After deleting the key, merge the empty space with
  its sibling. Fill the empty space in the grandparent node with the inorder
  successor.

![](https://cdn.programiz.com/sites/tutorial2program/files/deletion-5-b+tree_0.png)<br><small>Deleting 25 from B-tree</small>

#### Case III

In this case, the height of the tree gets shrinked. It is a little complicated.
Deleting 55 from the tree below leads to this condition. It can be understood in
the illustrations below.

![](https://cdn.programiz.com/sites/tutorial2program/files/deletion-6-b+tree_0.png)<br><small>Deleting 55 from B-tree</small>
