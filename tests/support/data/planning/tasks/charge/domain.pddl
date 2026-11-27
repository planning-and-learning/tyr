(define (domain charge) (:requirements :strips :durative-actions :duration-inequalities)
  (:predicates (empty) (full))
  (:durative-action charge :parameters () :duration (and (>= ?duration 1) (<= ?duration 3))
    :condition (at start (empty)) :effect (and (at start (not (empty))) (at end (full)))))
