(define (domain iw-goal-before-novelty)
  (:requirements :strips :numeric-fluents)
  (:predicates (stage-0) (stage-1) (stage-2))
  (:functions (progress))

  (:action advance-0
    :parameters ()
    :precondition (stage-0)
    :effect (and (not (stage-0)) (stage-1))
  )

  (:action advance-1
    :parameters ()
    :precondition (stage-1)
    :effect (and (not (stage-1)) (stage-2))
  )

  (:action finish
    :parameters ()
    :precondition (stage-2)
    :effect (and (not (stage-2)) (increase (progress) 1))
  )
)
