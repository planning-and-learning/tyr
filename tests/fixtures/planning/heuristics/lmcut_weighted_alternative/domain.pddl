(define (domain lmcut-weighted-alternative)
  (:requirements :strips :action-costs)
  (:predicates
    (left-goal)
    (right-goal)
  )
  (:functions
    (total-cost) - number
  )

  (:action reach-left
    :parameters ()
    :precondition ()
    :effect (and
      (left-goal)
      (increase (total-cost) 30)
    )
  )

  (:action reach-right
    :parameters ()
    :precondition ()
    :effect (and
      (right-goal)
      (increase (total-cost) 30)
    )
  )

  (:action reach-both
    :parameters ()
    :precondition ()
    :effect (and
      (left-goal)
      (right-goal)
      (increase (total-cost) 50)
    )
  )
)
