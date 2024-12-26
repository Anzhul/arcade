template <typename T>
class UnsortedSet {
public:
  // EFFECTS:  Initializes this set to be empty.
  UnsortedSet();

  // MODIFIES: *this
  // EFFECTS:  Adds value to the set, if it isn't already in the set.
  void insert(const T &value);

  // MODIFIES: *this
  // EFFECTS:  Removes value from the set, if it is in the set.
  void remove(const T &value);

  // EFFECTS:  Returns whether value is in the set.
  bool contains(const T &value) const;

  // EFFECTS:  Returns the number of elements.
  int size() const;

  // EFFECTS:  Prints out the set in an arbitrary order.
  void print(std::ostream &os) const;

private:
  // MODIFIES: *this
  // EFFECTS:  Doubles the capacity of this set.
  void grow();

  // Initial size of a set.
  static const int INITIAL_SIZE = 5;

  T *elements;
  int capacity;
  int num_elements;
  // INVARIANTS:
  // elements points to the sole dynamic array associated with this set
  // capacity is the size of the array that elements points to
  // 0 <= num_elements && num_elements <= capacity
  // the first num_elements items in elements are the items in the set
  // the first num_elements items in elements contain no duplicates
};