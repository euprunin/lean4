Push
   Theorem ReflIf (A : Type)
                  (R : A -> A -> Bool)
                  (Symmetry : Pi x y, R x y -> R y x)
                  (Transitivity : Pi x y z, R x y -> R y z -> R x z)
                  (Linked : Pi x, exists y, R x y)
                  :
                  Pi x, R x x :=
       fun x, ExistsElim (Linked x)
             (fun (w : A) (H : R x w),
                 let L1 : R w x := Symmetry x w H
                 in Transitivity x w x H L1)
Pop

Push
  (*
    Same example but using forall instead of Pi and => instead of ->
  *)
  Theorem ReflIf (A : Type)
                 (R : A -> A -> Bool)
                 (Symmetry : forall x y, R x y => R y x)
                 (Transitivity : forall x y z, R x y => R y z => R x z)
                 (Linked : forall x, exists y, R x y)
                 :
                 forall x, R x x :=
    ForallIntro (fun x,
         ExistsElim (ForallElim Linked x)
            (fun (w : A) (H : R x w),
                let L1 : R w x := (MP (ForallElim (ForallElim Symmetry x) w) H)
                in (MP (MP (ForallElim (ForallElim (ForallElim Transitivity x) w) x) H) L1)))
Pop

Scope
  (* Same example again. *)
  Variable A : Type
  Variable R : A -> A -> Bool
  Axiom Symmetry  {x y : A} : R x y -> R y x
  Axiom Transitivity {x y z : A} : R x y -> R y z -> R x z
  Axiom Linked (x : A) : exists y, R x y

  Theorem ReflIf (x : A) : R x x :=
      ExistsElim (Linked x) (fun (w : A) (H : R x w),
            let L1 : R w x := Symmetry H
            in Transitivity H L1)

  (* Even more compact proof of the same theorem *)
  Theorem ReflIf2 (x : A) : R x x :=
      ExistsElim (Linked x) (fun w H, Transitivity H (Symmetry H))

(*
  The command EndScope exports both theorem to the main scope
  The variables and axioms in the scope become parameters to both theorems.
*)
EndScope

(* Display the last two theorems *)
Show Environment 2