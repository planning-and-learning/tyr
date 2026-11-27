(define (problem window-1) (:domain window) (:init (at 5 (open)) (at 10 (not (open)))) (:goal (done)) (:metric minimize (total-time)))
