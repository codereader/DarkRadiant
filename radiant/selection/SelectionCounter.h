#ifndef SELECTIONCOUNTER_H_
#define SELECTIONCOUNTER_H_

/* greebo: This class maintains an internal counter. By calling the operator() method
 * with a Selectable as argument, it checks whether this selectable is selected or not
 * and increases/decreases the internal counter _count accordingly.
 * 
 * Additionally, the call is passed on to SelectionChangeCallback in either of the above cases. 
 */
class SelectionCounter {
private:
  std::size_t _count;
  SelectionChangeCallback _onChanged;

public:
  typedef const Selectable& first_argument_type;

  SelectionCounter(const SelectionChangeCallback& onChanged)
    : _count(0), _onChanged(onChanged) {}
  
  void operator()(const Selectable& selectable) {
    if (selectable.isSelected()) {
      ++_count;
    }
    else {
      ASSERT_MESSAGE(_count != 0, "selection counter underflow");
      --_count;
    }

    _onChanged(selectable);
  }
  
  bool empty() const {
    return _count == 0;
  }
  
  std::size_t size() const {
    return _count;
  }
};

#endif /*SELECTIONCOUNTER_H_*/
