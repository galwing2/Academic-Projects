"""A class represnting a node in an AVL tree"""
class AVLNode(object):
    """Constructor, you are allowed to add more fields. 
    
    @type key: int or None
    @param key: key of your node
    @type value: string
    @param value: data of your node
    """
    def __init__(self, key, value):
        self.key = key
        self.value = value
        self.left: AVLNode = None
        self.right: AVLNode = None
        self.parent: AVLNode = None
        self.height = -1
        self.old_bf = None
        self.bf = 0
        
    """returns whether self is not a virtual node 
    @rtype: bool
    @returns: False if self is a virtual node, True otherwise.
    """
    def is_real_node(self):
        return self.key is not None
    
    def make_virtual_children(self):
        n_r = AVLNode(None, None)
        n_l = AVLNode(None, None)
        n_r.parent = self
        n_l.parent = self
        self.right = n_r
        self.left = n_l

    def balance_factor(self):
        return self.left.height - self.right.height
        
    def update_height(self):
        self.old_bf = self.bf
        self.height = 1 + max(self.right.height, self.left.height)
        self.bf = self.balance_factor()

    def make_virtual(self):
        self.key = None
        self.value = None
        self.left = None
        self.right = None
        self.height = -1


"""
A class implementing an AVL tree.
"""

class AVLTree(object):

    """
    Constructor, you are allowed to add more fields.  

    """
    def __init__(self):
        self.root = None
        self._size = 0
        self.max = self.root
        self.bf0_cnt = 0


    """searches for a node in the dictionary corresponding to the key

    @type key: int
    @param key: a key to be searched
    @rtype: AVLNode
    @returns: node corresponding to key
    """
    def search(self, key):
        assert isinstance(key, int), "Key should be an integer"

        node = self.root
        while node.is_real_node():
            if key == node.key:
                return node
            elif key < node.key:
                node = node.left
            else:
                node = node.right
        
        return None

    """inserts a new node into the dictionary with corresponding key and value
    @type key: int
    @pre: key currently does not appear in the dictionary
    @param key: key of item that is to be inserted to self
    @type val: string
    @param val: the value of the item
    @param start: can be either "root" or "max"
    @rtype: int
    @returns: the number of rebalancing operation due to AVL rebalancing
    """
    def insert(self, key, val, start="root"):
        assert isinstance(key, int), "Key should be an integer"

        self._size += 1
        # empty tree case
        if self.root is None:
            self.root = AVLNode(key, val)
            self.root.make_virtual_children()
            self.root.update_height()
            self.max = self.root
            self.bf0_cnt = 1
            return 0
        
        # maximum insertion
        if start == "max":
            inserted_node = self._insert_bst_from_the_max(key, val)
        else:  # regular insertion 
            inserted_node = self._insert_bst_from_a_node(self.root, key, val)
            # updating max
            if inserted_node.key > self.max.key:
                self.max = inserted_node
        
        # rotations
        fix_cnt = 0
        node: AVLNode = inserted_node
        while node is not None:
            last_height = node.height
            node.update_height()

            bf = node.balance_factor()
            last_bf = node.old_bf

            if last_height == -1:
                self.bf0_cnt += 1
            elif last_bf == 0 and bf != 0:
                self.bf0_cnt -= 1
            elif last_bf != 0 and bf == 0:
                self.bf0_cnt += 1
            
            next_ = node.parent
            
            if abs(bf) < 2 and last_height == node.height:
                break
            if abs(bf) == 2:
                # rotations
                fix_cnt += self._rotate(node)
                break
            else:
                fix_cnt += 1
            
            node = next_
        
        # changing the root if necessary
        while self.root.parent is not None:
            self.root = self.root.parent

        return fix_cnt


    """deletes node from the dictionary
    @type node: AVLNode
    @pre: node is a real pointer to a node in self
    @rtype: int
    @returns: the number of rebalancing operation due to AVL rebalancing
    """
    def delete(self, node):
        curr = node.parent
        # deleting
        fix_cnt, already_done = self._deletion_from_bst(node)

        if already_done:
            return fix_cnt
        
        while curr is not None:
            curr.update_height()
            bf = curr.balance_factor()
            old_bf = curr.old_bf

            if old_bf == 0 and bf != 0:
                self.bf0_cnt -= 1
            elif old_bf != 0 and bf == 0:
                self.bf0_cnt += 1
            
            next_ = curr.parent # parent might change during rotation
            if abs(bf) >= 2:
                fix_cnt += self._rotate(curr)
            else:
                fix_cnt += 1
            curr = next_
        
        # changing the root if necessary
        while (self.root is not None) and (self.root.parent is not None):
            self.root = self.root.parent
        
        return fix_cnt


    """returns an array representing dictionary 

    @rtype: list
    @returns: a sorted list according to key of touples (key, value) representing the data structure
    """
    def avl_to_array(self):
        def _rec(node: AVLNode, arr):
            if node.is_real_node():
                _rec(node.left, arr)
                arr.append((node.key, node.value))
                _rec(node.right, arr)

        arr = list()
        _rec(self.root, arr)
        return arr	

    """returns the number of items in dictionary 

    @rtype: int
    @returns: the number of items in dictionary 
    """
    def size(self):
        return self._size

    """returns the root of the tree representing the dictionary

    @rtype: AVLNode
    @returns: the root, None if the dictionary is empty
    """
    def get_root(self):
        return self.root


    """gets amir's suggestion of balance factor

    @returns: the number of nodes which have balance factor equals to 0 divided by the total number of nodes
    """
    def get_amir_balance_factor(self):
        # empty tree case.
        if self.size() == 0:
            return 0
        
        return self.bf0_cnt / self.size()

    
    """
    Standart BST insert, returns the node we are adding to the tree
    """
    def _insert_bst_from_a_node(self, node: AVLNode, key, val):
        while node.is_real_node():
            if key < node.key:
                node = node.left
            else:
                node = node.right
        
        node.key = key
        node.value = val
        node.make_virtual_children()
        return node
    
    def _insert_bst_from_the_max(self, key, val):
        if key > self.max.key:
            inserted_node = self.max.right
            inserted_node.key = key
            inserted_node.value = val
            inserted_node.make_virtual_children()
            self.max = inserted_node
            return inserted_node

        curr = self.max
        while (curr != self.root) and (key < curr.parent.key):
            curr = curr.parent
        
        return self._insert_bst_from_a_node(curr, key, val)

    def _left_rotate(self, node: AVLNode, update_bf=True):
        son = node.right
        
        old_bf_n = node.balance_factor()
        old_son_bf = son.balance_factor()

        node.right = son.left
        node.right.parent = node
        son.left = node
        son.parent = node.parent
        if son.parent is not None:
            if son.parent.key > son.key:
                son.parent.left = son
            else:
                son.parent.right = son
        
        node.parent = son

        node.update_height()
        son.update_height()

        if not update_bf:
            return
        
        if old_bf_n == 0 and node.balance_factor() != 0:
            self.bf0_cnt -= 1
        if old_bf_n != 0 and node.balance_factor() == 0:
            self.bf0_cnt += 1
        if old_son_bf == 0 and son.balance_factor() != 0:
            self.bf0_cnt -= 1
        if old_son_bf != 0 and son.balance_factor() == 0:
            self.bf0_cnt += 1
    
    def _right_rotate(self, node: AVLNode, update_bf=True):
        son = node.left
        
        old_bf_n = node.balance_factor()
        old_son_bf = son.balance_factor()
        
        node.left = son.right
        node.left.parent = node
        son.right = node
        son.parent = node.parent
        if son.parent is not None:
            if son.parent.key > son.key:
                son.parent.left = son
            else:
                son.parent.right = son
        
        node.parent = son

        node.update_height()
        son.update_height()

        if not update_bf:
            return

        if old_bf_n == 0 and node.balance_factor() != 0:
            self.bf0_cnt -= 1
        if old_bf_n != 0 and node.balance_factor() == 0:
            self.bf0_cnt += 1
        if old_son_bf == 0 and son.balance_factor() != 0:
            self.bf0_cnt -= 1
        if old_son_bf != 0 and son.balance_factor() == 0:
            self.bf0_cnt += 1
    
    def _rotate(self, node):
        rotate_cnt = 1
        if node.balance_factor() == -2:
            if node.right.balance_factor() == 1:
                self._right_rotate(node.right, update_bf=True)
                rotate_cnt += 1
            self._left_rotate(node)
        elif node.balance_factor() == 2:
            if node.left.balance_factor() == -1:
                self._left_rotate(node.left, update_bf=True)
                rotate_cnt += 1
            self._right_rotate(node)
        return rotate_cnt
    
    def successor(self, node: AVLNode):
        if node.right.is_real_node():
            # minimum in right subtree
            p = node.right
            while p.is_real_node():
                p = p.left
            return p.parent

        y = node.parent
        while (y is not None) and (node == y.right):
            node = y
            y = node.parent
        
        return y
    
    """
    Regular deletion from bst
    """
    def _deletion_from_bst(self, node):
        # tracking the maximum
        original_parent = node.parent
        if node == self.max:
            self.max = original_parent

        if (node.right.is_real_node()) and (node.left.is_real_node()): # two sons
            successor = self.successor(node)
            k, v = successor.key, successor.value
            rotations = self.delete(successor)  #  will be only called once per original deletion
            node.value = v
            node.key = k
            return rotations, True

        if node.right.is_real_node() or node.left.is_real_node(): # one son case
            parent = node.parent
            new_son = node.right if node.right.is_real_node() else node.left  # which son 

            if node is not self.root:
                # is the node we're deleting the right or left son?
                if parent.right == node:
                    parent.right = new_son
                else:
                    parent.left = new_son
            else:
                self.root = new_son
            new_son.parent = parent
    
        else:  # leaf
            if node.balance_factor() == 0:
                self.bf0_cnt -= 1
            if node is self.root:
                self.root = None	
            node.make_virtual()
        
        self._size -= 1
        return 0, False
 
