(define (domain work)
  (:requirements :strips :typing :durative-actions)
  (:types obj)
  (:predicates (a ?x - obj) (b ?x - obj) (free))
  (:durative-action work
    :parameters (?x - obj)
    :duration (= ?duration 2)
    :condition (and (at start (a ?x)) (over all (free)))
    :effect (and (at start (not (a ?x))) (at end (b ?x))))
  (:durative-action block
    :parameters ()
    :duration (= ?duration 1)
    :condition (at start (free))
    :effect (and (at start (not (free))) (at end (free)))))
