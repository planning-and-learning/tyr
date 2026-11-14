(define (domain lmcut-state-routing)
  (:requirements :strips :typing)
  (:types token)
  (:predicates
    (start ?token - token)
    (landmark ?token - token)
    (side ?token - token)
    (goal ?token - token)
  )

  (:action reach-landmark
    :parameters (?token - token)
    :precondition (start ?token)
    :effect (and
      (not (start ?token))
      (landmark ?token)
      (side ?token)
    )
  )

  (:action reach-goal
    :parameters (?token - token)
    :precondition (landmark ?token)
    :effect (goal ?token)
  )
)
