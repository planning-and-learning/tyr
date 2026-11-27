(define (domain transfer)
  (:requirements :strips :conditional-effects :fluents)
  (:predicates (ready ?x) (done ?x))
  (:functions (fuel ?x))
  (:action finish
    :parameters (?x)
    :precondition (ready ?x)
    :effect (and (when (ready ?x) (done ?x)) (decrease (fuel ?x) 1))))
