(define (domain mixed)
  (:requirements :strips :typing :non-deterministic :probabilistic-effects :conditional-effects)
  (:types block)
  (:predicates (p ?b - block) (q ?b - block) (r))
  (:action a
    :parameters (?x - block)
    :precondition (p ?x)
    :effect (oneof (and (q ?x) (probabilistic 0.3 (r) 0.7 (not (p ?x))))
                   (forall (?y - block) (when (p ?y) (probabilistic 0.5 (oneof (q ?y) (r))))))))
