(define (problem effect-validity-problem)
  (:domain effect-validity)
  (:init
    (enabled low)
    (= (value low) 1)
    (= (value high) 1))
  (:goal (enabled low))
)
