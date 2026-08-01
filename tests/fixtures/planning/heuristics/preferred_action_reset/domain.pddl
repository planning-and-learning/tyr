(define (domain preferred-action-reset)
  (:requirements :strips)
  (:predicates (available) (goal))

  (:action achieve
    :parameters ()
    :precondition (available)
    :effect (goal))

  (:action consume
    :parameters ()
    :precondition (available)
    :effect (not (available))))
