(define (domain parallel-search-edge-cases)
  (:requirements :strips :typing)
  (:types token location)
  (:predicates
    (at ?token - token ?location - location)
    (edge ?from - location ?to - location)
  )

  (:action move
    :parameters (?token - token ?from - location ?to - location)
    :precondition (and (at ?token ?from) (edge ?from ?to))
    :effect (and (not (at ?token ?from)) (at ?token ?to))
  )
)
