;;; TREE-IL -> GLIL compiler

;; Copyright (C) 2001,2008,2009 Free Software Foundation, Inc.

;;;; This library is free software; you can redistribute it and/or
;;;; modify it under the terms of the GNU Lesser General Public
;;;; License as published by the Free Software Foundation; either
;;;; version 3 of the License, or (at your option) any later version.
;;;; 
;;;; This library is distributed in the hope that it will be useful,
;;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;;;; Lesser General Public License for more details.
;;;; 
;;;; You should have received a copy of the GNU Lesser General Public
;;;; License along with this library; if not, write to the Free Software
;;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

;;; Code:

(define-module (language tree-il analyze)
  #:use-module (srfi srfi-1)
  #:use-module (srfi srfi-9)
  #:use-module (system base syntax)
  #:use-module (system base message)
  #:use-module (language tree-il)
  #:export (analyze-lexicals
            report-unused-variables))

;; Allocation is the process of assigning storage locations for lexical
;; variables. A lexical variable has a distinct "address", or storage
;; location, for each procedure in which it is referenced.
;;
;; A variable is "local", i.e., allocated on the stack, if it is
;; referenced from within the procedure that defined it. Otherwise it is
;; a "closure" variable. For example:
;;
;;    (lambda (a) a) ; a will be local
;; `a' is local to the procedure.
;;
;;    (lambda (a) (lambda () a))
;; `a' is local to the outer procedure, but a closure variable with
;; respect to the inner procedure.
;;
;; If a variable is ever assigned, it needs to be heap-allocated
;; ("boxed"). This is so that closures and continuations capture the
;; variable's identity, not just one of the values it may have over the
;; course of program execution. If the variable is never assigned, there
;; is no distinction between value and identity, so closing over its
;; identity (whether through closures or continuations) can make a copy
;; of its value instead.
;;
;; Local variables are stored on the stack within a procedure's call
;; frame. Their index into the stack is determined from their linear
;; postion within a procedure's binding path:
;; (let (0 1)
;;   (let (2 3) ...)
;;   (let (2) ...))
;;   (let (2 3 4) ...))
;; etc.
;;
;; This algorithm has the problem that variables are only allocated
;; indices at the end of the binding path. If variables bound early in
;; the path are not used in later portions of the path, their indices
;; will not be recycled. This problem is particularly egregious in the
;; expansion of `or':
;;
;;  (or x y z)
;;    -> (let ((a x)) (if a a (let ((b y)) (if b b z))))
;;
;; As you can see, the `a' binding is only used in the ephemeral `then'
;; clause of the first `if', but its index would be reserved for the
;; whole of the `or' expansion. So we have a hack for this specific
;; case. A proper solution would be some sort of liveness analysis, and
;; not our linear allocation algorithm.
;;
;; Closure variables are captured when a closure is created, and stored
;; in a vector. Each closure variable has a unique index into that
;; vector.
;;
;;
;; The return value of `analyze-lexicals' is a hash table, the
;; "allocation".
;;
;; The allocation maps gensyms -- recall that each lexically bound
;; variable has a unique gensym -- to storage locations ("addresses").
;; Since one gensym may have many storage locations, if it is referenced
;; in many procedures, it is a two-level map.
;;
;; The allocation also stored information on how many local variables
;; need to be allocated for each procedure, and information on what free
;; variables to capture from its lexical parent procedure.
;;
;; That is:
;;
;;  sym -> {lambda -> address}
;;  lambda -> (nlocs . free-locs)
;;
;; address := (local? boxed? . index)
;; free-locs ::= ((sym0 . address0) (sym1 . address1) ...)
;; free variable addresses are relative to parent proc.

(define (make-hashq k v)
  (let ((res (make-hash-table)))
    (hashq-set! res k v)
    res))

(define (analyze-lexicals x)
  ;; bound-vars: lambda -> (sym ...)
  ;;  all identifiers bound within a lambda
  ;; free-vars: lambda -> (sym ...)
  ;;  all identifiers referenced in a lambda, but not bound
  ;;  NB, this includes identifiers referenced by contained lambdas
  ;; assigned: sym -> #t
  ;;  variables that are assigned
  ;; refcounts: sym -> count
  ;;  allows us to detect the or-expansion in O(1) time
  
  ;; returns variables referenced in expr
  (define (analyze! x proc)
    (define (step y) (analyze! y proc))
    (define (recur x new-proc) (analyze! x new-proc))
    (record-case x
      ((<application> proc args)
       (apply lset-union eq? (step proc) (map step args)))

      ((<conditional> test then else)
       (lset-union eq? (step test) (step then) (step else)))

      ((<lexical-ref> name gensym)
       (hashq-set! refcounts gensym (1+ (hashq-ref refcounts gensym 0)))
       (list gensym))
      
      ((<lexical-set> name gensym exp)
       (hashq-set! refcounts gensym (1+ (hashq-ref refcounts gensym 0)))
       (hashq-set! assigned gensym #t)
       (lset-adjoin eq? (step exp) gensym))
      
      ((<module-set> mod name public? exp)
       (step exp))
      
      ((<toplevel-set> name exp)
       (step exp))
      
      ((<toplevel-define> name exp)
       (step exp))
      
      ((<sequence> exps)
       (apply lset-union eq? (map step exps)))
      
      ((<lambda> vars meta body)
       (let ((locally-bound (let rev* ((vars vars) (out '()))
                              (cond ((null? vars) out)
                                    ((pair? vars) (rev* (cdr vars)
                                                        (cons (car vars) out)))
                                    (else (cons vars out))))))
         (hashq-set! bound-vars x locally-bound)
         (let* ((referenced (recur body x))
                (free (lset-difference eq? referenced locally-bound))
                (all-bound (reverse! (hashq-ref bound-vars x))))
           (hashq-set! bound-vars x all-bound)
           (hashq-set! free-vars x free)
           free)))
      
      ((<let> vars vals body)
       (hashq-set! bound-vars proc
                   (append (reverse vars) (hashq-ref bound-vars proc)))
       (lset-difference eq?
                        (apply lset-union eq? (step body) (map step vals))
                        vars))
      
      ((<letrec> vars vals body)
       (hashq-set! bound-vars proc
                   (append (reverse vars) (hashq-ref bound-vars proc)))
       (for-each (lambda (sym) (hashq-set! assigned sym #t)) vars)
       (lset-difference eq?
                        (apply lset-union eq? (step body) (map step vals))
                        vars))
      
      ((<fix> vars vals body)
       (hashq-set! bound-vars proc
                   (append (reverse vars) (hashq-ref bound-vars proc)))
       (lset-difference eq?
                        (apply lset-union eq? (step body) (map step vals))
                        vars))
      
      ((<let-values> vars exp body)
       (hashq-set! bound-vars proc
                   (let lp ((out (hashq-ref bound-vars proc)) (in vars))
                     (if (pair? in)
                         (lp (cons (car in) out) (cdr in))
                         (if (null? in) out (cons in out)))))
       (lset-difference eq?
                        (lset-union eq? (step exp) (step body))
                        vars))
      
      (else '())))
  
  (define (allocate! x proc n)
    (define (recur y) (allocate! y proc n))
    (record-case x
      ((<application> proc args)
       (apply max (recur proc) (map recur args)))

      ((<conditional> test then else)
       (max (recur test) (recur then) (recur else)))

      ((<lexical-set> name gensym exp)
       (recur exp))
      
      ((<module-set> mod name public? exp)
       (recur exp))
      
      ((<toplevel-set> name exp)
       (recur exp))
      
      ((<toplevel-define> name exp)
       (recur exp))
      
      ((<sequence> exps)
       (apply max (map recur exps)))
      
      ((<lambda> vars meta body)
       ;; allocate closure vars in order
       (let lp ((c (hashq-ref free-vars x)) (n 0))
         (if (pair? c)
             (begin
               (hashq-set! (hashq-ref allocation (car c))
                           x
                           `(#f ,(hashq-ref assigned (car c)) . ,n))
               (lp (cdr c) (1+ n)))))
      
       (let ((nlocs
              (let lp ((vars vars) (n 0))
                (if (not (null? vars))
                    ;; allocate args
                    (let ((v (if (pair? vars) (car vars) vars)))
                      (hashq-set! allocation v
                                  (make-hashq
                                   x `(#t ,(hashq-ref assigned v) . ,n)))
                      (lp (if (pair? vars) (cdr vars) '()) (1+ n)))
                    ;; allocate body, return number of additional locals
                    (- (allocate! body x n) n))))
             (free-addresses
              (map (lambda (v)
                     (hashq-ref (hashq-ref allocation v) proc))
                   (hashq-ref free-vars x))))
         ;; set procedure allocations
         (hashq-set! allocation x (cons nlocs free-addresses)))
       n)

      ((<let> vars vals body)
       (let ((nmax (apply max (map recur vals))))
         (cond
          ;; the `or' hack
          ((and (conditional? body)
                (= (length vars) 1)
                (let ((v (car vars)))
                  (and (not (hashq-ref assigned v))
                       (= (hashq-ref refcounts v 0) 2)
                       (lexical-ref? (conditional-test body))
                       (eq? (lexical-ref-gensym (conditional-test body)) v)
                       (lexical-ref? (conditional-then body))
                       (eq? (lexical-ref-gensym (conditional-then body)) v))))
           (hashq-set! allocation (car vars)
                       (make-hashq proc `(#t #f . ,n)))
           ;; the 1+ for this var
           (max nmax (1+ n) (allocate! (conditional-else body) proc n)))
          (else
           (let lp ((vars vars) (n n))
             (if (null? vars)
                 (max nmax (allocate! body proc n))
                 (let ((v (car vars)))
                   (hashq-set!
                    allocation v
                    (make-hashq proc
                                `(#t ,(hashq-ref assigned v) . ,n)))
                   (lp (cdr vars) (1+ n)))))))))
      
      ((<letrec> vars vals body)
       (let lp ((vars vars) (n n))
         (if (null? vars)
             (let ((nmax (apply max
                                (map (lambda (x)
                                       (allocate! x proc n))
                                     vals))))
               (max nmax (allocate! body proc n)))
             (let ((v (car vars)))
               (hashq-set!
                allocation v
                (make-hashq proc
                            `(#t ,(hashq-ref assigned v) . ,n)))
               (lp (cdr vars) (1+ n))))))

      ((<fix> vars vals body)
       (let lp ((vars vars) (n n))
         (if (null? vars)
             (let ((nmax (apply max
                                (map (lambda (x)
                                       (allocate! x proc n))
                                     vals))))
               (max nmax (allocate! body proc n)))
             (let ((v (car vars)))
               (if (hashq-ref assigned v)
                   (error "fixpoint procedures may not be assigned" x))
               (hashq-set! allocation v (make-hashq proc `(#t #f . ,n)))
               (lp (cdr vars) (1+ n))))))

      ((<let-values> vars exp body)
       (let ((nmax (recur exp)))
         (let lp ((vars vars) (n n))
           (if (null? vars)
               (max nmax (allocate! body proc n))
               (let ((v (if (pair? vars) (car vars) vars)))
                 (let ((v (car vars)))
                   (hashq-set!
                    allocation v
                    (make-hashq proc
                                `(#t ,(hashq-ref assigned v) . ,n)))
                   (lp (cdr vars) (1+ n))))))))
      
      (else n)))

  (define bound-vars (make-hash-table))
  (define free-vars (make-hash-table))
  (define assigned (make-hash-table))
  (define refcounts (make-hash-table))
  
  (define allocation (make-hash-table))
  
  (analyze! x #f)
  (allocate! x #f 0)

  allocation)


;;;
;;; Unused variable analysis.
;;;

;; <binding-info> records are used during tree traversals in
;; `report-unused-variables'.  They contain a list of the local vars
;; currently in scope, a list of locals vars that have been referenced, and a
;; "location stack" (the stack of `tree-il-src' values for each parent tree).
(define-record-type <binding-info>
  (make-binding-info vars refs locs)
  binding-info?
  (vars binding-info-vars)  ;; ((GENSYM NAME LOCATION) ...)
  (refs binding-info-refs)  ;; (GENSYM ...)
  (locs binding-info-locs)) ;; (LOCATION ...)

(define (report-unused-variables tree)
  "Report about unused variables in TREE.  Return TREE."

  (define (dotless-list lst)
    ;; If LST is a dotted list, return a proper list equal to LST except that
    ;; the very last element is a pair; otherwise return LST.
    (let loop ((lst    lst)
               (result '()))
      (cond ((null? lst)
             (reverse result))
            ((pair? lst)
             (loop (cdr lst) (cons (car lst) result)))
            (else
             (loop '() (cons lst result))))))

  (tree-il-fold (lambda (x info)
                  ;; X is a leaf: extend INFO's refs accordingly.
                  (let ((refs (binding-info-refs info))
                        (vars (binding-info-vars info))
                        (locs (binding-info-locs info)))
                    (record-case x
                      ((<lexical-ref> gensym)
                       (make-binding-info vars (cons gensym refs) locs))
                      (else info))))

                (lambda (x info)
                  ;; Going down into X: extend INFO's variable list
                  ;; accordingly.
                  (let ((refs (binding-info-refs info))
                        (vars (binding-info-vars info))
                        (locs (binding-info-locs info))
                        (src  (tree-il-src x)))
                    (define (extend inner-vars inner-names)
                      (append (map (lambda (var name)
                                     (list var name src))
                                   inner-vars
                                   inner-names)
                              vars))
                    (record-case x
                      ((<lexical-set> gensym)
                       (make-binding-info vars (cons gensym refs)
                                          (cons src locs)))
                      ((<lambda> vars names)
                       (let ((vars  (dotless-list vars))
                             (names (dotless-list names)))
                         (make-binding-info (extend vars names) refs
                                            (cons src locs))))
                      ((<let> vars names)
                       (make-binding-info (extend vars names) refs
                                          (cons src locs)))
                      ((<letrec> vars names)
                       (make-binding-info (extend vars names) refs
                                          (cons src locs)))
                      ((<fix> vars names)
                       (make-binding-info (extend vars names) refs
                                          (cons src locs)))
                      ((<let-values> vars names)
                       (make-binding-info (extend vars names) refs
                                          (cons src locs)))
                      (else info))))

                (lambda (x info)
                  ;; Leaving X's scope: shrink INFO's variable list
                  ;; accordingly and reported unused nested variables.
                  (let ((refs (binding-info-refs info))
                        (vars (binding-info-vars info))
                        (locs (binding-info-locs info)))
                    (define (shrink inner-vars refs)
                      (for-each (lambda (var)
                                  (let ((gensym (car var)))
                                    ;; Don't report lambda parameters as
                                    ;; unused.
                                    (if (and (not (memq gensym refs))
                                             (not (and (lambda? x)
                                                       (memq gensym
                                                             inner-vars))))
                                        (let ((name (cadr var))
                                              ;; We can get approximate
                                              ;; source location by going up
                                              ;; the LOCS location stack.
                                              (loc  (or (caddr var)
                                                        (find pair? locs))))
                                          (warning 'unused-variable loc name)))))
                                (filter (lambda (var)
                                          (memq (car var) inner-vars))
                                        vars))
                      (fold alist-delete vars inner-vars))

                    ;; For simplicity, we leave REFS untouched, i.e., with
                    ;; names of variables that are now going out of scope.
                    ;; It doesn't hurt as these are unique names, it just
                    ;; makes REFS unnecessarily fat.
                    (record-case x
                      ((<lambda> vars)
                       (let ((vars (dotless-list vars)))
                         (make-binding-info (shrink vars refs) refs
                                            (cdr locs))))
                      ((<let> vars)
                       (make-binding-info (shrink vars refs) refs
                                          (cdr locs)))
                      ((<letrec> vars)
                       (make-binding-info (shrink vars refs) refs
                                          (cdr locs)))
                      ((<fix> vars)
                       (make-binding-info (shrink vars refs) refs
                                          (cdr locs)))
                      ((<let-values> vars)
                       (make-binding-info (shrink vars refs) refs
                                          (cdr locs)))
                      (else info))))
                (make-binding-info '() '() '())
                tree)
  tree)
