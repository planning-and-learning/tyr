(define (domain witness)
  (:requirements :strips :existential-preconditions)
  (:predicates (edge ?x ?y) (done ?x))
  (:action mark
    :parameters (?x)
    :precondition (exists (?w) (edge ?x ?w))
    :effect (done ?x)))
