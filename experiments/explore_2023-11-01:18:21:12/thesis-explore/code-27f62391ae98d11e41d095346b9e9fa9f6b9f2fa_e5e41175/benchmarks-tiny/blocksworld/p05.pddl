(define 
(problem BLOCKS-5-1)
(:domain blocksworld)
(:objects A D C E B )
(:init 
(clear B) 
(clear E) 
(clear C) 
(on-table D) 
(on-table E)
(on-table C)
(on B A) 
(on A D) 
(arm-empty)
)
(:goal 
(and 
(on D C) 
(on C B) 
(on B A) 
(on A E)
))
)
