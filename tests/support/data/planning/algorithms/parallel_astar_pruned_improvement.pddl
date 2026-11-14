(define (problem parallel-astar-pruned-improvement)
  (:domain transport)
  (:objects
    start via merge goal - location
    truck - vehicle
  )
  (:init
    (= (total-cost) 0)
    (at truck start)
    (road start merge)
    (= (road-length start merge) 100)
    (road start via)
    (= (road-length start via) 1)
    (road via merge)
    (= (road-length via merge) 1)
    (road merge goal)
    (= (road-length merge goal) 1)
  )
  (:goal (at truck goal))
  (:metric minimize (total-cost))
)
