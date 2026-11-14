(define (problem lmcut-weighted-alternative-test)
  (:domain lmcut-weighted-alternative)
  (:init
    (= (total-cost) 0)
  )
  (:goal (and
    (left-goal)
    (right-goal)
  ))
  (:metric minimize (total-cost))
)
