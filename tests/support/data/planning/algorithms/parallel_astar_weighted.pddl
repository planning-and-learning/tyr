(define (problem parallel-astar-weighted)
  (:domain transport)
  (:objects
    start via goal - location
    truck - vehicle
  )
  (:init
    (= (total-cost) 0)
    (at truck start)
    (road start goal)
    (= (road-length start goal) 100)
    (road start via)
    (= (road-length start via) 1)
    (road via goal)
    (= (road-length via goal) 1)
  )
  (:goal (at truck goal))
  (:metric minimize (total-cost))
)
