/**
 * FibonacciHeap
 * An implementation of Fibonacci heap over positive integers.
 *
 */
public class FibonacciHeap
{
    public HeapNode min;
    public int c;
    public int size;
    public int treeCnt;
    public int linkCnt;
    public int cutCnt;
    /**
     *
     * Constructor to initialize an empty heap.
     * pre: c >= 2.
     *
     */
    public FibonacciHeap(int c) {
        this.min = null;
        this.c = c;
        this.size = 0;
        this.treeCnt = 0;
        this.linkCnt = 0;
        this.cutCnt = 0;
    }

    /**
     *
     * pre: key > 0
     *
     * Insert (key,info) into the heap and return the newly generated HeapNode.
     *
     */
    public HeapNode insert(int key, String info)
    {
        this.size++; //adding new node
        this.treeCnt++; // adding one tree of rank 0

        HeapNode inserted = new HeapNode(key, info);
        // empty heap case
        if (this.min == null) {
            this.min = inserted;
            return inserted;
        }
        // adding new node to the heap
        min.linkHorizontal(inserted);
        // updating min
        if (inserted.key < this.min.key) {
            this.min = inserted;
        }

        return inserted;
    }

    /**
     *
     * Return the minimal HeapNode, null if empty.
     *
     */
    public HeapNode findMin() {
        return this.min;
    }

    /**
     *
     * Delete the minimal item.
     * Return the number of links.
     *
     */
    public int deleteMin() {
        java.util.ArrayList<Object> buckets = new java.util.ArrayList<>();

        // decreasing heap size
        this.size--;
        // adding minimum children to the root
        HeapNode minChild = this.min.child;
        this.min.linkHorizontal(minChild);
        HeapNode.nullifyParent(minChild);

        // special case - one node in the heap
        if (this.min.next == min) {
            this.min = null;
            this.treeCnt = 0;
            return 0;
        }
        // deleting the minimum pointer
        HeapNode pre = this.min.prev;
        HeapNode next = this.min.next;
        pre.next = next;
        next.prev = pre;

        // new min will be the pre-node - random selection (could be any other node in the root level)
        this.min = pre;

//        ArrayList<HeapNode> bucket = new ArrayList<>();
        int linkCounter = HeapNode.toBuckets(buckets, this.min);
        this.min = HeapNode.fromBuckets(buckets);

        // updating tree count
        this.treeCnt = 0;
        for (int i = 0; i < buckets.size(); i++)  {
            if (buckets.get(i) != null) treeCnt++;
        }
        // updating link counter that happened due to the buckets
        this.linkCnt += linkCounter;
        return linkCounter;
    }

    /**
     *
     * pre: 0<diff<x.key
     *
     * Decrease the key of x by diff and fix the heap.
     * Return the number of cuts.
     *
     */
    public int decreaseKey(HeapNode x, int diff)
    {
        x.key -= diff;
        int cuts;
        if (x.parent == null || x.key >= x.parent.key) {  // heap structure maintained
            cuts = 0;
        } else { // cascading cuts according to the algorithm
            cuts = cascadingCut(x);
            this.cutCnt += cuts;
        }

        // updating min
        if (x.key < this.min.key) {
            this.min = x;
        }

        return cuts;
    }

    /**
     *
     * Delete the x from the heap.
     * Return the number of links.
     *
     */
    public int delete(HeapNode x) {
        //making it the minimum and delete it
        decreaseKey(x, x.key);
        int links = deleteMin();
        return links;
    }


    /**
     *
     * Return the total number of links.
     *
     */
    public int totalLinks() {
        return this.linkCnt;
    }


    /**
     *
     * Return the total number of cuts.
     *
     */
    public int totalCuts() {
        return this.cutCnt;
    }


    /**
     *
     * Meld the heap with heap2
     *
     */
    public void meld(FibonacciHeap heap2)
    {
        // meld tree roots to one big list
        this.min.linkHorizontal(heap2.min);
        // checking minimum
        if (heap2.min.key < this.min.key) {
            this.min = heap2.min;
        }
        heap2.min = null; // delete original heap
        // updating properties
        this.size += heap2.size;
        this.linkCnt += heap2.linkCnt;
        this.treeCnt += heap2.treeCnt;
        this.cutCnt += heap2.cutCnt;
    }

    /**
     *
     * Return the number of elements in the heap
     *
     */
    public int size() {
        return this.size;
    }


    /**
     *
     * Return the number of trees in the heap.
     *
     */
    public int numTrees() {
        return this.treeCnt;
    }

    /**
     *
     * Delete HeapNode x from its parent
     *
     */
    public void cutFromParent(HeapNode x) {
        HeapNode parentX = x.parent;

        x.parent = null; //delete link to parent
        x.markCnt = 0;
        if (parentX == null) {
            return;
        }

        parentX.rank = parentX.rank-1; // x is gone so child amount decreasing
        if (x.next == x) {  // if x is the only child
            parentX.child = null;
        } else {
            parentX.child = x.next;
            x.prev.next = x.next;
            x.next.prev = x.prev;
        }
        // adding x the root level
        x.next = x;
        x.prev = x;
        this.min.linkHorizontal(x);
        this.treeCnt++;
    }

    /**
     *
     * Delete x from its parent and do cascading cut if necessary
     * Returns the number of cuts
     *
     */
    public int cascadingCut(HeapNode x) {
        HeapNode parentX = x.parent;
        int cuts = 1;
        cutFromParent(x);
        if (parentX.parent != null) {
            if (parentX.markCnt < c-1) {
                parentX.markCnt++; // update mark
            } else {
                cuts += cascadingCut(parentX); // if too many marks do cascading cut to the parent
            }
        }
        return cuts;
    }
    /**
     * Class implementing a node in a Fibonacci Heap.
     *
     */
    public static class HeapNode {
        public int key;
        public String info;
        public HeapNode child;
        public HeapNode next;
        public HeapNode prev;
        public HeapNode parent;
        public int rank;
        public int markCnt;

        public HeapNode(int key, String info) {
            this.key = key;
            this.info = info;
            this.child = null;
            this.next = this;
            this.prev = this;
            this.parent = null;
            this.rank = 0;
            this.markCnt = 0;
        }

        @Override
        public String toString() {
            return String.format("<K:%d, R:%d>", this.key, this.rank);
        }

        /**
         *
         * adding new node the level of another node.
         * adding the new node siblings as well.
         */
        public void linkHorizontal(HeapNode other) {
            if (other == null) {
                return;
            }
            HeapNode otherEnd = other.prev;
            HeapNode originalNext = this.next;
            this.next = other;
            other.prev = this;
            otherEnd.next = originalNext;
            originalNext.prev = otherEnd;
        }

        /**
         *
         * Given two nodes from the same degree link them and make a new tree from a higher degree
         */
        public static HeapNode linkSameRankTree(HeapNode node1, HeapNode node2) {
            // make the min node in node1
            if (node1.key > node2.key) {
                HeapNode tmp = node1;
                node1 = node2;
                node2 = tmp;
            }

            node1.rank++;

            node2.parent = node1;
            // node1 has no child special case
            if (node1.child == null) {
                node1.child = node2;
                node2.next = node2;
                node2.prev = node2;
                return node1;
            }

            HeapNode connectingNode = node1.child.prev;
            connectingNode.linkHorizontal(node2);
            return node1;
        }

        /**
         *
         * "binary" adding of HeapNodes into bucket
         *  Returns the number of links
         */
        public static int toBuckets(java.util.ArrayList<Object> bucket, HeapNode start) {
            HeapNode node = start;

            node.prev.next = null;
            int linkCounter = 0;
            while (node != null) {
                HeapNode currNode = node;
                node = node.next; //in order for not losing the next node during the process

                // the node has no siblings now when changing it.
                currNode.prev = currNode;
                currNode.next = currNode;

                // "binary adding"
                while (currNode.rank < bucket.size() && bucket.get(currNode.rank) != null) {
                    currNode = linkSameRankTree((HeapNode) bucket.get(currNode.rank), currNode);
                    bucket.set(currNode.rank-1, null);
                    linkCounter++;
                }

                while (currNode.rank >= bucket.size()) {
                    bucket.add(null);
                }

                bucket.set(currNode.rank, currNode);
            }
            return linkCounter;
        }

        /**
         *
         * from the bucket, create the new heap by returning the new min.
         */
        public static HeapNode fromBuckets(java.util.ArrayList<Object> bucket) {
            HeapNode newMin = null;
            for (int i = 0; i < bucket.size(); i++) {
                if (bucket.get(i) != null) {
                    HeapNode curr = (HeapNode) bucket.get(i);
                    if (newMin == null) { //starting case
                        newMin = curr;
                        newMin.next = newMin;
                        newMin.prev = newMin;
                    } else {
                        // linking roots
                        newMin.linkHorizontal(curr);
                        // updating min
                        if (curr.key < newMin.key) {
                            newMin = (HeapNode) bucket.get(i);
                        }
                    }
                }
            }
            return newMin;
        }

        /**
         *
         * Given a node - delete from it and from its sibling the parents - make it null.
         * Returns the number of cuts
         *
         */
        public static void nullifyParent(HeapNode node) {
            if (node == null) {
                 return;
            }
            HeapNode p = node;
            do {
                node.parent = null;
                node = node.next;
            } while (node != p);
        }
    }
}
