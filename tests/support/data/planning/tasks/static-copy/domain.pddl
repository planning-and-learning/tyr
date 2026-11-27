(define (domain static-copy)
  (:requirements :adl)
  (:predicates (present) (absent) (done))
  (:action mark :parameters () :precondition (and) :effect (done)))
