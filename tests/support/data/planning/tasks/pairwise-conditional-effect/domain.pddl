(define (domain pairwise-conditional-effect)
  (:requirements :adl :typing :numeric-fluents)
  (:types item)
  (:predicates
    (ready ?x - item)
    (allowed ?x ?y - item)
    (marked ?x - item))
  (:functions (value ?x - item))

  (:action apply
    :parameters (?x - item)
    :precondition (ready ?x)
    :effect (forall (?y - item)
      (when (allowed ?x ?y)
        (and
          (marked ?y)
          (increase (value ?y) 1)))))
)
